#ifndef NYU_TEST_UTIL_HPP
#define NYU_TEST_UTIL_HPP

#include <cstddef>

namespace nyu {

// clang-format off
template<typename T>
concept tickable = requires (T a) {
  a.clk;
  a.eval();
};

template<typename T>
concept resetable = requires(T a) {
  a.nReset;
  a.eval();
};
// clang-format on

void tick(tickable auto& dut, size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
  }
}

void reset(resetable auto& dut) {
  dut.nReset = 1;
  dut.eval();
  dut.nReset = 0;
  dut.eval();
  dut.nReset = 1;
}

} // namespace nyu

#endif // NYU_TEST_UTIL_HPP
