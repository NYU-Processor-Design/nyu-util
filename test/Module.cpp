#include <catch2/catch_test_macros.hpp>
#include <NyuCatch2TestUtil.hpp>

#include <VModule.h>


TEST_CASE("Test") {
  auto& dut = nyu::get_dut_catch2<VModule>();
  nyu::reset(dut);
  nyu::tick(dut);
  nyu::tick(dut, 5);
  nyu::eval(dut);
  nyu::eval(dut, 5);
}
