// NRSSL through the encoder: known encodings and round trips.

#include "nrssl/Encoder.h"

#include "nrs/NrsFormat.h"
#include "nrssl/Jvm.h"

#include <llvm/Support/Error.h>

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace {

nrs::nrssl::Encoder make_encoder(nrs::Format format) {
  const char *jars = std::getenv("NRSSL_JARS");
  REQUIRE(jars != nullptr);
  auto jvm = nrs::nrssl::Jvm::get(jars);
  REQUIRE(static_cast<bool>(jvm));
  auto encoder = nrs::nrssl::Encoder::create(format, *jvm);
  if (!encoder) {
    FAIL(llvm::toString(encoder.takeError()));
  }
  return std::move(*encoder);
}

std::uint64_t encode(const nrs::nrssl::Encoder &encoder, double value) {
  auto bits = encoder.encode(value);
  if (!bits) {
    FAIL(llvm::toString(bits.takeError()));
  }
  return *bits;
}

double decode(const nrs::nrssl::Encoder &encoder, std::uint64_t bits) {
  auto value = encoder.decode(bits);
  if (!value) {
    FAIL(llvm::toString(value.takeError()));
  }
  return *value;
}

} // namespace

TEST_CASE("posit1 encodes small values exactly") {
  const auto posit1 = make_encoder(nrs::Format::Posit1);
  CHECK(encode(posit1, 0.0) == 0x00000000U);
  CHECK(encode(posit1, 1.0) == 0x40000000U);
  CHECK(encode(posit1, 3.0) == 0x4c000000U);
  CHECK(encode(posit1, -1.5) == 0xbc000000U);
}

TEST_CASE("Morris encodes +-1.0 away from the zero and NR patterns") {
  const auto morris = make_encoder(nrs::Format::Morris);
  CHECK(encode(morris, 1.0) == 0x04000000U);
  CHECK(encode(morris, -1.0) == 0x84000000U);
}

TEST_CASE("decoding inverts encoding in every format") {
  for (const auto &format : nrs::formats) {
    const auto encoder = make_encoder(format.format);
    for (const double value : {1.0, -2.5, 0.5, 1000.0, static_cast<double>(0.1F)}) {
      INFO(std::string(format.name) << " " << value);
      CHECK(decode(encoder, encode(encoder, value)) == value);
    }
  }
}
