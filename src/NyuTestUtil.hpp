#ifndef NYU_TEST_UTIL_HPP
#define NYU_TEST_UTIL_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <verilated.h>
#include <verilated_fst_c.h>

namespace nyu {

// Macros

#define NYU_META_ASSIGNABLE_CONCEPTS(NAME, MEMBER)                             \
  template <class T, class R>                                                  \
  concept NAME = requires(T t, R r) { t.MEMBER = std::forward<R>(r); };        \
                                                                               \
  template <class T, class R>                                                  \
  concept nothrow_##NAME = requires(T t, R r) {                                \
    { t.MEMBER = std::forward<R>(r) } noexcept;                                \
  };

#define NYU_META_MEMBER_CALLABLE_CONCEPTS(NAME, METHOD)                        \
  template <class T, class... Args>                                            \
  concept NAME =                                                               \
      requires(T t, Args... args) { t.METHOD(std::forward<Args>(args)...); };  \
                                                                               \
  template <class T, class... Args>                                            \
  concept nothrow_##NAME = requires(T t, Args... args) {                       \
    { t.METHOD(std::forward<Args>(args)...) } noexcept;                        \
  };

#define NYU_META_CALLABLE_CONCEPTS(NAME, FUNCTION)                             \
  template <class T, class... Args>                                            \
  concept NAME = requires(T t, Args... args) {                                 \
    FUNCTION(std::forward<T>(t), std::forward<Args>(args)...);                 \
  };                                                                           \
                                                                               \
  template <class T, class... Args>                                            \
  concept nothrow_##NAME = requires(T t, Args... args) {                       \
    { FUNCTION(std::forward<T>(t), std::forward<Args>(args)...) } noexcept;    \
  };


// Concepts

NYU_META_CALLABLE_CONCEPTS(tag_invocable, tag_invoke)
NYU_META_MEMBER_CALLABLE_CONCEPTS(has_eval, eval)
NYU_META_ASSIGNABLE_CONCEPTS(has_clk_assign_from, clk)
NYU_META_ASSIGNABLE_CONCEPTS(has_nreset_assign_from, nReset)

template <typename T>
concept is_trace_capable = requires { T::traceCapable; } && T::traceCapable;

template <typename T>
concept can_trace = is_trace_capable<T> && requires(T t) {
  t.trace(static_cast<VerilatedTraceBaseC*>(nullptr), 0, 0);
};

template <typename T>
concept nothrow_can_trace = is_trace_capable<T> && requires(T t) {
  { t.trace(static_cast<VerilatedTraceBaseC*>(nullptr), 0, 0) } noexcept;
};


// Eval

struct eval_default_t {
  template <typename Dut>
  requires has_eval<Dut&&>
  constexpr void operator()(Dut&& dut, std::size_t cycles = 1) const
      noexcept(nothrow_has_eval<Dut&&>) {
    for(std::size_t i = 0; i < cycles; ++i) {
      dut.eval();
    }
  }
};

inline constexpr eval_default_t eval_default {};

struct eval_t {
private:
  template <typename Dut>
  static constexpr bool is_nothrow = tag_invocable<eval_t, Dut, std::size_t>
      ? nothrow_tag_invocable<eval_t, Dut, std::size_t>
      : nothrow_has_eval<Dut>;

public:
  template <typename Dut>
  constexpr decltype(auto) operator()(Dut&& dut, std::size_t cycles = 1) const
      noexcept(is_nothrow<Dut&&>) {
    if constexpr(tag_invocable<eval_t, Dut&&, std::size_t>) {
      return tag_invoke(*this, std::forward<Dut>(dut), cycles);
    } else {
      return ::nyu::eval_default(std::forward<Dut>(dut), cycles);
    }
  }
};

namespace cpo {
inline constexpr eval_t eval {};
}
using cpo::eval;

NYU_META_CALLABLE_CONCEPTS(can_call_eval, ::nyu::eval)


// Tick

template <typename T, typename R = int>
concept tick_default_ok = can_call_eval<T> && has_clk_assign_from<T, R>;

template <typename T, typename R = int>
concept nothrow_tick_default_ok =
    nothrow_can_call_eval<T> && nothrow_has_clk_assign_from<T, R>;

