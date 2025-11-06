cmake_minimum_required(VERSION 3.27)

include(cmake/BootstrapVcpkg.cmake)

project(nyu-util VERSION 2.0.5)

find_package(Catch2 3 CONFIG REQUIRED)
find_package(verilator CONFIG REQUIRED)

add_library(__cvr STATIC src/CovRecorder.cpp)
target_link_libraries(__cvr PUBLIC Catch2::Catch2)
target_include_directories(__cvr PRIVATE ${VERILATOR_ROOT}/include)
add_library(covrecorder INTERFACE)
target_link_libraries(covrecorder INTERFACE
  $<LINK_LIBRARY:WHOLE_ARCHIVE,$<BUILD_INTERFACE:__cvr>$<INSTALL_INTERFACE:nyu::__cvr>>
)

add_library(testutil INTERFACE)
target_link_libraries(testutil INTERFACE Catch2::Catch2)
target_include_directories(testutil INTERFACE ${VERILATOR_ROOT}/include)
target_sources(testutil INTERFACE
  FILE_SET HEADERS
  BASE_DIRS src
  FILES
    src/NyuTestUtil.hpp
    src/NyuCatch2TestUtil.hpp
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(
  TARGETS __cvr covrecorder testutil
  EXPORT nyuUtilTargets
  FILE_SET HEADERS
)

install(
  EXPORT nyuUtilTargets
  NAMESPACE nyu::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/nyu-util
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/nyu-util-config-version.cmake
  COMPATIBILITY ExactVersion
  ARCH_INDEPENDENT
)

install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/nyu-util-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/nyu-util-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/nyu-util
)
