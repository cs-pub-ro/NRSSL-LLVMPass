cmake_minimum_required(VERSION 3.28)

if (NOT CMAKE_HOST_LINUX)
  message(FATAL_ERROR "This project is only meant for Linux")
endif()

project(NRSConversionPass LANGUAGES CXX)

if (NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
  message(FATAL_ERROR "This project requires Clang or GCC; the compiler is ${CMAKE_CXX_COMPILER_ID}")
endif()

option(NRS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors (on in the presets)" OFF)
option(NRS_ENABLE_CLANG_TIDY "Run clang-tidy on the project sources while building" OFF)
option(NRS_ENABLE_UBSAN "Build the unit tests with the undefined behavior sanitizer" OFF)
option(NRS_BUILD_TESTS "Build the lit and unit tests" ON)

set(LLVM_DIR "/workspace/shared/llvm-project/build/nrs/lib/cmake/llvm" CACHE PATH
    "CMake dir of the LLVM build with the NRSSL intrinsics")
find_package(LLVM REQUIRED CONFIG)
find_package(JNI REQUIRED)
find_program(SBT sbt REQUIRED)

include(cmake/ProjectOptions.cmake)

# NRSSL (git submodule) is built with sbt into build/jars. The pass finds the
# jars there, $NRSSL_JARS overrides that at run time.
set(NRSSL_JARS_DIR "${CMAKE_BINARY_DIR}/jars")

add_subdirectory(src)

add_custom_command(
  OUTPUT "${NRSSL_JARS_DIR}/nrssl.jar" "${NRSSL_JARS_DIR}/scala-library-2.13.8.jar"
  COMMAND ${CMAKE_COMMAND} -DSBT=${SBT} -DSRC=${CMAKE_SOURCE_DIR}/nrssl -DOUT=${NRSSL_JARS_DIR}
          -P ${CMAKE_SOURCE_DIR}/cmake/BuildNRSSL.cmake
  COMMENT "Building NRSSL with sbt"
  VERBATIM)
add_custom_target(nrssl_jars ALL DEPENDS "${NRSSL_JARS_DIR}/nrssl.jar")

if(NRS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
  add_subdirectory(unittests)
endif()
