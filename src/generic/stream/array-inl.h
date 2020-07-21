namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

really_inline array::array() noexcept : value{}, error{} {}
really_inline array::array(stream::json *json, uint32_t depth, error_code _error) noexcept : value(json, depth), error{_error} {}
really_inline array::array(stream::value &parent, error_code _error) noexcept : array(parent.json, parent.depth+1, _error) {}
really_inline simdjson_result<value> array::operator*() noexcept { return { value, error }; }
really_inline bool array::operator!=(array &) noexcept { return value.json->depth < value.depth; }
really_inline array &array::operator++() noexcept { advance(); return *this; }

really_inline void array::advance() noexcept {
  // The user MUST NOT continue iterating if there is an error.
  SIMDJSON_ASSUME(!error);

  // Finish iterating any partly-iterated child arrays/objects
  value.finish();

  value.json->next_array_element();
}

really_inline simdjson_result<stream::array> array::try_begin(stream::value &parent) noexcept {
  SIMDJSON_ASSUME(parent.depth == parent.json->depth);
  bool has_element;
  if (auto error = parent.json->begin_array().get(has_element)) { return { parent, error }; }
  return array(parent);
}

really_inline array array::begin(stream::value &parent) noexcept {
  SIMDJSON_ASSUME(parent.depth == parent.json->depth);
  bool has_element;
  if (auto error = parent.json->begin_array().get(has_element)) { return { parent, error }; }
  return { parent };
}

really_inline array array::end() noexcept {
  return {};
}

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array>::simdjson_result(SIMDJSON_IMPLEMENTATION::stream::array &&value) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::array>(std::forward<SIMDJSON_IMPLEMENTATION::stream::array>(value)) {}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array>::simdjson_result(SIMDJSON_IMPLEMENTATION::stream::value &parent, error_code error) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::array>({ parent, error }, error) {}

really_inline SIMDJSON_IMPLEMENTATION::stream::array simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array>::begin() noexcept {
  return first;
}
really_inline SIMDJSON_IMPLEMENTATION::stream::array simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array>::end() noexcept {
  return {};
}

} // namespace simdjson
