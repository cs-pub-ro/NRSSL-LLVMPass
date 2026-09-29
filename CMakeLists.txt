cmake_minimum_required(VERSION 3.21)
project(NRSConversionPass LANGUAGES CXX)

set(LLVM_DIR "/workspace/shared/llvm-project/build/lib/cmake/llvm" CACHE PATH
    "CMake dir of the LLVM build with the NRSSL intrinsics")
find_package(LLVM REQUIRED CONFIG)
find_package(JNI REQUIRED)
find_program(SBT sbt REQUIRED)

add_subdirectory(src)

# NRSSL (git submodule) is built with sbt. The pass loads nrssl.jar and the
# Scala runtime from $NRSSL_JARS at run time, point it to build/jars.
set(NRSSL_JARS_DIR "${CMAKE_BINARY_DIR}/jars")
file(GLOB_RECURSE _nrssl_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/nrssl/src/main/*.scala)
add_custom_command(
  OUTPUT "${NRSSL_JARS_DIR}/nrssl.jar" "${NRSSL_JARS_DIR}/scala-library-2.13.8.jar"
  COMMAND ${CMAKE_COMMAND} -DSBT=${SBT} -DSRC=${CMAKE_SOURCE_DIR}/nrssl -DOUT=${NRSSL_JARS_DIR}
          -P ${CMAKE_SOURCE_DIR}/cmake/BuildNRSSL.cmake
  DEPENDS ${_nrssl_sources} ${CMAKE_SOURCE_DIR}/nrssl/build.sbt ${CMAKE_SOURCE_DIR}/cmake/BuildNRSSL.cmake
  COMMENT "Building NRSSL with sbt"
  VERBATIM)
add_custom_target(nrssl_jars ALL DEPENDS "${NRSSL_JARS_DIR}/nrssl.jar")
