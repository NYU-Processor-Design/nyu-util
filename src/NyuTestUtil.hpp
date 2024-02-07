#ifndef NYU_TEST_UTIL_HPP
#define NYU_TEST_UTIL_HPP

#include <cstddef>
#include <cstdint>

#include <catch2/interfaces/catch_interfaces_capture.hpp>

#include <verilated.h>
#include <verilated_fst_c.h>

namespace nyu {

template <typename T>
concept evalable = requires(T a) { a.eval(); };

template <typename T>
concept tickable = requires(T a) {
  a.eval();
  a.clk = 0;
};

template <typename T>
concept resetable = requires(T a) {
  a.eval();
  a.nReset = 0;
};

template <typename T>
concept traceable = requires(T a, VerilatedFstC fst) { a.trace(&fst, 1); };

template <typename T>
concept not_traceable = evalable<T> && !traceable<T>;

template <traceable T> struct tracer : T {
  void eval() {
    T::eval();
    _fst->dump(_time++);
  }

  ~tracer() {
    delete _fst;
  }

  VerilatedFstC* _fst {nullptr};
  std::uint64_t _time {0};
};

constexpr inline auto eval = [](evalable auto& dut, std::size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    dut.eval();
  }
};

constexpr inline auto tick = [](tickable auto& dut, std::size_t cycles = 1) {
  for(size_t i {0}; i < cycles; ++i) {
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
  }
};

constexpr inline auto reset = [](resetable auto& dut) {
  dut.nReset = 1;
  dut.eval();
  dut.nReset = 0;
  dut.eval();
  dut.nReset = 1;
};

template <traceable T>
tracer<T>& getDUT(std::string fileName = {}, int levels = 99) {
  static tracer<T> dut;

  if(fileName.empty()) {
    std::string testName {Catch::getResultCapture().getCurrentTestName()};
    std::string unsafe {"/<>:\"\\|?*"};

    fileName.resize(testName.size());
    std::transform(testName.begin(), testName.end(), fileName.begin(),
        [&](char c) { return unsafe.find(c) == std::string::npos ? c : '_'; });
    fileName += ".fst";
  }

  Verilated::traceEverOn(true);

  delete dut._fst;
  dut._time = 0;
  dut._fst = new VerilatedFstC;
  dut.trace(dut._fst, levels);
  dut._fst->open(fileName.c_str());

  return dut;
}

template <not_traceable T> T& getDUT(std::string = {}, int = 0) {
  static T dut;
  return dut;
}

} // namespace nyu

#endif // NYU_TEST_UTIL_HPP
