# NRSSL LLVM Pass

An LLVM pass plugin (`nrs-convert`) that converts the `float` operations and
constants of a program to one of the NRS representations (posit, Morris, ...)
and replaces them with the `llvm.riscv.nrssl.*` intrinsics of our LLVM fork,
which lower to the custom NRS instructions of our rocket-chip. Constants are
converted at compile time with [NRSSL](https://github.com/Albert24GG/nrssl)
(git submodule `nrssl/`), called through JNI.

```sh
opt -load-pass-plugin=build/src/libNRSConversionPass.so -passes='nrs-convert<posit1>' in.ll -S -o out.ll
```

Formats: `posit1`, `posit2`, `morris`, `morrisHeb`, `morrisBiasHeb`, `morrisUnaryHeb`.
The plugin starts a JVM for NRSSL and finds the jars of the build by itself
(`NRSSL_JARS` overrides the directory).

What the pass does with the code:

- f32 arithmetic, compares, int conversions, `min`/`max`/`sqrt` become intrinsics;
  `fneg`, `fabs` and the compares without an intrinsic (`>`, `>=`, `!=`) are
  built from the others
- f32 constants get the bits of the format, wherever they are (globals,
  operands, arrays, structs, vectors)
- errors: f32 operations without an NRS equivalent (`frem`, `fcmp ord/uno`,
  `fma` and other math intrinsics, vector operations) and `fpext`/`fptrunc`
  between NRS and IEEE values
- warnings: code on other floating point types (`double`, ...) stays IEEE,
  calls to f32 functions outside the module (libm) compute in IEEE
- the format is recorded in the `nrs.format` module flag, converting a module
  twice is an error

## Layout

| Path | What |
| --- | --- |
| `src/` | the pass |
| `nrssl/` | NRSSL, built with sbt as part of the build |
| `precision/` | accuracy tests running on the rocket-chip emulator, see [its README](precision/README.md) |
| `docker/` | the build and test environment |

## Environment

The image has the toolchains (RISC-V GCC, pk, spike, verilator, mill, firtool,
sbt, JDK 17, clang 19). The sources are mounted: a directory holding
`llvm-project` (our fork, built in `llvm-project/build`), `rocket-chip` and this
repository goes to `/workspace/shared`.

```sh
docker build -t nrs-dev --build-arg UID=$(id -u) --build-arg GID=$(id -g) docker
docker run -it --name nrs-dev -v /path/to/shared:/workspace/shared nrs-dev
```

The container runs as a user with your UID/GID, so build output in the mounted
directory belongs to you.

## Building and testing

```sh
git submodule update --init
cmake --preset default          # LLVM_DIR defaults to /workspace/shared/llvm-project/build/lib/cmake/llvm
cmake --build --preset default  # build/src/libNRSConversionPass.so and build/jars/
ctest --preset default          # lit tests (test/) and unit tests (unittests/)
```

Warnings are errors in the presets. The `tidy` preset also runs clang-tidy
(C++ Core Guidelines checks, see `.clang-tidy`), `ubsan` builds the unit tests
with the undefined behavior sanitizer.

The lit tests run `opt` with the plugin on small IR files and check the output
with FileCheck, for instance:

```llvm
; RUN: %opt_nrs -passes='nrs-convert<posit1>' -S %s | FileCheck %s
; CHECK: call float @llvm.riscv.nrssl.f32.posit1.add(float %x, float %y)
```

The precision tests need the rocket-chip emulator:

```sh
cd precision
cmake --preset posit1 && cmake --build --preset posit1 && ctest --preset posit1
```

## Code style

C++20, following the C++ Core Guidelines. `snake_case` for functions and
variables, `CamelCase` for types, formatted with the LLVM based
`.clang-format`. Errors are `llvm::Error`/`llvm::Expected`, LLVM is built
without exceptions and RTTI.
