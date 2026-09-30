cmake_minimum_required(VERSION 3.21)
project(NRSConversionPass LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NRS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(NRS_ENABLE_CLANG_TIDY "Run clang-tidy on the project sources while building" OFF)
option(NRS_ENABLE_UBSAN "Build the unit tests with the undefined behavior sanitizer" OFF)
option(NRS_BUILD_TESTS "Build the lit and unit tests" ON)

set(LLVM_DIR "/workspace/shared/llvm-project/build/lib/cmake/llvm" CACHE PATH
    "CMake dir of the LLVM build with the NRSSL intrinsics")
find_package(LLVM REQUIRED CONFIG)
find_package(JNI REQUIRED)
find_program(SBT sbt REQUIRED)

include(cmake/ProjectOptions.cmake)

# NRSSL (git submodule) is built with sbt into build/jars. The pass finds the
# jars there, $NRSSL_JARS overrides that at run time.
set(NRSSL_JARS_DIR "${CMAKE_BINARY_DIR}/jars")

add_subdirectory(src)

file(GLOB_RECURSE _nrssl_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/nrssl/src/main/*.scala)
add_custom_command(
  OUTPUT "${NRSSL_JARS_DIR}/nrssl.jar" "${NRSSL_JARS_DIR}/scala-library-2.13.8.jar"
  COMMAND ${CMAKE_COMMAND} -DSBT=${SBT} -DSRC=${CMAKE_SOURCE_DIR}/nrssl -DOUT=${NRSSL_JARS_DIR}
          -P ${CMAKE_SOURCE_DIR}/cmake/BuildNRSSL.cmake
  DEPENDS ${_nrssl_sources} ${CMAKE_SOURCE_DIR}/nrssl/build.sbt ${CMAKE_SOURCE_DIR}/cmake/BuildNRSSL.cmake
  COMMENT "Building NRSSL with sbt"
  VERBATIM)
add_custom_target(nrssl_jars ALL DEPENDS "${NRSSL_JARS_DIR}/nrssl.jar")

if(NRS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
  add_subdirectory(unittests)
endif()
