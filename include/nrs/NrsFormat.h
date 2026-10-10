// The NRS formats the pass can convert to, and everything the pass needs to
// know about each of them: its name, how NRSSL represents it, and which
// intrinsic implements which operation.

#pragma once

#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicsRISCV.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace nrs {

enum class Format : std::uint8_t {
  Posit1,
  Posit2,
  Morris,
  MorrisHeb,
  MorrisBiasHeb,
  MorrisUnaryHeb,
};

// Operations with an NRS intrinsic. The sign injection intrinsics (sgnj*) are
// left out on purpose: they work on the sign bit, which is not how negation
// works in every format (posit is two's complement).
enum class Op : std::uint8_t {
  Add,
  Sub,
  Mul,
  Div,
  Min,
  Max,
  Sqrt,
  Eq,
  Lt,
  Le,
  CvtToInt,
  CvtToUInt,
  CvtFromInt,
  CvtFromUInt,
};

inline constexpr std::size_t op_count = static_cast<std::size_t>(Op::CvtFromUInt) + 1;

struct FormatInfo {
  Format format;
  // Used for the pass parameter and in the intrinsic names.
  std::string_view name;
  // NRSSL class (JNI name) and the class of its values.
  std::string_view nrssl_class;
  std::string_view nrssl_value_class;
  // NRSSL's second apply() parameter for 32-bit values: the exponent size for
  // posit, the size of the G field for Morris. MorrisUnaryHEB has none.
  std::optional<int> nrssl_parameter;
  std::array<llvm::Intrinsic::ID, op_count> intrinsics;

  [[nodiscard]] constexpr llvm::Intrinsic::ID intrinsic(Op op) const {
    return intrinsics.at(static_cast<std::size_t>(op));
  }
};

// The intrinsic IDs are generated per format by the LLVM fork, so they can only
// be spelled out one by one. Same order as Op.
#define NRS_INTRINSICS(fmt)                                                                        \
  std::array<llvm::Intrinsic::ID, op_count> {                                                      \
    llvm::Intrinsic::riscv_nrssl_f32_##fmt##_add, llvm::Intrinsic::riscv_nrssl_f32_##fmt##_sub,    \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_mul,                                              \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_div,                                              \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_min,                                              \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_max,                                              \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_sqrt,                                             \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_eq, llvm::Intrinsic::riscv_nrssl_f32_##fmt##_lt,  \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_le,                                               \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_cvtToInt,                                         \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_cvtToUInt,                                        \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_cvtFromInt,                                       \
        llvm::Intrinsic::riscv_nrssl_f32_##fmt##_cvtFromUInt,                                      \
  }

inline constexpr std::array formats{
    FormatInfo{Format::Posit1, "posit1", "ro/upb/nrs/sl/Posit", "ro/upb/nrs/sl/Posit_B", 2,
               NRS_INTRINSICS(posit1)},
    FormatInfo{Format::Posit2, "posit2", "ro/upb/nrs/sl/Posit", "ro/upb/nrs/sl/Posit_B", 3,
               NRS_INTRINSICS(posit2)},
    FormatInfo{Format::Morris, "morris", "ro/upb/nrs/sl/Morris", "ro/upb/nrs/sl/Morris_B", 4,
               NRS_INTRINSICS(morris)},
    FormatInfo{Format::MorrisHeb, "morrisHeb", "ro/upb/nrs/sl/MorrisHEB",
               "ro/upb/nrs/sl/MorrisHEB_B", 4, NRS_INTRINSICS(morrisHeb)},
    FormatInfo{Format::MorrisBiasHeb, "morrisBiasHeb", "ro/upb/nrs/sl/MorrisBiasHEB",
               "ro/upb/nrs/sl/MorrisBiasHEB_B", 4, NRS_INTRINSICS(morrisBiasHeb)},
    FormatInfo{Format::MorrisUnaryHeb, "morrisUnaryHeb", "ro/upb/nrs/sl/MorrisUnaryHEB",
               "ro/upb/nrs/sl/MorrisUnaryHEB_B", std::nullopt, NRS_INTRINSICS(morrisUnaryHeb)},
};

#undef NRS_INTRINSICS

[[nodiscard]] constexpr const FormatInfo &info(Format format) {
  return formats.at(static_cast<std::size_t>(format));
}

[[nodiscard]] constexpr std::optional<Format> format_from_name(std::string_view name) {
  for (const auto &f : formats) {
    if (f.name == name) {
      return f.format;
    }
  }
  return std::nullopt;
}

// The table is indexed by Format, keep the two in the same order.
static_assert([] {
  for (std::size_t i = 0; i < formats.size(); ++i) {
    if (static_cast<std::size_t>(formats.at(i).format) != i) {
      return false;
    }
  }
  return true;
}());

} // namespace nrs
