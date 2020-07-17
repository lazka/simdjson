namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

really_inline raw_json_string::raw_json_string() noexcept : buf{nullptr} {} // for constructing a simdjson_result
really_inline raw_json_string::raw_json_string(const uint8_t * _buf) noexcept : buf{_buf} {}
really_inline raw_json_string::raw_json_string(const raw_json_string &other) noexcept : buf{other.buf} {}
really_inline raw_json_string &raw_json_string::operator=(const raw_json_string &other) noexcept { buf = other.buf; return *this; }
really_inline const char * raw_json_string::raw() const noexcept { return (const char *)buf; }
really_inline WARN_UNUSED simdjson_result<std::string_view> raw_json_string::unescape(uint8_t *&dst) const noexcept {
  uint8_t *end = stage2::stringparsing::parse_string(buf, dst);
  if (!end) { return STRING_ERROR; }
  std::string_view result((const char *)dst, end-dst);
  dst = end;
  return result;
}

really_inline bool operator==(const raw_json_string &a, std::string_view b) noexcept {
  return !strncmp(a.raw(), b.data(), b.size());
}

really_inline bool operator==(std::string_view a, const raw_json_string &b) noexcept {
  return b == a;
}

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
