# Everything except the plugin entry point, shared by the plugin and the unit tests.
add_library(nrs_core STATIC NRSSL.cpp)
target_include_directories(nrs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(nrs_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
nrs_target_options(nrs_core)
nrs_llvm_options(nrs_core)

add_library(NRSConversionPass MODULE NRSConversionPass.cpp)
target_link_libraries(NRSConversionPass PRIVATE nrs_core)
nrs_target_options(NRSConversionPass)
nrs_llvm_options(NRSConversionPass)
