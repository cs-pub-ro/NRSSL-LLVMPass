# cmake -DSBT=<sbt>
#       -DSRC=<nrssl checkout>
#       -DNRSSL_JAR=<jar name>
#       -DSCALA_LIB_JAR=<scala library name>
#       -DOUT=<dir>
#       -P BuildNRSSL.cmake
#
# Packages NRSSL and copies the jar and the Scala library it needs into OUT,
# under the names the pass expects, given by NRSSL_JAR and SCALA_LIB_JAR.

execute_process(
  COMMAND ${SBT} -batch package "export runtime:dependencyClasspath"
  WORKING_DIRECTORY ${SRC}
  OUTPUT_VARIABLE out
  RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "sbt failed:\n${out}")
endif()

file(GLOB jar "${SRC}/target/scala-*/sl_*.jar")
string(REGEX MATCH "[^:\n]*scala-library-[^:\n]*" scala_library "${out}")
if(NOT jar OR NOT scala_library)
  message(FATAL_ERROR "can't find the NRSSL jar or scala-library in the sbt output:\n${out}")
endif()

file(MAKE_DIRECTORY ${OUT})
file(COPY_FILE ${jar} ${OUT}/${NRSSL_JAR})
file(COPY_FILE ${scala_library} ${OUT}/${SCALA_LIB_JAR})