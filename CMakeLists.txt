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

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

add_executable(ledyrochen src/main.cpp)
target_link_libraries(ledyrochen PRIVATE ${LIBUSB_LIBRARIES})
target_include_directories(ledyrochen PRIVATE ${LIBUSB_INCLUDE_DIRS})
target_include_directories(ledyrochen PRIVATE ${stb_SOURCE_DIR})

target_compile_features(ledyrochen PRIVATE cxx_std_20)
set_target_properties(ledyrochen PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(
  ledyrochen
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

option(LEDYROCHEN_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(LEDYROCHEN_WARNINGS_AS_ERRORS)
  target_compile_options(
    ledyrochen PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Werror>
  )
endif()

include(GNUInstallDirs)
install(TARGETS ledyrochen RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
