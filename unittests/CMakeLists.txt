# Catch2 unit tests. Use an installed Catch2 3 if there is one, otherwise fetch it.
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(Catch2)
endif()
include(Catch)

add_executable(nrs_unittests NrsslTest.cpp)
target_link_libraries(nrs_unittests PRIVATE nrs_core Catch2::Catch2WithMain ${JAVA_JVM_LIBRARY})
nrs_target_options(nrs_unittests)
if(NRS_ENABLE_UBSAN)
  target_compile_options(nrs_unittests PRIVATE -fsanitize=undefined -fno-sanitize-recover=undefined)
  target_link_options(nrs_unittests PRIVATE -fsanitize=undefined)
endif()
add_dependencies(nrs_unittests nrssl_jars)

catch_discover_tests(nrs_unittests
  PROPERTIES ENVIRONMENT "NRSSL_JARS=${NRSSL_JARS_DIR}")