struct tick_default_t {
  template <typename Dut, typename R = int>
  requires tick_default_ok<Dut&&, R>
  constexpr void operator()(Dut&& dut, std::size_t cycles = 1) const
      noexcept(nothrow_tick_default_ok<Dut&&, R>) {

    // The default tick protocol is low - high - high - low
    // There are several advantages to this:
    // * Detection of level-triggered elements in the design
    // * Combinational logic always resolves before edge-triggered
    //   sequential logic
    // * Input changes don't appear on clock edges in traces

    for(std::size_t i = 0; i < cycles; ++i) {
      dut.clk = R {0};
      ::nyu::eval(dut);
      dut.clk = R {1};
      ::nyu::eval(dut, 2);
      dut.clk = R {0};
      ::nyu::eval(dut);
    }
  }
};

inline constexpr tick_default_t tick_default {};

struct tick_t {
private:
  template <typename Dut>
  static constexpr bool is_nothrow = tag_invocable<tick_t, Dut, std::size_t>
      ? nothrow_tag_invocable<tick_t, Dut, std::size_t>
      : nothrow_tick_default_ok<Dut>;

public:
  template <typename Dut, typename... Args>
  constexpr decltype(auto) operator()(Dut&& dut, std::size_t cycles = 1,
      Args&&... args) const noexcept(is_nothrow<Dut&&>) {
    if constexpr(tag_invocable<tick_t, Dut&&, std::size_t, Args&&...>) {
      return tag_invoke(*this, std::forward<Dut>(dut), cycles,
          std::forward<Args>(args)...);
    } else {
      static_assert(sizeof...(Args) == 0,
          "nyu::tick default does not accept extra parameters; provide a "
          "tag_invoke(nyu::tick_t, Dut&&, cycles, ...) overload.");
      return ::nyu::tick_default(std::forward<Dut>(dut), cycles);
    }
  }
};

namespace cpo {
inline constexpr tick_t tick {};
}
using cpo::tick;

NYU_META_CALLABLE_CONCEPTS(can_call_tick, ::nyu::tick)


// Reset

template <typename T, typename R = int>
concept reset_default_ok =
    has_nreset_assign_from<T, R> && (can_call_tick<T> || can_call_eval<T>);

template <typename T, typename R = int>
concept nothrow_reset_default_ok = nothrow_has_nreset_assign_from<T, R> &&
    (can_call_tick<T> ? nothrow_can_call_tick<T> : nothrow_can_call_eval<T>);

struct reset_default_t {
  template <typename Dut, typename R = int>
  requires(reset_default_ok<Dut &&, R>)
  constexpr void operator()(Dut&& dut) const
      noexcept(nothrow_reset_default_ok<Dut&&, R>) {

    // The default reset protocol uses tick if available, otherwise eval.
    // For clocked elements tick is preferable because:
    // * It verifies the DUT is stable under clock input during reset
    // * It maintains a consistent clock signal in traces
    //
    // Reset is left high without an additional tick to prevent starting
    // DUT operation prior to input configuration. Eval reset grabs an extra
    // cycle to verify level-triggered elements remain constant.

    dut.nReset = R {0};
    if constexpr(can_call_tick<Dut&&>)
      ::nyu::tick(dut);
    else
      ::nyu::eval(dut, 2);
    dut.nReset = R {1};
  }
};

inline constexpr reset_default_t reset_default {};

struct reset_t {
private:
  template <typename Dut, typename... Args>
  static constexpr bool is_nothrow = tag_invocable<reset_t, Dut, Args...>
      ? nothrow_tag_invocable<reset_t, Dut, Args...>
      : nothrow_reset_default_ok<Dut>;

public:
  template <typename Dut, typename... Args>
  constexpr decltype(auto) operator()(Dut&& dut, Args&&... args) const
      noexcept(is_nothrow<Dut&&, Args&&...>) {
    if constexpr(tag_invocable<reset_t, Dut&&, Args&&...>) {
      return tag_invoke(*this, std::forward<Dut>(dut),
          std::forward<Args>(args)...);
    } else {
      static_assert(sizeof...(Args) == 0,
          "nyu::reset default does not accept extra parameters; provide a "
          "tag_invoke(nyu::reset_t, Dut&&, ...) overload.");
      return ::nyu::reset_default(std::forward<Dut>(dut));
    }
  }
};

namespace cpo {
inline constexpr reset_t reset {};
}
using cpo::reset;

NYU_META_CALLABLE_CONCEPTS(can_call_reset, ::nyu::reset)


// Tracer

template <typename T>
struct tracer : T {
  using T::T;

  void eval() {
    T::eval();
    if(mFst)
      mFst->dump(mTime++);
  }

  std::unique_ptr<VerilatedFstC> mFst {};
  std::uint64_t mTime {0};
};


