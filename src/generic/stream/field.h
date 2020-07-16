#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

/**
 * A JSON field in an object.
 */
class field : public value {
public:
  really_inline field() noexcept = default;

  really_inline raw_json_string key() noexcept;
protected:
  really_inline field(stream::json *_json, uint32_t _depth) noexcept;
  friend class simdjson_result<stream::field>;
  friend class object;
};

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
