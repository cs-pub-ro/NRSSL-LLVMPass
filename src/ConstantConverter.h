// Converts the f32 values inside LLVM constants to the bits of an NRS format.

#pragma once

#include "nrssl/Encoder.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Constant.h>
#include <llvm/Support/Error.h>

namespace nrs {

class ConstantConverter {
public:
  explicit ConstantConverter(const nrssl::Encoder &encoder) : encoder_(&encoder) {}

  // The constant with every f32 value in it (also inside arrays, structs and
  // vectors) replaced by its NRS bits. Constants without f32 values, zeros and
  // double values come back unchanged.
  [[nodiscard]] llvm::Expected<llvm::Constant *> convert(llvm::Constant &constant);

  // NRS zero as an f32 constant.
  [[nodiscard]] llvm::Expected<llvm::Constant *> zero_f32(llvm::LLVMContext &context);

private:
  [[nodiscard]] llvm::Expected<llvm::Constant *> convert_uncached(llvm::Constant &constant);

  const nrssl::Encoder *encoder_;
  // Constants are uniqued, so every distinct value is encoded once.
  llvm::DenseMap<llvm::Constant *, llvm::Constant *> cache_;
};

// True if the type is f32 or an aggregate with f32 somewhere inside.
[[nodiscard]] bool contains_float(const llvm::Type &type);

} // namespace nrs
