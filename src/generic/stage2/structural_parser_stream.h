// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2.h" (this simplifies amalgation)

#include "generic/stage2/structural_stream.h"
#include "generic/stage2/tape_writer.h"
#include "generic/stage2/atomparsing.h"

namespace { // Make everything here private
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct structural_parser_stream {
  /** Lets you append to the tape */
  tape_writer tape;
  /** The parser. */
  dom_parser_implementation &parser;
  /** Next write location in the string buf for string parsing */
  uint8_t *current_string_buf_loc;

  // For non-streaming, to pass an explicit 0 as next_structural, which enables optimizations
  really_inline structural_parser_stream(dom_parser_implementation &_parser)
    : tape{_parser.doc->tape.get()},
      parser{_parser},
      current_string_buf_loc{_parser.doc->string_buf.get()} {
  }

  WARN_UNUSED really_inline error_code start_scope(uint32_t depth) {
    parser.containing_scope[depth].tape_index = next_tape_index();
    parser.containing_scope[depth].count = 0;
    tape.skip(); // We don't actually *write* the start element until the end.
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code start_document() {
    log_start();
    log_start_value("document");
    return start_scope(0);
  }

  WARN_UNUSED really_inline error_code start_object(uint32_t depth) {
    log_start_value("object");
    return start_scope(depth);
  }

  WARN_UNUSED really_inline error_code start_array(uint32_t depth) {
    log_start_value("array");
    return start_scope(depth);
  }

  // this function is responsible for annotating the start of the scope
  really_inline error_code end_scope(uint32_t depth, internal::tape_type start, internal::tape_type end) noexcept {
    // write our doc->tape location to the header scope
    // The root scope gets written *at* the previous location.
    tape.append(parser.containing_scope[depth].tape_index, end);
    // count can overflow if it exceeds 24 bits... so we saturate
    // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
    const uint32_t start_tape_index = parser.containing_scope[depth].tape_index;
    const uint32_t count = parser.containing_scope[depth].count;
    const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
    // This is a load and an OR. It would be possible to just write once at doc->tape[d.tape_index]
    tape_writer::write(parser.doc->tape[start_tape_index], next_tape_index() | (uint64_t(cntsat) << 32), start);
    return SUCCESS;
  }

  really_inline error_code end_object(uint32_t depth) {
    log_end_value("object");
    return end_scope(depth, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
  }
  really_inline error_code end_array(uint32_t depth) {
    log_end_value("array");
    return end_scope(depth, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
  }
  really_inline error_code end_document() {
    log_end_value("document");
    return end_scope(0, internal::tape_type::ROOT, internal::tape_type::ROOT);
  }

  really_inline uint32_t next_tape_index() {
    return uint32_t(tape.next_tape_loc - parser.doc->tape.get());
  }

  // increment_count increments the count of keys in an object or values in an array.
  // Note that if you are at the level of the values or elements, the count
  // must be increment in the preceding depth (depth-1) where the array or
  // the object resides.
  really_inline void increment_count() {
    parser.containing_scope[depth - 1].count++; // we have a key value pair in the object at parser.depth - 1
  }

  really_inline uint8_t *on_start_string() noexcept {
    // we advance the point, accounting for the fact that we have a NULL termination
    tape.append(current_string_buf_loc - parser.doc->string_buf.get(), internal::tape_type::STRING);
    return current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline void on_end_string(uint8_t *dst) noexcept {
    uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    current_string_buf_loc = dst + 1;
  }

  WARN_UNUSED really_inline error_code parse_string(const uint8_t *src) {
    uint8_t *dst = on_start_string();
    dst = stringparsing::parse_string(src, dst);
    if (dst == nullptr) {
      return error(STRING_ERROR, "Invalid escape in string");
    }
    on_end_string(dst);
    return STRING_ERROR;
  }
  WARN_UNUSED really_inline error_code key(const uint8_t *src) {
    log_value("key");
    return parse_string(src);
  }
  WARN_UNUSED really_inline error_code string(const uint8_t *src, UNUSED size_t remaining_len=0) {
    log_value("string");
    return parse_string(src);
  }
  WARN_UNUSED really_inline error_code number(const uint8_t *src) {
    log_value("number");
    if (!numberparsing::parse_number(src, tape)) { return error(NUMBER_ERROR, "Invalid number"); }
    return SUCCESS;
  }
  really_inline error_code number(const uint8_t *src, size_t remaining_len) {
    /**
    * We need to make a copy to make sure that the string is space terminated.
    * This is not about padding the input, which should already padded up
    * to len + SIMDJSON_PADDING. However, we have no control at this stage
    * on how the padding was done. What if the input string was padded with nulls?
    * It is quite common for an input string to have an extra null character (C string).
    * We do not want to allow 9\0 (where \0 is the null character) inside a JSON
    * document, but the string "9\0" by itself is fine. So we make a copy and
    * pad the input with spaces when we know that there is just one input element.
    * This copy is relatively expensive, but it will almost never be called in
    * practice unless you are in the strange scenario where you have many JSON
    * documents made of single atoms.
    */
    uint8_t *copy = static_cast<uint8_t *>(malloc(remaining_len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      return MEMALLOC_ERROR;
    }
    memcpy(copy, src, remaining_len);
    memset(copy + remaining_len, ' ', SIMDJSON_PADDING);
    error_code result = number(copy); // parse_number does not throw
    free(copy);
    return result;
  }

  WARN_UNUSED really_inline error_code true_atom(const uint8_t *src) {
    log_value("true");
    if (!atomparsing::is_valid_true_atom(src)) { return T_ATOM_ERROR; }
    tape.append(0, internal::tape_type::TRUE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code true_atom(const uint8_t *src, size_t remaining_len) {
    log_value("true");
    if (!atomparsing::is_valid_true_atom(src, remaining_len)) { return T_ATOM_ERROR; }
    tape.append(0, internal::tape_type::TRUE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code false_atom(const uint8_t *src) {
    log_value("false");
    if (!atomparsing::is_valid_false_atom(src)) { return F_ATOM_ERROR; }
    tape.append(0, internal::tape_type::FALSE_VALUE);
    return SUCCESS;
  }

  WARN_UNUSED really_inline error_code false_atom(const uint8_t *src, size_t remaining_len) {
    log_value("false");
    if (!atomparsing::is_valid_false_atom(src, remaining_len)) { return F_ATOM_ERROR; }
    tape.append(0, internal::tape_type::FALSE_VALUE);
    return SUCCESS;
  }
}; // struct structural_parser_stream

} // namespace stage2

template<bool STREAMING>
WARN_UNUSED static really_inline error_code parse_structural_stream(dom_parser_implementation &parser, dom::document &doc) noexcept {
  parser.doc = doc;
  structural_parser_stream visitor(parser);
  return structural_stream::parse<STREAMING>(visitor);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
