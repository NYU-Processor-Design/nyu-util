#ifndef NYU_TEST_UTIL_HPP
#define NYU_TEST_UTIL_HPP

#include <cstddef>
#include <cstdint>

#include <verilated.h>
#include <verilated_fst_c.h>

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

template<typename T>
concept traceable =
    tickable<T> &&
    resetable<T> &&
    requires(T a, VerilatedFstC fst) {
  a.trace(&fst, 1);
};
// clang-format on

template <traceable T> struct tracer {

  tracer(T& dut, const char* file, int levels = 99) : dut {dut} {
    Verilated::traceEverOn(true);
    dut.trace(&fst, levels);
    fst.open(file);
  }

  T& dut;
  VerilatedFstC fst;
  std::uint64_t time {0};
};

void tick(tickable auto& dut, std::size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
  }
}

template <traceable T> void tick(tracer<T>& tracer, std::size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    tracer.dut.clk = 0;
    tracer.dut.eval();
    tracer.fst.dump(tracer.time++);
    tracer.dut.clk = 1;
    tracer.dut.eval();
    tracer.fst.dump(tracer.time++);
  }
}

void reset(resetable auto& dut) {
  dut.nReset = 1;
  dut.eval();
  dut.nReset = 0;
  dut.eval();
  dut.nReset = 1;
}

template <traceable T> void reset(tracer<T>& tracer) {
  tracer.dut.nReset = 1;
  tracer.dut.eval();
  tracer.fst.dump(tracer.time++);
  tracer.dut.nReset = 0;
  tracer.dut.eval();
  tracer.fst.dump(tracer.time++);
  tracer.dut.nReset = 1;
}

} // namespace nyu

#endif // NYU_TEST_UTIL_HPP
