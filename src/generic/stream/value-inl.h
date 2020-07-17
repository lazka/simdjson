namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

really_inline value::value() noexcept : json{}, depth{} {}
really_inline value::value(stream::json *_json, uint32_t _depth) noexcept : json{_json}, depth{_depth} {}
really_inline value::value(const value &other) noexcept = default;

really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array> value::get_array() && noexcept {
  consumed = true;
  return array::try_begin(*this);
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object> value::get_object() && noexcept {
  consumed = true;
  return object::try_begin(*this);
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::raw_json_string> value::get_raw_json_string() && noexcept {
  logger::log_event("raw_json_string", json);
  const uint8_t *str = consume();
  bool error = str[0] != '"';
  if (error) { logger::log_error("not a string", json); }
  ++json;
  return { raw_json_string(&str[1]), error ? INCORRECT_TYPE : SUCCESS };
}
really_inline simdjson_result<std::string_view> value::get_string() && noexcept {
  logger::log_event("string", json);
  auto [str, error] = std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_raw_json_string();
  if (error) { return error; }
  return str.unescape(json->string_buf);
}
really_inline simdjson_result<double> value::get_double() && noexcept {
  logger::log_event("double", json);
  return stage2::numberparsing::parse_double(consume());
}
really_inline simdjson_result<uint64_t> value::get_uint64() && noexcept {
  logger::log_event("unsigned", json);
  return stage2::numberparsing::parse_unsigned(consume());
}
really_inline simdjson_result<int64_t> value::get_int64() && noexcept {
  logger::log_event("integer", json);
  return stage2::numberparsing::parse_integer(consume());
}
really_inline simdjson_result<bool> value::get_bool() && noexcept {
  logger::log_event("bool", json);
  const uint8_t *src = consume();
  auto not_true = stage2::atomparsing::str4ncmp(src, "true");
  auto not_false = stage2::atomparsing::str4ncmp(src, "fals") | (src[4] ^ 'e');
  bool error = (not_true && not_false) || stage2::is_not_structural_or_whitespace(src[not_true ? 5 : 4]);
  return simdjson_result<bool>(!not_true, error ? INCORRECT_TYPE : SUCCESS);
}

#if SIMDJSON_EXCEPTIONS
really_inline value::operator array() && noexcept(false) { return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_array(); }
really_inline value::operator object() && noexcept(false) { return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_object(); }
really_inline value::operator uint64_t() && noexcept(false) { return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_uint64(); }
really_inline value::operator int64_t() && noexcept(false) { return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_int64(); }
really_inline value::operator double() && noexcept(false) { return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_double(); }
really_inline value::operator std::string_view() && noexcept(false) { return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_string(); }
really_inline value::operator raw_json_string() && noexcept(false) { return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_raw_json_string(); }
really_inline value::operator bool() && noexcept(false) { return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(*this).get_bool(); }
#endif

really_inline const uint8_t *value::consume() noexcept { consumed = true; return json->advance(); }
really_inline array value::begin() noexcept { consumed = true; return array::begin(*this, json->advance_if_start('[')); }
really_inline array value::end() noexcept { return {}; }
// TODO this CANNOT be reused. Each time you try, it will get you a new object.
really_inline simdjson_result<value> value::operator[](std::string_view key) && noexcept { return std::forward<value>(*this).get_object()[key]; }

really_inline void value::finish() noexcept {
  if (!consumed) {
    auto ch = *json->advance() & ~(1<<5); // Masking out the 6th bit turns { into [ and } into ]
    json->depth += (ch == '[') - (ch == ']'); // depth-- for [ or {, depth++ for ] or }
    if (ch == '[') { logger::log_start_event("skip unconsumed start", json, true); }
    else if (ch == ']') { logger::log_end_event("skip unconsumed end", json, true); }
    else { logger::log_event("skip unconsumed", json, true); }
  }
  while (json->depth != depth) {
    auto ch = *json->advance() & ~(1<<5); // Masking out the 6th bit turns { into [ and } into ]
    json->depth += (ch == '[') - (ch == ']'); // depth-- for [ or {, depth++ for ] or }
    if (ch == '[') { logger::log_start_event("skip", json, true); }
    else if (ch == ']') { logger::log_end_event("skip", json, true); }
    else { logger::log_event("skip", json, true); }
  }
}

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::simdjson_result(SIMDJSON_IMPLEMENTATION::stream::value &&value) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::value>(std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(value)) {}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::simdjson_result(SIMDJSON_IMPLEMENTATION::stream::value &&value, error_code error) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::value>(std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(value), error) {}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::simdjson_result(const SIMDJSON_IMPLEMENTATION::stream::value &value, error_code error) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::value>(SIMDJSON_IMPLEMENTATION::stream::value(value), error) {}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::simdjson_result(const simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value> &other) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::value>(SIMDJSON_IMPLEMENTATION::stream::value(other.first), other.second) {}

really_inline SIMDJSON_IMPLEMENTATION::stream::array simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::begin() noexcept {
  return SIMDJSON_IMPLEMENTATION::stream::array::begin(first, error());
}
really_inline SIMDJSON_IMPLEMENTATION::stream::array simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::end() noexcept {
  return {};
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator[](std::string_view key) && noexcept {
  if (error()) { return *this; }
  return std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first)[key];
}

really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::get_array() && noexcept { return error() ? simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array>(first, error()) : std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first).get_array(); }
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::get_object() && noexcept { return error() ? simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object>(first, error()) : std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first).get_object(); }
really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::get_uint64() && noexcept { return error() ? error() : std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first).get_uint64(); }
really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::get_int64() && noexcept { return error() ? error() : std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first).get_int64(); }
really_inline simdjson_result<double> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::get_double() && noexcept { return error() ? error() : std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first).get_double(); }
really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::get_string() && noexcept { return error() ? error() : std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first).get_string(); }
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::get_raw_json_string() && noexcept { return error() ? error() : std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first).get_raw_json_string(); }
really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::get_bool() && noexcept { return error() ? error() : std::forward<SIMDJSON_IMPLEMENTATION::stream::value>(first).get_bool(); }

#if SIMDJSON_EXCEPTIONS
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator SIMDJSON_IMPLEMENTATION::stream::array() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>>(*this).get_array();
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator SIMDJSON_IMPLEMENTATION::stream::object() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>>(*this).get_object();
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator uint64_t() && noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>>(*this).get_uint64();
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator int64_t() && noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>>(*this).get_int64();
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator double() && noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>>(*this).get_double();
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator std::string_view() && noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>>(*this).get_string();
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator SIMDJSON_IMPLEMENTATION::stream::raw_json_string() && noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>>(*this).get_raw_json_string();
}
really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>::operator bool() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value>>(*this).get_bool();
}
#endif

} // namespace simdjson
