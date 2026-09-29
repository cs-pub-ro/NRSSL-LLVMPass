# cmake -DSBT=<sbt> -DSRC=<nrssl checkout> -DOUT=<dir> -P BuildNRSSL.cmake
#
# Packages NRSSL and copies the jar and the Scala library it needs into OUT,
# under the names the pass expects (nrssl.jar, scala-library-2.13.8.jar).

execute_process(
  COMMAND ${SBT} -batch package "export runtime:dependencyClasspath"
  WORKING_DIRECTORY ${SRC}
  OUTPUT_VARIABLE out
  RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "sbt failed:\n${out}")
endif()

file(GLOB jar "${SRC}/target/scala-2.13/sl_2.13-*.jar")
string(REGEX MATCH "[^:\n]*scala-library-2\\.13\\.8\\.jar" scala_library "${out}")
if(NOT jar OR NOT scala_library)
  message(FATAL_ERROR "can't find the NRSSL jar or scala-library in the sbt output:\n${out}")
endif()

file(MAKE_DIRECTORY ${OUT})
file(COPY_FILE ${jar} ${OUT}/nrssl.jar)
file(COPY_FILE ${scala_library} ${OUT}/scala-library-2.13.8.jar)
