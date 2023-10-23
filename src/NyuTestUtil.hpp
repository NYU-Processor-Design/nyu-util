#ifndef NYU_TEST_UTIL_HPP
#define NYU_TEST_UTIL_HPP

#include <cstddef>
#include <cstdint>

#include <verilated.h>
#include <verilated_fst_c.h>

namespace nyu {

// clang-format off
template <typename T>
concept evalable = requires(T a) {
  a.eval();
};

template<typename T>
concept tickable = requires (T a) {
  a.eval();
  a.clk;
};

template<typename T>
concept resetable = requires(T a) {
  a.eval();
  a.nReset;
};

template <typename T>
concept traceable = requires(T a, VerilatedFstC fst) {
  a.trace(&fst, 1);
};

template<typename T>
concept sync_traceable = tickable<T> && resetable<T> && traceable<T>;
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

template <evalable T> void eval(T& dut, std::size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    dut.eval();
  }
}

template <traceable T> void eval(tracer<T>& tracer, std::size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    tracer.dut.eval();
    tracer.fst.dump(tracer.time++);
  }
}

void tick(tickable auto& dut, std::size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
  }
}

template <sync_traceable T>
void tick(tracer<T>& tracer, std::size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    tracer.dut.clk = 0;
    eval(tracer);
    tracer.dut.clk = 1;
    eval(tracer);
  }
}

void reset(resetable auto& dut) {
  dut.nReset = 1;
  dut.eval();
  dut.nReset = 0;
  dut.eval();
  dut.nReset = 1;
}

template <sync_traceable T> void reset(tracer<T>& tracer) {
  tracer.dut.nReset = 1;
  eval(tracer);
  tracer.dut.nReset = 0;
  eval(tracer);
  tracer.dut.nReset = 1;
}

template <typename T> T& getDUT() {
  static T* p {new T};
  return *p;
}

} // namespace nyu

#endif // NYU_TEST_UTIL_HPP
