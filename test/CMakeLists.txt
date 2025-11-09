find_package(nyu-cmake CONFIG REQUIRED)

add_executable(tests)

target_sources(tests
  PRIVATE
    Module.cpp
)
target_link_libraries(tests
  PRIVATE
    Catch2::Catch2WithMain
    testutil
)
target_compile_features(tests PRIVATE cxx_std_23)

nyu_add_sv(tests Module.sv)
nyu_target_verilate(tests
  TOP_MODULES Module
  ARGS
    ${_traces_option}
)
