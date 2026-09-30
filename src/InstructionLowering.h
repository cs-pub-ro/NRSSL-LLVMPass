// Replaces the f32 operations of single instructions with NRS intrinsics.

#pragma once

#include "ConstantConverter.h"
#include "NrsFormat.h"

#include <llvm/IR/Instruction.h>
#include <llvm/Support/Error.h>

#include <cstdint>

namespace nrs {

// What happened to an instruction.
enum class Lowering : std::uint8_t {
  // no f32 operation in it
  Unchanged,
  // replaced with NRS intrinsics (and erased)
  Replaced,
  // arithmetic on another floating point type (double, half, ...), stays IEEE
  KeptIeee,
  // call to a function outside the module that takes or returns f32, it
  // computes in IEEE unless it was converted too
  ExternalCall,
};

class InstructionLowering {
public:
  InstructionLowering(const FormatInfo &format, ConstantConverter &constants)
      : format_(&format), constants_(&constants) {}

  // Converts the f32 constants the instruction uses, then replaces it if it is
  // an f32 operation. Fails for f32 operations without an NRS equivalent and
  // for conversions between NRS f32 values and IEEE doubles.
  [[nodiscard]] llvm::Expected<Lowering> lower(llvm::Instruction &inst);

private:
  [[nodiscard]] llvm::Error convert_constant_operands(llvm::Instruction &inst);
  [[nodiscard]] llvm::Expected<llvm::Value *> replacement(llvm::Instruction &inst);

  const FormatInfo *format_;
  ConstantConverter *constants_;
};

} // namespace nrs
