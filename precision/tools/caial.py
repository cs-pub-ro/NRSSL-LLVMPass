#!/usr/bin/env python3
"""Host side of the caial tests.

  caial.py run      run one test on the emulator, decode and compare its results
  caial.py gold     regenerate tests/<test>.gold.h from the IEEE f32 and f64 builds
  caial.py diff     compare the traces of two runs of the same test
  caial.py summary  one table of relative errors over several builds
  caial.py check-ir fail if post-pass IR still has IEEE float operations

Tests print raw bits (see include/caial.h). IEEE bits are decoded here, NRS
bits with NRSSL through tools/NrsDecode.java.
"""

import argparse
import csv
import math
import os
import re
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))


def ieee_to_float(bits, width):
    if width == 32:
        return struct.unpack(">f", struct.pack(">I", bits))[0]
    return struct.unpack(">d", struct.pack(">Q", bits))[0]


def decode(bits, float_type, width, jars):
    """Decode a list of bit patterns of the given representation to floats."""
    if not bits:
        return []
    if float_type == "ieee":
        return [ieee_to_float(b, width) for b in bits]
    if not jars:
        sys.exit("--jars is needed to decode %s values" % float_type)
    cp = os.pathsep.join(os.path.join(jars, j) for j in ("nrssl.jar", "scala-library.jar"))
    out = subprocess.run(
        ["java", "-cp", cp, os.path.join(TOOLS_DIR, "NrsDecode.java"), float_type, str(width)],
        input="\n".join("%x" % b for b in bits),
        capture_output=True,
        text=True,
        check=True,
    ).stdout.split()
    return [float(v) for v in out]


