cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  include(FetchContent)
  FetchContent_Declare(
    vcpkg
    GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
    GIT_TAG master
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(vcpkg)
  set(CMAKE_TOOLCHAIN_FILE
    ${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake
    CACHE FILEPATH "Vcpkg toolchain file"
  )
  set(VCPKG_ROOT_DIR ${vcpkg_SOURCE_DIR} CACHE PATH "Vcpkg Root Directory")
endif()

add_custom_target(UpdateVcpkgBaseline
  ${VCPKG_ROOT_DIR}/vcpkg x-update-baseline
)

project(nyu-util VERSION 1.0.1)

find_package(Catch2 3 CONFIG REQUIRED)
find_package(verilator CONFIG REQUIRED)

add_library(__cvr STATIC src/CovRecorder.cpp)
target_link_libraries(__cvr PRIVATE Catch2::Catch2)
target_include_directories(__cvr PRIVATE ${VERILATOR_ROOT}/include)
add_library(covrecorder INTERFACE)
target_link_libraries(covrecorder INTERFACE
  $<LINK_LIBRARY:WHOLE_ARCHIVE,$<BUILD_INTERFACE:__cvr>$<INSTALL_INTERFACE:nyu::__cvr>>
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(
  TARGETS __cvr covrecorder
  EXPORT nyuUtilTargets
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
