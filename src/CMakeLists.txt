# Everything except the plugin entry point, shared by the plugin and the unit tests.
add_library(nrs_core STATIC nrssl/Jvm.cpp nrssl/Encoder.cpp)
target_include_directories(nrs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(nrs_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
nrs_target_options(nrs_core)
nrs_llvm_options(nrs_core)

add_library(NRSConversionPass MODULE NRSConversionPass.cpp)
# libjvm is linked directly (with a runpath to it), no LD_PRELOAD needed
target_link_libraries(NRSConversionPass PRIVATE nrs_core ${JAVA_JVM_LIBRARY})
# NRSSL_JARS can override this at run time
target_compile_definitions(NRSConversionPass PRIVATE NRS_DEFAULT_JARS_DIR="${NRSSL_JARS_DIR}")
nrs_target_options(NRSConversionPass)
nrs_llvm_options(NRSConversionPass)
