#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

/**
 * A forward-only JSON array.
 */
class array {
public:
  really_inline array() noexcept;

  //
  // Iterator interface
  //
  really_inline simdjson_result<stream::value> operator*() noexcept;
  really_inline bool operator!=(array &other) noexcept;
  really_inline array &operator++() noexcept;
  really_inline array begin() noexcept;
  really_inline array end() noexcept;

protected:
  really_inline array(stream::value &parent, error_code _error=SUCCESS) noexcept;

  really_inline void advance() noexcept;
  really_inline static simdjson_result<array> try_begin(stream::value &parent) noexcept;
  really_inline static array begin(stream::value &parent) noexcept;

  stream::value value;
  error_code error;

  friend class stream::value;
  friend struct simdjson_result<stream::value>;
  friend struct simdjson_result<stream::array>;
};

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array> : public internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::array> {
public:
  really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::stream::array &&value) noexcept; ///< @private
  really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::stream::value &parent, error_code error) noexcept; ///< @private

  really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value> operator[](std::string_view key) noexcept;

  really_inline SIMDJSON_IMPLEMENTATION::stream::array begin() noexcept;
  really_inline SIMDJSON_IMPLEMENTATION::stream::array end() noexcept;
};

} // namespace simdjson
