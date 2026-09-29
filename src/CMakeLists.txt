add_library(NRSConversionPass MODULE NRSConversionPass.cpp NRSSL.cpp)

separate_arguments(_llvm_definitions NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_definitions(NRSConversionPass PRIVATE ${_llvm_definitions})
target_include_directories(NRSConversionPass PRIVATE ${LLVM_INCLUDE_DIRS} ${JNI_INCLUDE_DIRS})
target_compile_options(NRSConversionPass PRIVATE -Wall -fno-rtti)
