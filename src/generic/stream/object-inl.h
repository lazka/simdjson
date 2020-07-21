namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

really_inline object::object() noexcept : field{}, error{UNINITIALIZED} {}
really_inline object::object(stream::value &parent, const uint8_t *key_string) noexcept : field{key_string, parent.json, parent.depth+1}, error{SUCCESS} {}
really_inline object::object(stream::value &parent, error_code _error) noexcept : field{nullptr, parent.json, 0}, error{_error} {}
really_inline simdjson_result<stream::field&> object::operator*() noexcept {
  return simdjson_result<stream::field&>(field, error);
}
really_inline bool object::operator==(const object &) noexcept { return !field._key.buf; }
really_inline bool object::operator!=(const object &) noexcept { return !!field._key.buf; }
really_inline object &object::operator++() noexcept { advance(); return *this; }

really_inline simdjson_result<value> object::operator[](std::string_view key) noexcept {
  if (error) { return { field, error }; }
  // Resume where we left off
  while (field._key.buf) {
    if (field.key() == key) { return field; }
    advance();
  }
  // TODO Come back around and search the first n fields?
  return { field, NO_SUCH_FIELD };
}

really_inline void object::advance() noexcept {
  // The user MUST NOT continue iterating if there is an error.
  SIMDJSON_ASSUME(!error);

  // Finish iterating any partly-iterated child arrays/objects
  field.finish();

  if ((error = field.json->next_object_field().get(field._key.buf))) { return; }
}

really_inline simdjson_result<stream::object> object::try_begin(stream::value &parent) noexcept {
  SIMDJSON_ASSUME(parent.depth == parent.json->depth);
  const uint8_t *key_string;
  if (auto error = parent.json->begin_object().get(key_string)) { return { parent, error }; }
  return object(parent, key_string);
}

really_inline object object::begin(stream::value &parent) noexcept {
  SIMDJSON_ASSUME(parent.depth == parent.json->depth);
  const uint8_t *key_string;
  if (auto error = parent.json->begin_object().get(key_string)) { return { parent, error }; }
  return { parent, key_string };
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
