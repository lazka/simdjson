#include "generic/stage2/logger.h"
#include "generic/stage2/structural_iterator.h"

namespace { // Make everything here private
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct structural_stream : structural_iterator {
  /** Lets you append to the tape */
  tape_writer tape;
  /** Current depth (nested objects and arrays) */
  uint32_t depth{0};

  really_inline structural_stream(dom_parser_implementation &_parser, uint32_t start_structural_index)
    : structural_iterator(_parser, start_structural_index),
      tape{parser.doc->tape.get()}
  }

  WARN_UNUSED really_inline error_code start_scope(const char *type, bool parent_is_array) {
    log_start_value(type);
    parser.is_array[depth] = parent_is_array;
    depth++;
    if (depth >= parser.max_depth()) { return visitor.error(DEPTH_ERROR, "Exceeded max depth!"); }
    return SUCCESS;
  }
  WARN_UNUSED really_inline error_code start_document() {
    return start_scope("document", false);
  }
  WARN_UNUSED really_inline error_code start_object(bool parent_is_array) {
    return start_scope("object", parent_is_array);
  }
  WARN_UNUSED really_inline error_code start_array(bool parent_is_array) {
    return start_scope("array", parent_is_array);
  }

  really_inline void end_scope(const char *type) noexcept {
    log_end_value(type);
    depth--;
  }
  really_inline void end_document() {
    end_scope("document");
  }
  really_inline void end_object() {
    end_scope("object");
  }
  really_inline void end_array() {
    end_scope("array");
  }

  really_inline uint32_t next_tape_index() {
    return uint32_t(tape.next_tape_loc - parser.doc->tape.get());
  }

  WARN_UNUSED really_inline error_code finish() {
    end_document();
    parser.next_structural_index = uint32_t(current_structural + 1 - &parser.structural_indexes[0]);

    if (depth != 0) {
      log_error("Unclosed objects or arrays!");
      return TAPE_ERROR;
    }

    return SUCCESS;
  }

  really_inline void init() {
    log_start();
  }

  WARN_UNUSED really_inline error_code start() {
    // If there are no structurals left, return EMPTY
    if (at_end(parser.n_structural_indexes)) {
      return EMPTY;
    }

    init();
    // Push the root scope (there is always at least one scope)
    if (start_document()) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }

  really_inline void log_value(const char *type) {
    logger::log_line(*this, "", type, "");
  }

  static really_inline void log_start() {
    logger::log_start();
  }

  really_inline void log_start_value(const char *type) {
    logger::log_line(*this, "+", type, "");
    if (logger::LOG_ENABLED) { logger::log_depth++; }
  }

  really_inline void log_end_value(const char *type) {
    if (logger::LOG_ENABLED) { logger::log_depth--; }
    logger::log_line(*this, "-", type, "");
  }

  really_inline void log_error(const char *error) {
    logger::log_line(*this, "", "ERROR", error);
  }

  template<typename T, bool STREAMING>
  WARN_UNUSED static really_inline error_code parse(T &visitor, dom_parser_implementation &dom_parser, dom::document &doc) noexcept;
}; // struct structural_parser

