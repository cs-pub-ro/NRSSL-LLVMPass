# lit + FileCheck tests of the pass: run opt with the plugin on small IR files.
find_program(LLVM_LIT
    NAMES llvm-lit lit lit.py
    HINTS
        ${LLVM_TOOLS_BINARY_DIR}
    REQUIRED
)

# the pass needs libjvm preloaded, FindJNI knows where it is
set(JVM_LIBRARY "${JAVA_JVM_LIBRARY}")

configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)

add_custom_target(check-lit
  COMMAND ${LLVM_LIT} -sv ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS NRSConversionPass nrssl_jars
  USES_TERMINAL
  VERBATIM)

add_test(NAME lit COMMAND ${LLVM_LIT} -sv ${CMAKE_CURRENT_BINARY_DIR})
