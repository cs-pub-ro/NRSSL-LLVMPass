# Precision tests

Small floating-point workloads (series, sequences, linear algebra and simple
ML algorithms) that measure the accuracy of the NRS representations against
IEEE 754. Each test is built either as plain IEEE code or through the NRS
pass of this repository, which lowers the float operations to the custom NRS
instructions of our rocket-chip, and runs bare metal on the rocket-chip
Verilator emulator.

These started out as [caial](https://github.com/sdcioc/caial) and were
rewritten for the NRS pass.

## Requirements

Everything is expected to run in the container from `../docker`, with the
`shared/` directory mounted at `/workspace/shared`:

- the pass and the NRSSL jars, built from the parent directory (`cmake --preset default && cmake --build --preset default` there)
- clang/opt/llc/ld.lld from `shared/llvm-project/build`
- the RISC-V GCC toolchain in `/opt/riscv` (only for headers, libgcc and objdump)
- the emulator from `shared/rocket-chip/out/emulator/...DefaultConfig...`
- python3 and java (for decoding NRS values)

All paths are CMake cache variables with these defaults, see
`CMakeLists.txt` and `cmake/riscv64-clang.cmake` to override them.

## Building and running

There is one preset (and build directory) per representation: `ieee`,
`ieee64` (IEEE with `fp_t = double`), `posit1`, `posit2`, `morris`,
`morrisHeb`, `morrisUnaryHeb`, `morrisBiasHeb`.

```sh
cmake --preset posit1           # configure build/posit1
cmake --build --preset posit1   # build all tests
ctest --preset posit1           # run all tests, results in build/posit1/Testing/Temporary/LastTest.log
ninja -C build/posit1 run-pi    # raw output of one test
ninja -C build/posit1 trace-pi  # +verbose, instruction trace in build/posit1/pi/pi.trace
```

Other options: `-DCAIAL_OPT_LEVEL=0..3` (default 2), `-DCAIAL_TRACE=ON` (per
iteration values, see below).

Every test is compiled as `clang -> [NRS pass] -> opt -O<n> -> llc`, so the IEEE
and NRS builds differ only by the pass. The pass runs on unoptimized IR, before
anything can fold constants or rewrite float operations with IEEE semantics.
`build/<type>/<test>/` keeps all the intermediate files:

| File | Content |
| --- | --- |
| `<test>.ll` | clang output, unoptimized |
| `<test>.nrs.ll` | after the NRS pass (NRS builds only) |
| `<test>.opt.ll` | after `opt -O<n>` |
| `<test>.s` | assembly |
| `<test>.elf`, `<test>.dump` | binary and its disassembly |
| `<test>.results.csv` | decoded results and differences, written by ctest |
| `<test>.trace.csv` | decoded per-iteration values, with `CAIAL_TRACE=ON` |

NRS builds fail if the post-pass IR still contains IEEE float operations
(`tools/caial.py check-ir`), so a test cannot silently run partly in IEEE.

## Results and references

The target can't turn NRS values into something readable, every float
operation in the test is converted too. So the tests only print raw bits
(`include/caial.h`) and `tools/caial.py run`, which ctest calls, decodes them:
IEEE directly, NRS values with NRSSL (`tools/NrsDecode.java`). Each result is
shown next to three references and the absolute difference to each:

- **ref f32**: the IEEE float result of the same test
- **ref f64**: the IEEE double result of the same test (only rounding error, no algorithmic error)
- **exact**: the mathematical value, where the test knows it

```
e (posit1, fp32)
result                        value          ref f32     |diff|          ref f64     |diff|            exact     |diff|
e                        2.71828184       2.71828198 1.49011612e-07       2.71828183 6.85856616e-09       2.71828183 6.8585666e-09
```

The IEEE builds must reproduce their gold values bit for bit. The NRS builds
only report differences for now, there are no tolerances yet.

The f32 and f64 references live in `tests/<test>.gold.h`. Regenerate them
after changing a test:

```sh
cmake --build --preset ieee64 && cmake --build --preset ieee --target gold
```

## Traces

With `-DCAIAL_TRACE=ON` the tests also print their intermediate values
(`CAIAL_TRACE_VALUE`), which end up decoded in `<test>.trace.csv`. Two runs can
be compared to see where a representation drifts away:

```sh
tools/caial.py diff build/ieee64/pi/pi.trace.csv build/posit1/pi/pi.trace.csv --threshold 1e-6
```

## Writing a test

A test is a single C file in `tests/`, registered in `CMakeLists.txt` with
`caial_add_test(<name> tests/<name>.c)`, that computes with `fp_t` and reports
through `CAIAL_RESULT(S)`, `CAIAL_IRESULT(S)` and `CAIAL_EXACT`. Result names
are C identifiers. Then run the gold target.

Keep in mind what the pass can convert:

- only `float` (f32), no `double` anywhere; use `FP_C(1.5)` for literals
- no libm, use `fp_sqrt`, `fp_abs` from `caial.h` and `fp_exp` from `caial_math.h`
- int <-> float conversions only for 32-bit signed ints
- no printing of floats, report them through the macros

## Runtime

`runtime/` holds a minimal bare-metal runtime: startup code, HTIF console and
exit, a trap handler and a tiny libc (`printf` without floating point, `mem*`,
`str*`). There is no newlib: the one shipped with the toolchain is built for
medlow and cannot be linked at 0x80000000.

## Tests

| Test | What |
| --- | --- |
| `e`, `pi`, `series`, `sequences` | series and sequences with known limits (e, pi three ways, H_n, pi^2/6, ln 2, sqrt 2, golden ratio) |
| `logistic` | chaotic logistic map, error growth |
| `summation` | naive vs Kahan summation, telescoping sum with cancellation |
| `horner` | (x-1)^7 near 1 (catastrophic cancellation), Taylor sin |
| `matmul` | 6x6 Hilbert matrix squared |
| `ops` | every NRS instruction on a table of inputs |
| `knn`, `kmeans`, `linreg`, `nb`, `dnn`, `svm` | small ML algorithms on the iris data |

The CNN, CT and DNN2 tests of the original caial were dropped: CNN took very
long without adding much, CT is mostly integer logic and DNN2 overlapped with
DNN.

## Known limitations

- Only 32-bit floats: the pass maps f32 operations only and leaves `double`
  code alone (the IR check rejects it).
- Int <-> float conversions work for 32-bit signed ints only. The hardware
  converts the low 32 bits as signed, and the pass sign-extends unsigned inputs.
- The Morris formats have no infinity: out-of-range values become NaR, which
  then propagates. Keep intermediate values in range (see `fp_exp`).
- Morris and MorrisHEB round to zero in the hardware (rocket-chip `FPU.scala`),
  the other formats and NRSSL round to nearest even. Round-to-nearest-even is
  broken for these two: the nrshgl encoders round by adding 1 to the packed
  bits, which only works when the bit order is the value order. With a
  variable-length exponent field, a carry out of the fraction corrupts the
  exponent (Viete's pi goes wrong once sqrt(2+a) rounds up to 2). The fix
  belongs in the nrshgl encoders: round the mantissa before packing and bump
  the exponent on overflow. Until then, long accumulations in these two
  formats show the bias of truncation.
- Constant expressions such as `FP_C(1.0) / FP_C(3.0)` are folded by clang in
  IEEE before the pass sees them, and only the rounded result is converted.
  Write the operands as separate values if that matters.
- NRS builds only report errors, there are no tolerance checks yet.

## Authors

* **Ciocîrlan Ștefan-Dan** - original algorithms and build system - [sdcioc](https://github.com/sdcioc)

## License

BSD 3-Clause, see [LICENSE](LICENSE).
