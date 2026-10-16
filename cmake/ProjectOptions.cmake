# nrs_configure_target_options(<target>)
#
# Warnings, clang-tidy and sanitizers for the project's own targets. Kept per
# target so that LLVM, JNI and fetched dependencies are not affected.

set(NRS_WARNINGS
  -Wall -Wextra -Wpedantic
  -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual
  -Wold-style-cast -Wcast-align -Wunused
  -Wconversion -Wsign-conversion -Wdouble-promotion
  -Wnull-dereference -Wformat=2 -Wimplicit-fallthrough)

if(NRS_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-19 REQUIRED)
endif()

function(nrs_configure_target_options target)
  target_compile_features(${target} PRIVATE cxx_std_20)
  set_target_properties(${target} PROPERTIES
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
  )
  target_compile_options(${target} PRIVATE ${NRS_WARNINGS})
  if(NRS_WARNINGS_AS_ERRORS)
    target_compile_options(${target} PRIVATE -Werror)
  endif()
  if(NRS_ENABLE_CLANG_TIDY)
    set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
  endif()
endfunction()

# LLVM is built without RTTI and exceptions, code linked into the plugin must
# match.
function(nrs_configure_llvm_options target)
  target_compile_options(${target} PRIVATE -fno-rtti -fno-exceptions)
  separate_arguments(_llvm_definitions NATIVE_COMMAND "${LLVM_DEFINITIONS}")
  target_compile_definitions(${target} PRIVATE ${_llvm_definitions})
  target_include_directories(${target} SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS} ${JNI_INCLUDE_DIRS})
endfunction()
