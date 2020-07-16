#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stream {

class array;
class object;
class value;
class raw_json_string;

/**
 * A JSON fragment iterator.
 *
 * This holds the actual iterator as well as the buffer for writing strings.
 */
class json {
public:
  really_inline json() noexcept = default;
  really_inline json(json &&other) noexcept;
  really_inline json(const json &other) = delete;
  really_inline json &operator=(const json &other) = delete;
  really_inline stream::value && as_value() noexcept;

  really_inline simdjson_result<array> get_array() noexcept;
  really_inline simdjson_result<object> get_object() noexcept;
  really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  really_inline simdjson_result<int64_t> get_int64() noexcept;
  really_inline simdjson_result<double> get_double() noexcept;
  really_inline simdjson_result<std::string_view> get_string() noexcept;
  really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  really_inline simdjson_result<bool> get_bool() noexcept;

#if SIMDJSON_EXCEPTIONS
  really_inline operator array() noexcept(false);
  really_inline operator object() noexcept(false);
  really_inline operator uint64_t() noexcept(false);
  really_inline operator int64_t() noexcept(false);
  really_inline operator double() noexcept(false);
  really_inline operator std::string_view() noexcept(false);
  really_inline operator raw_json_string() noexcept(false);
  really_inline operator bool() noexcept(false);
#endif

  really_inline array begin() noexcept;
  really_inline array end() noexcept;
  really_inline simdjson_result<stream::value&> operator[](std::string_view key) noexcept;

// protected:
  really_inline json(const uint32_t *_index, const uint8_t *_buf, uint8_t *_string_buf, uint32_t _depth=0) noexcept;
  const uint32_t *index; //< Current position
  const uint8_t * buf; //< Buffer
  uint8_t *string_buf; //< String buffer
  uint32_t depth; //< Current depth
  stream::value value; //< Points to this; used internally only, to return value& when needed

  //
  // Token methods
  //
  really_inline const uint8_t *advance() noexcept;
  really_inline const uint8_t *peek(int n) const noexcept;
  really_inline bool advance_if_start(uint8_t structural) noexcept;
  really_inline bool advance_if_end(uint8_t structural) noexcept;
  really_inline bool advance_if(uint8_t structural) noexcept;
  really_inline bool advance_if(uint8_t structural, uint8_t structural2) noexcept;
  really_inline bool advance_if(uint8_t structural, uint8_t structural2, uint8_t structural3) noexcept;

  friend class simdjson_result<stream::json>;
  friend class dom::parser;
  // friend struct simdjson::SIMDJSON_IMPLEMENTATION::stage2::structural_parser;
  friend class stream::value;
  friend class object;
  friend class array;
  friend class field;
  template<typename T>
  friend void logger::log_event(const char *event_prefix, const char *event, T &json, const char *detail, bool prev);
};

} // namespace stream
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::stream::json> : public internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::stream::json> {
public:
  really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::stream::json &&value) noexcept; ///< @private
  really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::stream::json &&value, error_code error) noexcept; ///< @private

  really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value&> as_value() noexcept;
  really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::array> get_array() && noexcept;
  really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::object> get_object() && noexcept;
  really_inline simdjson_result<uint64_t> get_uint64() && noexcept;
  really_inline simdjson_result<int64_t> get_int64() && noexcept;
  really_inline simdjson_result<double> get_double() && noexcept;
  really_inline simdjson_result<std::string_view> get_string() && noexcept;
  really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::raw_json_string> get_raw_json_string() && noexcept;
  really_inline simdjson_result<bool> get_bool() && noexcept;

#if SIMDJSON_EXCEPTIONS
  really_inline operator SIMDJSON_IMPLEMENTATION::stream::array() && noexcept(false);
  really_inline operator SIMDJSON_IMPLEMENTATION::stream::object() && noexcept(false);
  really_inline operator uint64_t() && noexcept(false);
  really_inline operator int64_t() && noexcept(false);
  really_inline operator double() && noexcept(false);
  really_inline operator std::string_view() && noexcept(false);
  really_inline operator SIMDJSON_IMPLEMENTATION::stream::raw_json_string() && noexcept(false);
  really_inline operator bool() && noexcept(false);
#endif

  really_inline SIMDJSON_IMPLEMENTATION::stream::array begin() noexcept;
  really_inline SIMDJSON_IMPLEMENTATION::stream::array end() noexcept;
  really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::stream::value&> operator[](std::string_view key) && noexcept;
};

} // namespace simdjson
