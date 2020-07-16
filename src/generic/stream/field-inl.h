namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

really_inline raw_json_string field::key() noexcept { return json->peek(-2)+1; }
really_inline field::field(stream::json *_json, uint32_t _depth) noexcept : value(_json, _depth) {}

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
