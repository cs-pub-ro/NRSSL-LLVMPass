// The format table has to match the intrinsics of the LLVM fork.

#include "nrs/NrsFormat.h"

#include <llvm/IR/Intrinsics.h>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>
#include <string_view>

namespace {

// Same order as nrs::Op.
constexpr std::array<std::string_view, nrs::op_count> op_names{
    "add", "sub", "mul", "div",      "min",       "max",        "sqrt",
    "eq",  "lt",  "le",  "cvtToInt", "cvtToUInt", "cvtFromInt", "cvtFromUInt",
};

} // namespace

TEST_CASE("every format maps each op to llvm.riscv.nrssl.f32.<format>.<op>") {
  for (const auto &format : nrs::formats) {
    for (std::size_t op = 0; op < nrs::op_count; ++op) {
      const auto expected =
          "llvm.riscv.nrssl.f32." + std::string(format.name) + "." + std::string(op_names.at(op));
      CHECK(llvm::Intrinsic::getBaseName(format.intrinsics.at(op)).str() == expected);
    }
  }
}

TEST_CASE("formats can be looked up by name") {
  for (const auto &format : nrs::formats) {
    CHECK(nrs::get_format_from_name(format.name) == format.format);
    CHECK(&nrs::get_format_info(format.format) == &format);
  }
  CHECK_FALSE(nrs::get_format_from_name("ieee").has_value());
  CHECK_FALSE(nrs::get_format_from_name("Posit1").has_value());
}