// Get Test Name

struct default_test_name_token_t {};
inline constexpr default_test_name_token_t default_test_name_token;

struct get_test_name_t {
  template <typename Token>
  static constexpr bool is_nothrow = tag_invocable<get_test_name_t, Token>
      ? nothrow_tag_invocable<get_test_name_t, Token>
      : true;

  template <typename Token = default_test_name_token_t>
  constexpr std::string operator()(Token) const noexcept(is_nothrow<Token>) {
    if constexpr(tag_invocable<get_test_name_t, Token>) {
      return tag_invoke(*this, Token {});
    } else {
      return "test";
    }
  }
};

namespace cpo {
inline constexpr get_test_name_t get_test_name {};
}
using cpo::get_test_name;


// Get DUT

struct dut_options {
  std::optional<std::string> trace_file;
  int trace_levels = 99;
  bool enable_trace = true;
  std::string trace_ext = ".fst";
};

struct get_dut_t {
private:
  template <typename Dut>
  static constexpr bool nothrow_get_dut_default =
      std::is_nothrow_default_constructible_v<Dut> && !can_trace<Dut>;

  template <typename Dut, typename NameToken>
  static constexpr bool is_nothrow =
      tag_invocable<get_dut_t, dut_options, NameToken, std::type_identity<Dut>>
      ? nothrow_tag_invocable<get_dut_t, dut_options, NameToken,
            std::type_identity<Dut>>
      : nothrow_get_dut_default<Dut>;

  static std::string sanitize_filename(std::string_view name,
      std::string_view ext) {
    static constexpr std::string_view unsafe {R"(/<>:"\|?*)"};
    std::string fileName(name.size(), '\0');

    std::transform(name.begin(), name.end(), fileName.begin(), [&](char c) {
      return unsafe.find(c) == std::string_view::npos ? c : '_';
    });

    fileName += ext;
    return fileName;
  }

  template <typename NameToken>
  static std::string make_trace_filename(const dut_options& opts) {
    if(opts.trace_file && !opts.trace_file->empty())
      return *opts.trace_file;
    return sanitize_filename(nyu::get_test_name(NameToken {}), opts.trace_ext);
  }

  template <typename Dut>
  static Dut& plain_instance() noexcept(
      std::is_nothrow_default_constructible_v<Dut>) {
    static Dut dut;
    return dut;
  }

  template <typename Dut>
  static tracer<Dut>& disabled_traced_instance() noexcept(
      std::is_nothrow_default_constructible_v<Dut>) {
    static tracer<Dut> dut;
    dut.mFst.reset();
    dut.mTime = 0;
    return dut;
  }

  template <typename Dut>
  static tracer<Dut>& enabled_traced_instance(std::string_view file,
      int trace_levels) {
    auto& dut {disabled_traced_instance<Dut>()};
    Verilated::traceEverOn(true);
    dut.mFst = std::make_unique<VerilatedFstC>();
    dut.trace(dut.mFst.get(), trace_levels, 0);
    dut.mFst->open(std::string(file).c_str());
    return dut;
  }

  template <typename Dut, typename NameToken>
  requires(nyu::can_call_eval<Dut>)
  static decltype(auto) get_dut_default(const dut_options& opts) {
    if constexpr(can_trace<Dut>) {
      if(opts.enable_trace) {
        auto file = make_trace_filename<NameToken>(opts);
        return enabled_traced_instance<Dut>(file, opts.trace_levels);
      }
      return disabled_traced_instance<Dut>();
    } else {
      return plain_instance<Dut>();
    }
  }

public:
  template <typename Dut, typename NameToken = default_test_name_token_t>
  decltype(auto) operator()(const dut_options& opts = {}, NameToken = {},
      std::type_identity<Dut> = {}) const noexcept(is_nothrow<Dut, NameToken>) {
    if constexpr(tag_invocable<get_dut_t, dut_options, NameToken,
                     std::type_identity<Dut>>) {
      return tag_invoke(*this, opts, NameToken {}, std::type_identity<Dut> {});
    } else {
      return get_dut_default<Dut, NameToken>(opts);
    }
  }
};

namespace cpo {
inline constexpr get_dut_t get_dut {};
}

template <typename Dut, typename NameToken = default_test_name_token_t>
decltype(auto) get_dut(const dut_options& opts = {}, NameToken = {}) {
  return ::nyu::cpo::get_dut(opts, NameToken {}, std::type_identity<Dut> {});
}

} // namespace nyu

#endif // NYU_TEST_UTIL_HPP
