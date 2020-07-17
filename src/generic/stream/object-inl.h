namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

really_inline object::object() noexcept : field{}, finished{}, error{} {}
really_inline object::object(stream::json *json, uint32_t depth, bool _finished, error_code _error) noexcept : field{json, depth}, finished{_finished}, error{_error} {}
really_inline object::object(stream::value &parent, error_code _error) noexcept : object(parent.json, parent.depth+1, true, _error) {}
really_inline simdjson_result<stream::field&> object::operator*() noexcept {
  return simdjson_result<stream::field&>(field, error);
}
really_inline bool object::operator!=(object &) noexcept { return !finished; }
really_inline object &object::operator++() noexcept { advance(); return *this; }

really_inline simdjson_result<value> object::operator[](std::string_view key) noexcept {
  // Resume where we left off
  while (!finished) {
    if (error || field.key() == key) { return { field, error }; }
    advance();
  }
  // TODO Come back around and search the first n fields?
  return { field, NO_SUCH_FIELD };
}

really_inline void object::advance() noexcept {
  // We should never get here we are finished or if there is an error! The user is supposed to stop
  // on error, and != stops on finished.
  SIMDJSON_ASSUME(!finished && !error);

  // Finish iterating any partly-iterated child arrays/objects
  field.finish();

  // Check whether we are finished
  finished = field.json->advance_if_end('}');
  if (finished) { logger::log_end_event("object", field.json); }

  // Jump to the <value> in , "key" : <value>
  bool has_next = field.json->advance_if(',', '"', ':');
  if (!finished && !has_next) { logger::log_error("missing comma, key or :", field.json); }
  error = !finished && !has_next ? TAPE_ERROR : SUCCESS;
}

really_inline simdjson_result<object> object::try_begin(stream::value &parent) noexcept {
  if (!parent.json->advance_if_start('{')) {
    logger::log_error("not an object", parent.json);
    return simdjson_result<object>(parent, INCORRECT_TYPE);
  }
  return begin(parent, true);
}

really_inline object object::begin(stream::value &parent, error_code error) noexcept {
  return begin(parent, parent.json->advance_if_start('{'), error);
}

really_inline object object::begin(stream::value &parent, bool is_object, error_code error) noexcept {
  SIMDJSON_ASSUME(!is_object || parent.json->depth == parent.depth+1);

  logger::log_start_event("object", parent.json);
  if (!is_object) { logger::log_error("not an object", parent.json); }
  error = error ? error : (is_object ? SUCCESS : INCORRECT_TYPE);

  // Check for {}
  bool finished = !error && parent.json->advance_if('}');
  if (finished) { logger::log_end_event("empty object", parent.json); }

  // Jump to the <value> in { "key" : <value>
  bool has_next = !error && parent.json->advance_if('"', ':');
  if (!finished && !has_next) { logger::log_error("missing key or :", parent.json); }
  error = error ? error : (!finished && !has_next ? TAPE_ERROR : SUCCESS);

  return { parent.json, parent.depth+1, finished, error };
}

really_inline object object::end() noexcept {
  return {};
}

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object>::simdjson_result(SIMDJSON_IMPLEMENTATION::stream::object &&value) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::object>(std::forward<SIMDJSON_IMPLEMENTATION::stream::object>(value)) {}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object>::simdjson_result(SIMDJSON_IMPLEMENTATION::stream::value &parent, error_code error) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::object>({ parent, error }, error) {}

really_inline SIMDJSON_IMPLEMENTATION::stream::object simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object>::begin() noexcept {
  return first;
}
really_inline SIMDJSON_IMPLEMENTATION::stream::object simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object>::end() noexcept {
  return {};
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object>::operator[](std::string_view key) noexcept {
  if (error()) { return { first.field, error() }; }
  return first[key];
}

} // namespace simdjson
