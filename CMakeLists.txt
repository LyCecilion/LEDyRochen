cmake_minimum_required(VERSION 3.25)

project(
  LEDyRochen
  VERSION 0.1.0
  DESCRIPTION "CH546 11x44 display writer"
  LANGUAGES CXX
)

include(FetchContent)
FetchContent_Declare(
  stb
  URL https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h
  DOWNLOAD_NO_EXTRACT TRUE
  DOWNLOAD_NAME stb_truetype.h
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
  cli11
  URL https://github.com/CLIUtils/CLI11/releases/download/v2.6.2/CLI11.hpp
  DOWNLOAD_NO_EXTRACT TRUE
  DOWNLOAD_NAME CLI11.hpp
)
FetchContent_MakeAvailable(cli11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

add_library(
  ledyrochen_core
  STATIC
  src/utils.cpp
  src/pack.cpp
  src/protocol.cpp
  src/device.cpp
  src/vendor_font.cpp
  src/truetype.cpp
)
target_link_libraries(ledyrochen_core PUBLIC PkgConfig::LIBUSB)
target_include_directories(ledyrochen_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(ledyrochen_core SYSTEM PRIVATE ${stb_SOURCE_DIR})
target_compile_features(ledyrochen_core PUBLIC cxx_std_20)
set_target_properties(ledyrochen_core PROPERTIES CXX_EXTENSIONS OFF)

add_executable(ledyrochen src/main.cpp)
target_link_libraries(ledyrochen PRIVATE ledyrochen_core)
target_include_directories(ledyrochen SYSTEM PRIVATE ${cli11_SOURCE_DIR})
set_target_properties(ledyrochen PROPERTIES CXX_EXTENSIONS OFF)

function(ledyrochen_enable_warnings target)
  target_compile_options(
    ${target}
    PRIVATE
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wall>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wextra>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wshadow>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wconversion>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wno-sign-conversion>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wpedantic>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wformat=2>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wnull-dereference>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wdouble-promotion>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wimplicit-fallthrough>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wvla>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wnon-virtual-dtor>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Woverloaded-virtual>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wold-style-cast>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wcast-qual>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wfloat-equal>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wundef>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wmissing-declarations>
      $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wuseless-cast>
      $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wduplicated-cond>
      $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wduplicated-branches>
      $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wlogical-op>
      $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wstrict-aliasing=3>
  )
endfunction()

ledyrochen_enable_warnings(ledyrochen_core)
ledyrochen_enable_warnings(ledyrochen)

include(CTest)
if(BUILD_TESTING)
  add_executable(ledyrochen_tests tests/tests.cpp)
  target_link_libraries(ledyrochen_tests PRIVATE ledyrochen_core)
  set_target_properties(ledyrochen_tests PROPERTIES CXX_EXTENSIONS OFF)
  ledyrochen_enable_warnings(ledyrochen_tests)
  add_test(NAME ledyrochen_tests COMMAND ledyrochen_tests)
endif()

option(LEDYROCHEN_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(LEDYROCHEN_WARNINGS_AS_ERRORS)
  target_compile_options(
    ledyrochen_core PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Werror>
  )
  target_compile_options(
    ledyrochen PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Werror>
  )
  if(BUILD_TESTING)
    target_compile_options(
      ledyrochen_tests PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Werror>
    )
  endif()
endif()

include(GNUInstallDirs)
install(TARGETS ledyrochen RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES udev/70-ledyrochen.rules DESTINATION lib/udev/rules.d)
