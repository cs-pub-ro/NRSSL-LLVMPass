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

  NumOps,
};

inline constexpr std::size_t op_count = static_cast<std::size_t>(Op::NumOps);

struct FormatInfo {
  Format format;
  // Used for the pass parameter and in the intrinsic names.
  std::string_view name;
  // NRSSL class (the derived factory class) and the class of its values (the base class that gets
  // returned). Both hold the JVM names of the classes, e.g. "ro/upb/nrs/sl/Posit" and
  // "ro/upb/nrs/sl/Posit_B".
  std::string_view nrs_class;
  std::string_view nrs_value_class;
  // NRSSL's second apply() parameter for 32-bit values: the exponent size for
  // posit, the size of the G field for Morris. MorrisUnaryHEB has none.
  std::optional<int> nrs_apply_extra_param;
  std::array<llvm::Intrinsic::ID, op_count> intrinsics;

  [[nodiscard]] constexpr llvm::Intrinsic::ID intrinsic(Op op) const {
    return intrinsics.at(static_cast<std::size_t>(op));
  }
};

// The intrinsic IDs are generated per format by the LLVM fork, so they can only
// be spelled out one by one. Same order as Op.
// TODO: Replace `nrssl` with `nrs` in the intrinsic names once the LLVM fork is updated.
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
    FormatInfo{
        .format = Format::Posit1,
        .name = "posit1",
        .nrs_class = "ro/upb/nrs/sl/Posit",
        .nrs_value_class = "ro/upb/nrs/sl/Posit_B",
        .nrs_apply_extra_param = 2,
        .intrinsics = NRS_INTRINSICS(posit1),
    },
    FormatInfo{
        .format = Format::Posit2,
        .name = "posit2",
        .nrs_class = "ro/upb/nrs/sl/Posit",
        .nrs_value_class = "ro/upb/nrs/sl/Posit_B",
        .nrs_apply_extra_param = 3,
        .intrinsics = NRS_INTRINSICS(posit2),
    },
    FormatInfo{
        .format = Format::Morris,
        .name = "morris",
        .nrs_class = "ro/upb/nrs/sl/Morris",
        .nrs_value_class = "ro/upb/nrs/sl/Morris_B",
        .nrs_apply_extra_param = 4,
        .intrinsics = NRS_INTRINSICS(morris),
    },
    FormatInfo{
        .format = Format::MorrisHeb,
        .name = "morrisHeb",
        .nrs_class = "ro/upb/nrs/sl/MorrisHEB",
        .nrs_value_class = "ro/upb/nrs/sl/MorrisHEB_B",
        .nrs_apply_extra_param = 4,
        .intrinsics = NRS_INTRINSICS(morrisHeb),
    },
    FormatInfo{
        .format = Format::MorrisBiasHeb,
        .name = "morrisBiasHeb",
        .nrs_class = "ro/upb/nrs/sl/MorrisBiasHEB",
        .nrs_value_class = "ro/upb/nrs/sl/MorrisBiasHEB_B",
        .nrs_apply_extra_param = 4,
        .intrinsics = NRS_INTRINSICS(morrisBiasHeb),
    },
    FormatInfo{
        .format = Format::MorrisUnaryHeb,
        .name = "morrisUnaryHeb",
        .nrs_class = "ro/upb/nrs/sl/MorrisUnaryHEB",
        .nrs_value_class = "ro/upb/nrs/sl/MorrisUnaryHEB_B",
        .nrs_apply_extra_param = std::nullopt,
        .intrinsics = NRS_INTRINSICS(morrisUnaryHeb),
    },
};

#undef NRS_INTRINSICS

inline constexpr std::string_view rounding_type_class = "ro/upb/nrs/sl/RoundingType";

[[nodiscard]] constexpr const FormatInfo &get_format_info(Format format) {
  return formats.at(static_cast<std::size_t>(format));
}

[[nodiscard]] constexpr std::optional<Format> get_format_from_name(std::string_view name) {
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
    if (static_cast<std::size_t>(formats[i].format) != i) {
      return false;
    }
  }
  return true;
}());

} // namespace nrs
