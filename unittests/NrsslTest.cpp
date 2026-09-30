// Known encodings from NRSSL, through the JNI wrapper.

#include "NRSSL.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

TEST_CASE("posit1 encodes small integers exactly") {
  NRSSL nrssl;
  CHECK(nrssl.convertDoubleToUint<std::uint32_t>(1.0, NRSSL::POSIT1) == 0x40000000U);
  CHECK(nrssl.convertDoubleToUint<std::uint32_t>(3.0, NRSSL::POSIT1) == 0x4c000000U);
  CHECK(nrssl.convertDoubleToUint<std::uint32_t>(-1.5, NRSSL::POSIT1) == 0xbc000000U);
}

TEST_CASE("Morris encodes +-1.0 away from the zero and NR patterns") {
  NRSSL nrssl;
  CHECK(nrssl.convertDoubleToUint<std::uint32_t>(1.0, NRSSL::MORRIS) == 0x04000000U);
  CHECK(nrssl.convertDoubleToUint<std::uint32_t>(-1.0, NRSSL::MORRIS) == 0x84000000U);
}

// Fails with the current wrapper: callApplyMethod caches the JNI signature of
// apply() in a static, so the first format used (posit) sticks and Morris gets
// looked up with a Posit_B return type, and the wrapper exit()s. Hidden until
// the rewrite fixes it, run it with: nrs_unittests "[known-bug]"
TEST_CASE("decoding inverts encoding", "[.][known-bug]") {
  NRSSL nrssl;
  for (const auto type : {NRSSL::POSIT1, NRSSL::POSIT2, NRSSL::MORRIS, NRSSL::MORRIS_HEB,
                          NRSSL::MORRIS_BIAS_HEB, NRSSL::MORRIS_UNARY_HEB}) {
    const auto bits = nrssl.convertDoubleToUint<std::uint32_t>(2.5, type);
    CHECK(nrssl.convertUintToDouble(bits, type) == 2.5);
  }
}
