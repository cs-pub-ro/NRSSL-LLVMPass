# Cross toolchain: custom clang from llvm-project, newlib/libgcc from the
# rocket-tools GCC toolchain. Defaults match the racheta container.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(LLVM_BIN_DIR "/workspace/shared/llvm-project/build/bin" CACHE PATH "Directory with clang, opt and llc")
set(RISCV_GCC_TOOLCHAIN "/opt/riscv" CACHE PATH "RISC-V GCC toolchain (newlib, libgcc, binutils)")

set(CMAKE_C_COMPILER "${LLVM_BIN_DIR}/clang")
set(CMAKE_ASM_COMPILER "${LLVM_BIN_DIR}/clang")
set(CMAKE_C_COMPILER_TARGET riscv64-unknown-elf)
set(CMAKE_ASM_COMPILER_TARGET riscv64-unknown-elf)
set(CMAKE_SYSROOT "${RISCV_GCC_TOOLCHAIN}/riscv64-unknown-elf")
# binutils 2.39 from rocket-tools crashes on LLVM 19 objects, link with lld
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_OBJDUMP "${RISCV_GCC_TOOLCHAIN}/bin/riscv64-unknown-elf-objdump")

set(RISCV_ARCH_FLAGS "-march=rv64imafd -mabi=lp64d -mcmodel=medany")
set(CMAKE_C_FLAGS_INIT "${RISCV_ARCH_FLAGS} --gcc-toolchain=${RISCV_GCC_TOOLCHAIN}")
set(CMAKE_ASM_FLAGS_INIT "${RISCV_ARCH_FLAGS} --gcc-toolchain=${RISCV_GCC_TOOLCHAIN}")

# No startup files for the host-side compiler checks; our runtime provides them.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
