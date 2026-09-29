# NRSSL LLVM Pass

An LLVM pass plugin (`ieee-to-posit`) that converts the `float` operations and
constants of a program to one of the NRS representations (posit, Morris, ...)
and replaces them with the `llvm.riscv.nrssl.*` intrinsics of our LLVM fork,
which lower to the custom NRS instructions of our rocket-chip. Constants are
converted at compile time with [NRSSL](https://github.com/Albert24GG/nrssl)
(git submodule `nrssl/`), called through JNI.

```sh
opt -load-pass-plugin=build/src/libNRSConversionPass.so -passes=ieee-to-posit \
    --float_type=<posit1|posit2|morris|morrisHeb|morrisUnaryHeb|morrisBiasHeb> in.ll -S -o out.ll
```

The pass starts a JVM, so `opt` needs `NRSSL_JARS=build/jars` and
`LD_PRELOAD=$JAVA_HOME/lib/server/libjvm.so`.

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

## Building

```sh
git submodule update --init
cmake --preset default          # LLVM_DIR defaults to /workspace/shared/llvm-project/build/lib/cmake/llvm
cmake --build --preset default  # build/src/libNRSConversionPass.so and build/jars/
```

Then, for the precision tests:

```sh
cd precision
cmake --preset posit1 && cmake --build --preset posit1 && ctest --preset posit1
```