def run_emulator(emulator, elf, timeout):
    """Run a test binary, return (exit code, output lines). Exit code None means timeout."""
    try:
        p = subprocess.run([emulator, elf], capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired as e:
        out = (e.stdout or b"").decode(errors="replace") + (e.stderr or b"").decode(errors="replace")
        return None, out.splitlines()
    return p.returncode, p.stdout.splitlines() + p.stderr.splitlines()


def parse(lines):
    """Pick the @-lines of the test output apart."""
    results, iresults, exact, trace = [], [], {}, []
    for line in lines:
        f = line.split()
        if not f or not f[0].startswith("@"):
            continue
        if f[0] == "@result":
            gold = [None if g == "-" else int(g, 16) for g in f[4:6]]
            results.append((f[1], int(f[2]), int(f[3], 16), gold[0], gold[1]))
        elif f[0] == "@iresult":
            iresults.append((f[1], int(f[2]), int(f[3]), None if f[4] == "-" else int(f[4])))
        elif f[0] == "@exact":
            exact[(f[1], int(f[2]))] = f[3]
        elif f[0] == "@trace":
            trace.append((f[1], int(f[2]), int(f[3], 16)))
    return results, iresults, exact, trace


def fmt(v):
    return "-" if v is None else "%.9g" % v


def absdiff(a, b):
    return None if a is None or b is None else abs(a - b)


def cmd_run(args):
    width = args.fp_bits
    code, lines = run_emulator(args.emulator, args.elf, args.timeout)
    for line in lines:
        if not line.startswith("@") and args.verbose:
            print(line)
    results, iresults, exact, trace = parse(lines)
    failed = code != 0
    if code is None:
        print("FAIL: timed out after %ds" % args.timeout)
    elif code != 0:
        print("\n".join(l for l in lines if not l.startswith("@")))
        print("FAIL: emulator exited with %d" % code)

    values = decode([r[2] for r in results], args.type, width, args.jars)
    trace_values = decode([t[2] for t in trace], args.type, width, args.jars)

    os.makedirs(args.out, exist_ok=True)
    name = os.path.splitext(os.path.basename(args.elf))[0]

    rows = []
    for (res, idx, bits, g32, g64), v in zip(results, values):
        ref32 = None if g32 is None else ieee_to_float(g32, 32)
        ref64 = None if g64 is None else ieee_to_float(g64, 64)
        ex = exact.get((res, idx))
        ex = None if ex is None else float(ex)
        rows.append((res, idx, bits, v, ref32, ref64, ex))
        if args.type == "ieee":
            gold = g32 if width == 32 else g64
            if gold is None:
                print("FAIL: %s[%d] has no gold value, run caial.py gold" % (res, idx))
                failed = True
            elif gold != bits:
                print("FAIL: %s[%d] = %s, gold is %s" % (res, idx, fmt(v), fmt(ieee_to_float(gold, width))))
                failed = True

    with open(os.path.join(args.out, name + ".results.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["name", "idx", "bits", "value", "ref_f32", "ref_f64", "exact",
                    "absdiff_f32", "absdiff_f64", "absdiff_exact"])
        for res, idx, bits, v, r32, r64, ex in rows:
            w.writerow([res, idx, "%x" % bits, repr(v), repr(r32), repr(r64), repr(ex),
                        absdiff(v, r32), absdiff(v, r64), absdiff(v, ex)])
    with open(os.path.join(args.out, name + ".trace.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["label", "iter", "bits", "value"])
        for (label, it, bits), v in zip(trace, trace_values):
            w.writerow([label, it, "%x" % bits, repr(v)])

    print("%s (%s, fp%d)" % (name, args.type, width))
    hdr = ("result", "value", "ref f32", "|diff|", "ref f64", "|diff|", "exact", "|diff|")
    print("%-18s %16s %16s %10s %16s %10s %16s %10s" % hdr)
    for res, idx, bits, v, r32, r64, ex in rows:
        label = res if idx == 0 and not any(r[0] == res and r[1] > 0 for r in rows) else "%s[%d]" % (res, idx)
        print("%-18s %16s %16s %10s %16s %10s %16s %10s" % (
            label, fmt(v), fmt(r32), fmt(absdiff(v, r32)), fmt(r64), fmt(absdiff(v, r64)),
            fmt(ex), fmt(absdiff(v, ex))))
    for res, idx, v, gold in iresults:
        mark = "" if gold is None or gold == v else "   (differs)"
        print("%-18s %16d %16s%s" % ("%s[%d]" % (res, idx), v, "-" if gold is None else gold, mark))
        if args.type == "ieee" and gold != v:
            print("FAIL: %s[%d] = %d, gold is %s" % (res, idx, v, gold))
            failed = True

    if not results and not iresults:
        print("FAIL: no results reported")
        failed = True
    return 1 if failed else 0


def cmd_gold(args):
    """Run every test in both IEEE builds and write tests/<test>.gold.h."""
    def collect(build, test):
        code, lines = run_emulator(args.emulator, os.path.join(build, test, test + ".elf"), args.timeout)
        if code != 0:
            sys.exit("%s in %s exited with %s:\n%s" % (test, build, code, "\n".join(lines)))
        results, iresults, _, _ = parse(lines)
        return results, iresults

    tests = args.tests or sorted(
        d for d in os.listdir(args.f32) if os.path.exists(os.path.join(args.f32, d, d + ".elf")))
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        r32 = dict(zip(tests, pool.map(lambda t: collect(args.f32, t), tests)))
        r64 = dict(zip(tests, pool.map(lambda t: collect(args.f64, t), tests)))

    for test in tests:
        (res32, ires), (res64, ires64) = r32[test], r64[test]
        names = list(dict.fromkeys(r[0] for r in res32))
        out = ["// Generated by tools/caial.py gold, do not edit.",
               "#define CAIAL_HAVE_GOLD 1", ""]
        for n in names:
            b32 = ["0x%08x" % r[2] for r in res32 if r[0] == n]
            b64 = ["0x%016xull" % r[2] for r in res64 if r[0] == n]
            if len(b32) != len(b64):
                sys.exit("%s: %s has %d f32 and %d f64 values" % (test, n, len(b32), len(b64)))
            out.append("static const uint32_t gold32_%s[] = {%s};" % (n, ", ".join(b32)))
            out.append("static const uint64_t gold64_%s[] = {%s};" % (n, ", ".join(b64)))
        for n in dict.fromkeys(r[0] for r in ires):
            vals = [str(r[2]) for r in ires if r[0] == n]
            out.append("static const long goldi_%s[] = {%s};" % (n, ", ".join(vals)))
            v64 = [str(r[2]) for r in ires64 if r[0] == n]
            if v64 != vals:
                print("note: %s: %s differs between f32 and f64, keeping the f32 values" % (test, n))
        path = os.path.join(args.src, test + ".gold.h")
        with open(path, "w") as f:
            f.write("\n".join(out) + "\n")
        print("wrote", path)
    return 0


def cmd_diff(args):
    """Line up two trace CSVs and show where they drift apart."""
    def load(path):
        with open(path) as f:
            return [(r["label"], int(r["iter"]), float(r["value"])) for r in csv.DictReader(f)]

    a, b = load(args.a), load(args.b)
    first = {}
    print("%-12s %6s %16s %16s %10s %10s" % ("label", "iter", "a", "b", "|diff|", "rel"))
    for (la, ia, va), (lb, ib, vb) in zip(a, b):
        if (la, ia) != (lb, ib):
            print("traces are not aligned at %s/%d vs %s/%d" % (la, ia, lb, ib))
            return 1
        d = abs(va - vb)
        rel = d / abs(va) if va else d
        mark = ""
        if rel > args.threshold and la not in first:
            first[la] = ia
            mark = "  <- first above %g" % args.threshold
        if not args.only_drift or mark:
            print("%-12s %6d %16s %16s %10s %10s%s" % (la, ia, fmt(va), fmt(vb), fmt(d), fmt(rel), mark))
    return 0


def cmd_summary(args):
    """Relative error of every result against the f64 (or exact) reference, one column per build."""
    col = "exact" if args.ref == "exact" else "ref_f64"
    table, order = {}, []
    for build in args.builds:
        for test in sorted(os.listdir(build)):
            path = os.path.join(build, test, test + ".results.csv")
            if not os.path.exists(path):
                continue
            with open(path) as f:
                for r in csv.DictReader(f):
                    key = (test, r["name"], int(r["idx"]))
                    if key not in table:
                        table[key] = {}
                        order.append(key)
                    v, ref = float(r["value"]), r[col]
                    if ref == "None":
                        continue
                    ref = float(ref)
                    err = abs(v - ref) / abs(ref) if ref else abs(v - ref)
                    table[key][build] = err
    names = [os.path.basename(os.path.normpath(b)) for b in args.builds]
    print("relative error against %s" % ("the exact value" if args.ref == "exact" else "IEEE double"))
    print("%-28s" % "result" + "".join("%15s" % n for n in names))
    for key in order:
        errs = [table[key].get(b) for b in args.builds]
        if all(e is None for e in errs):
            continue
        print("%-28s" % ("%s.%s[%d]" % key) + "".join("%15s" % ("-" if e is None else "%.3g" % e) for e in errs))
    return 0


# IEEE float work that the pass left in place. Run on the .nrs.ll output.
IR_PATTERNS = [
    (re.compile(r"= (fadd|fsub|fmul|fdiv|frem|fneg)( \w+)* (float|double)\b"), "float arithmetic"),
    (re.compile(r"= fcmp \w+ (float|double)\b"), "float compare"),
    (re.compile(r"= (fpext|fptrunc)\b"), "float width conversion"),
    (re.compile(r"= (sitofp|uitofp) \S+ \S+ to (float|double)|= (fptosi|fptoui) (float|double)"), "int conversion"),
    (re.compile(r"call \S*\s*(float|double) @llvm\.(fma|fmuladd|fabs|copysign|minnum|maxnum|sqrt|pow|exp|log|sin|cos)"),
     "float intrinsic"),
    (re.compile(r"call \S*\s*(float|double) @(?!llvm\.)"), None),  # checked below
]


def cmd_check_ir(args):
    with open(args.ll) as f:
        text = f.read()
    defined = set(re.findall(r"^define [^@]*@([\w.]+)\(", text, re.M))
    bad = []
    for n, line in enumerate(text.splitlines(), 1):
        if line.startswith(("declare", "define", "!", "attributes", "@")):
            continue
        if re.search(r"\bdouble\b", line):
            bad.append((n, "double", line))
            continue
        for pat, what in IR_PATTERNS:
            if not pat.search(line):
                continue
            if what is None:
                m = re.search(r"@([\w.]+)\(", line)
                if m and m.group(1) in defined:
                    continue
                what = "call to a float library function"
            bad.append((n, what, line))
            break
    for n, what, line in bad:
        print("%s:%d: %s: %s" % (args.ll, n, what, line.strip()))
    if bad:
        print("%d IEEE float operation(s) left after the NRS pass" % len(bad))
    return 1 if bad else 0


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    r = sub.add_parser("run", help="run a test and compare against the gold values")
    r.add_argument("--type", required=True, help="ieee or an NRS type")
    r.add_argument("--fp-bits", type=int, default=32)
    r.add_argument("--emulator", required=True)
    r.add_argument("--jars", help="directory with nrssl.jar, needed for the NRS types")
    r.add_argument("--out", required=True, help="where to put <test>.results.csv and <test>.trace.csv")
    r.add_argument("-v", "--verbose", action="store_true", help="also print the plain test output")
    r.add_argument("--timeout", type=int, default=300, help="seconds before the emulator is killed")
    r.add_argument("elf")

    g = sub.add_parser("gold", help="regenerate the gold headers")
    g.add_argument("--f32", required=True, help="IEEE build dir with FP_BITS=32")
    g.add_argument("--f64", required=True, help="IEEE build dir with FP_BITS=64")
    g.add_argument("--src", required=True, help="where the gold headers go (tests/)")
    g.add_argument("--emulator", required=True)
    g.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    g.add_argument("--timeout", type=int, default=300)
    g.add_argument("tests", nargs="*")

    d = sub.add_parser("diff", help="compare two trace CSVs")
    d.add_argument("a")
    d.add_argument("b")
    d.add_argument("--threshold", type=float, default=1e-6, help="relative difference to flag")
    d.add_argument("--only-drift", action="store_true", help="only print the first line above the threshold")

    s = sub.add_parser("summary", help="relative errors of all results over several builds")
    s.add_argument("builds", nargs="+", help="build directories that ran ctest")
    s.add_argument("--ref", choices=["f64", "exact"], default="f64")

    c = sub.add_parser("check-ir", help="check post-pass IR for leftover IEEE float operations")
    c.add_argument("ll")

    args = p.parse_args()
    return {"run": cmd_run, "gold": cmd_gold, "diff": cmd_diff, "summary": cmd_summary, "check-ir": cmd_check_ir}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
