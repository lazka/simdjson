namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

really_inline raw_json_string field::key() noexcept { return _key; }
really_inline field::field(const uint8_t *key_string, stream::json *_json, uint32_t _depth) noexcept : value(_json, _depth), _key{key_string+1} {}

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
