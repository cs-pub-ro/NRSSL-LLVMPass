# Everything except the plugin entry point, shared by the plugin and the unit tests.
add_library(nrs_core STATIC)
target_sources(nrs_core
    PRIVATE
        ConstantConverter.cpp
        InstructionLowering.cpp
        NrsConversionPass.cpp
        nrssl/Encoder.cpp
        nrssl/Jvm.cpp

    PRIVATE
        FILE_SET internalHeaders
        TYPE HEADERS
        FILES
            ConstantConverter.h
            InstructionLowering.h
            nrssl/Encoder.h
            nrssl/Jvm.h

    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/include
        FILES
            ${PROJECT_SOURCE_DIR}/include/nrs/NrsConversionPass.h
            ${PROJECT_SOURCE_DIR}/include/nrs/NrsFormat.h
)
set_target_properties(nrs_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
nrs_configure_target_options(nrs_core)
nrs_configure_llvm_options(nrs_core)

add_library(NRSConversionPass MODULE Plugin.cpp)
# libjvm is linked directly (with a runpath to it), no LD_PRELOAD needed
target_link_libraries(NRSConversionPass PRIVATE nrs_core ${JAVA_JVM_LIBRARY})
# NRSSL_JARS can override this at run time
target_compile_definitions(NRSConversionPass PRIVATE NRS_DEFAULT_JARS_DIR="${NRSSL_JARS_DIR}")
nrs_configure_target_options(NRSConversionPass)
nrs_configure_llvm_options(NRSConversionPass)