template<typename T, bool STREAMING>
WARN_UNUSED really_inline error_code structural_stream::parse(T &visitor, dom_parser_implementation &dom_parser, dom::document &doc) noexcept {
  dom_parser.doc = &doc;
  stage2::structural_stream stream(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
  error_code error = parser.start();
  if (error) { return error; }

  //
  // Read first value
  //
  {
    const uint8_t *value = parser.current_char();
    switch (*value) {
    case '{':
      if ((error = parser.start_object(false) || (error = visitor.start_object(parser.depth))) { return error; };
      goto object_begin;
    case '[':
      if ((error = parser.start_array(false)) || (error = visitor.start_array(parser.depth))) { return error; };
      // Make sure the outer array is closed before continuing; otherwise, there are ways we could get
      // into memory corruption. See https://github.com/simdjson/simdjson/issues/906
      if (!STREAMING) {
        if (parser.buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]] != ']') {
          return TAPE_ERROR;
        }
      }
      goto array_begin;
    case '"': if ((error = visitor.string(value, parser.remaining_len())) { return error; }; goto finish;
    case 't': if ((error = visitor.true_atom(value, parser.remaining_len())) { return error; }; goto finish;
    case 'f': if ((error = visitor.false_atom(value, parser.remaining_len())) { return error; }; goto finish;
    case 'n': if ((error = visitor.null_atom(value, parser.remaining_len())) { return error; }; goto finish;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      // Next line used to be an interesting functional programming exercise with
      // a lambda that gets passed to another function via a closure. This would confuse the
      // clangcl compiler under Visual Studio 2019 (recent release).
      if ((error = visitor.number(value, parser.remaining_len())) { goto error; }
      goto finish;
    default:
      return visitor.error(TAPE_ERROR, "Document starts with a non-value character");
    }
  }

//
// Object parser states
//
object_begin:
  switch (parser.advance_char()) {
  case '"':
    if ((error = visitor.on_key(parser.current())) { return error; }
    goto object_key_state;
  }
  case '}':
    if ((error = visitor.end_object(parser.depth))) { return error; }
    parser.end_object();
    goto scope_end;
  default:
    return visitor.on_error(TAPE_ERROR, "Object does not start with a key");
  }

object_key_state: {
  if (parser.advance_char() != ':' ) { return visitor.on_error(TAPE_ERROR, "Missing colon after key in object"); }
  const uint8_t *value = parser.advance();
  switch (*value) {
    case '{': if ((error = parser.start_object(false)) || (error = visitor.start_object(parser.depth))) { return error; }; goto object_begin;
    case '[': if ((error = parser.start_array(false)) || (error = visitor.start_array(parser.depth))) { return error; }; goto array_begin;
    case '"': if ((error = visitor.string(value))) { return error; }; break;
    case 't': if ((error = visitor.true_atom(value))) { return error; }; break;
    case 'f': if ((error = visitor.false_atom(value))) { return error; }; break;
    case 'n': if ((error = visitor.null_atom(value))) { return error; }; break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if ((error = visitor.number(value))) { return error; }; break;
    default:
      return visitor.error(TAPE_ERROR, "Non-value found when value was expected!");
  }

object_continue:
  switch (parser.advance_char()) {
  case ',': {
    const uint8_t *key = parser.advance();
    if (*key != '"' ) { return visitor.error(TAPE_ERROR, "Key string missing at beginning of field in object"); }
    if ((error = visitor.key(key))) { return error; }
    goto object_key_state;
  case '}':
    if ((error = visitor.end_object(parser.depth))) { return error; }
    parser.end_object();
    goto scope_end;
  default:
    return visitor.error(TAPE_ERROR, "No comma between object fields");
  }

scope_end:
  if (parser.depth == 1) { goto finish; }
  if (parser.parser.is_array[parser.depth]) { goto array_continue; }
  goto object_continue;

//
// Array parser states
//
array_begin:
  if (parser.peek_next_char() == ']') {
    parser.advance_char();
    if ((error = visitor.end_array(parser.depth)) { return error; }
    parser.end_array();
    goto scope_end;
  }

main_array_switch: {
  const uint8_t *value;
  switch (*value) {
    case '{': if ((error = parser.start_object(true)) || (error = visitor.start_object(parser.depth))) { return error; }; goto object_begin;
    case '[': if ((error = parser.start_array(true)) || (error = visitor.start_array(parser.depth))) { return error; }; goto array_begin;
    case '"': if ((error = visitor.string(value))) { return error; }; break;
    case 't': if ((error = visitor.true_atom(value))) { return error; }; break;
    case 'f': if ((error = visitor.false_atom(value))) { return error; }; break;
    case 'n': if ((error = visitor.null_atom(value))) { return error; }; break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if ((error = visitor.number(value))) { return error; }; break;
    default:
      return visitor.error(TAPE_ERROR, "Non-value found when value was expected!");
  }

array_continue:
  switch (parser.advance_char()) {
  case ',':
    goto main_array_switch;
  case ']':
    if ((error = visitor.end_array(parser.depth)) { return error; }
    parser.end_array();
    goto scope_end;
  default:
    return visitor.error(TAPE_ERROR, "Missing comma between array values");
  }

finish:
  return parser.finish();
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
