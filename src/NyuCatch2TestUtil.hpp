#ifndef NYU_CATCH2_TEST_UTIL
#define NYU_CATCH2_TEST_UTIL

#include <NyuTestUtil.hpp>

#include <catch2/interfaces/catch_interfaces_capture.hpp>

namespace nyu {
struct catch2_test_name_token_t {};
inline constexpr catch2_test_name_token_t catch2_test_name_token;

inline std::string tag_invoke(::nyu::get_test_name_t,
    ::nyu::catch2_test_name_token_t) {
  return Catch::getResultCapture().getCurrentTestName();
}

template <typename Dut> decltype(auto) get_dut_catch2(dut_options opts = {}) {
  return ::nyu::get_dut<Dut>(std::move(opts), ::nyu::catch2_test_name_token);
}

} // namespace nyu

#endif // NYU_CATCH2_TEST_UTIL
