# caial_add_test(<name> <source>)
#
# Builds one test through an explicit clang -> opt -> llc pipeline, so that the
# IEEE and NRS builds differ only by the conversion pass:
#
#   <name>.ll      clang output, unoptimized (no optnone)
#   <name>.nrs.ll  after the NRS pass (NRS builds only)
#   <name>.opt.ll  after opt -O<level>
#   <name>.s/.o    llc output
#   <name>.elf     linked with the bare-metal runtime, plus <name>.dump
#
# The pass runs before any optimization: running it on optimized IR would let
# InstCombine & co. fold constants and rewrite float ops with IEEE semantics.
# It must also run exactly once, since it rewrites constants in place.

set(CAIAL_C_FLAGS -std=gnu11 -g -ffp-contract=off -fno-math-errno -Wall
  -DFP_BITS=${CAIAL_FP_BITS} -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/data -I${CMAKE_SOURCE_DIR}/tests)
if(CAIAL_TRACE)
  list(APPEND CAIAL_C_FLAGS -DCAIAL_TRACE)
endif()

# Gold headers are picked up with __has_include, so reconfigure when one appears.
file(GLOB _caial_gold_headers CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*.gold.h)

set(CAIAL_PY ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/caial.py)

separate_arguments(_caial_arch_flags UNIX_COMMAND "${RISCV_ARCH_FLAGS}")
set(_caial_clang
  ${CMAKE_C_COMPILER} --target=${CMAKE_C_COMPILER_TARGET} --sysroot=${CMAKE_SYSROOT}
  --gcc-toolchain=${RISCV_GCC_TOOLCHAIN} ${_caial_arch_flags} ${CAIAL_C_FLAGS})

if(CAIAL_OPT_LEVEL STREQUAL "0")
  set(_caial_frontend_opt -O0 -Xclang -disable-O0-optnone)
else()
  set(_caial_frontend_opt -O${CAIAL_OPT_LEVEL} -Xclang -disable-llvm-passes)
endif()

set(_caial_llc ${LLC} -O${CAIAL_OPT_LEVEL} -march=riscv64 -mattr=+m,+a,+f,+d
  -target-abi=lp64d -code-model=medium)

function(caial_add_test name source)
  set(out "${CMAKE_BINARY_DIR}/${name}")
  set(src "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
  file(MAKE_DIRECTORY "${out}")
  set(deps "${src}")
  if(EXISTS "${CMAKE_SOURCE_DIR}/tests/${name}.gold.h")
    list(APPEND deps "${CMAKE_SOURCE_DIR}/tests/${name}.gold.h")
  endif()

  add_custom_command(
    OUTPUT "${out}/${name}.ll"
    COMMAND ${_caial_clang} ${_caial_frontend_opt} -DCAIAL_TEST=${name}
            -MD -MF "${out}/${name}.d" -S -emit-llvm -o "${out}/${name}.ll" "${src}"
    DEPENDS ${deps}
    DEPFILE "${out}/${name}.d"
    COMMENT "[${name}] clang -> ${name}.ll"
    VERBATIM)

  set(ir "${out}/${name}.ll")
  if(NOT CAIAL_FLOAT_TYPE STREQUAL "ieee")
    add_custom_command(
      OUTPUT "${out}/${name}.nrs.ll"
      COMMAND ${CMAKE_COMMAND} -E env LD_PRELOAD=${JVM_LIB} NRSSL_JARS=${NRSSL_JARS}
              ${OPT} -load-pass-plugin=${NRS_PASS_PLUGIN} -passes=ieee-to-posit
              --float_type=${CAIAL_FLOAT_TYPE} -S -o "${out}/${name}.nrs.ll" "${ir}"
              > "${out}/${name}.pass.log"
      COMMAND ${CAIAL_PY} check-ir "${out}/${name}.nrs.ll"
      DEPENDS "${ir}" "${NRS_PASS_PLUGIN}" "${NRSSL_JARS}/nrssl.jar" "${CMAKE_SOURCE_DIR}/tools/caial.py"
      COMMENT "[${name}] NRS pass (${CAIAL_FLOAT_TYPE}) -> ${name}.nrs.ll"
      VERBATIM)
    set(ir "${out}/${name}.nrs.ll")
  endif()

  add_custom_command(
    OUTPUT "${out}/${name}.opt.ll"
    COMMAND ${OPT} -passes=default<O${CAIAL_OPT_LEVEL}> -S -o "${out}/${name}.opt.ll" "${ir}"
    DEPENDS "${ir}"
    COMMENT "[${name}] opt -O${CAIAL_OPT_LEVEL} -> ${name}.opt.ll"
    VERBATIM)

  add_custom_command(
    OUTPUT "${out}/${name}.o" "${out}/${name}.s"
    COMMAND ${_caial_llc} -filetype=obj -o "${out}/${name}.o" "${out}/${name}.opt.ll"
    COMMAND ${_caial_llc} -filetype=asm -o "${out}/${name}.s" "${out}/${name}.opt.ll"
    DEPENDS "${out}/${name}.opt.ll"
    COMMENT "[${name}] llc -> ${name}.o, ${name}.s"
    VERBATIM)

  set_source_files_properties("${out}/${name}.o" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
  add_executable(${name} "${out}/${name}.o")
  target_link_libraries(${name} PRIVATE caial_runtime)
  set_target_properties(${name} PROPERTIES
    LINKER_LANGUAGE C
    OUTPUT_NAME "${name}.elf"
    RUNTIME_OUTPUT_DIRECTORY "${out}")
  add_custom_command(TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${name}> > "${out}/${name}.dump"
    VERBATIM)

  # run-<name>: plain run; trace-<name>: +verbose, disassembled trace in <name>.trace
  add_custom_target(run-${name}
    COMMAND ${EMULATOR} $<TARGET_FILE:${name}>
    DEPENDS ${name}
    USES_TERMINAL
    VERBATIM)
  add_custom_target(trace-${name}
    COMMAND bash -c "'${EMULATOR}' +verbose '$<TARGET_FILE:${name}>' 2> >(spike-dasm > '${out}/${name}.trace')"
    DEPENDS ${name}
    USES_TERMINAL
    VERBATIM)

  set_property(GLOBAL APPEND PROPERTY CAIAL_TESTS ${name})

  add_test(NAME ${name}
    COMMAND ${CAIAL_PY} run --type ${CAIAL_FLOAT_TYPE} --fp-bits ${CAIAL_FP_BITS}
            --emulator ${EMULATOR} --jars ${NRSSL_JARS} --timeout ${CAIAL_TEST_TIMEOUT}
            --out "${out}" $<TARGET_FILE:${name}>)
endfunction()

# gold: rerun all tests in this IEEE f32 build and the IEEE f64 build and
# rewrite tests/<test>.gold.h. Build the ieee64 preset first.
function(caial_add_gold_target)
  if(NOT (CAIAL_FLOAT_TYPE STREQUAL "ieee" AND CAIAL_FP_BITS STREQUAL "32"))
    return()
  endif()
  set(CAIAL_GOLD_F64_DIR "${CMAKE_SOURCE_DIR}/build/ieee64" CACHE PATH "IEEE f64 build used by the gold target")
  get_property(tests GLOBAL PROPERTY CAIAL_TESTS)
  add_custom_target(gold
    COMMAND ${CAIAL_PY} gold --f32 ${CMAKE_BINARY_DIR} --f64 ${CAIAL_GOLD_F64_DIR}
            --src ${CMAKE_SOURCE_DIR}/tests --emulator ${EMULATOR} ${tests}
    DEPENDS ${tests}
    USES_TERMINAL
    VERBATIM)
endfunction()
