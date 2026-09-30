#include "ConstantConverter.h"

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Error.h>

namespace nrs {

bool contains_float(const llvm::Type &type) {
  if (type.isFloatTy()) {
    return true;
  }
  for (const llvm::Type *element : type.subtypes()) {
    if (contains_float(*element)) {
      return true;
    }
  }
  return false;
}

llvm::Expected<llvm::Constant *> ConstantConverter::convert(llvm::Constant &constant) {
  if (!contains_float(*constant.getType())) {
    return &constant;
  }
  if (const auto cached = cache_.find(&constant); cached != cache_.end()) {
    return cached->second;
  }
  auto converted = convert_uncached(constant);
  if (converted) {
    cache_.try_emplace(&constant, *converted);
  }
  return converted;
}

llvm::Expected<llvm::Constant *> ConstantConverter::zero(llvm::LLVMContext &context) {
  return convert(*llvm::ConstantFP::get(llvm::Type::getFloatTy(context), 0.0));
}

llvm::Expected<llvm::Constant *> ConstantConverter::convert_uncached(llvm::Constant &constant) {
  if (auto *fp = llvm::dyn_cast<llvm::ConstantFP>(&constant)) {
    auto bits = encoder_->encode(static_cast<double>(fp->getValueAPF().convertToFloat()));
    if (!bits) {
      return bits.takeError();
    }
    const llvm::APFloat value(llvm::APFloat::IEEEsingle(), llvm::APInt(nrssl::Encoder::width, *bits));
    return llvm::ConstantFP::get(constant.getContext(), value);
  }

  // Zeros are all-zero bits in every format, undef and poison stay what they are.
  if (constant.isNullValue() || llvm::isa<llvm::UndefValue>(constant)) {
    return &constant;
  }

  // Arrays, structs and vectors: convert the elements and rebuild.
  const bool is_aggregate =
      llvm::isa<llvm::ConstantAggregate>(constant) || llvm::isa<llvm::ConstantDataSequential>(constant);
  if (!is_aggregate) {
    // e.g. a constant expression: nothing to convert in it as a value
    return &constant;
  }

  llvm::SmallVector<llvm::Constant *, 16> elements;
  for (unsigned i = 0; llvm::Constant *element = constant.getAggregateElement(i); ++i) {
    auto converted = convert(*element);
    if (!converted) {
      return converted.takeError();
    }
    elements.push_back(*converted);
  }

  llvm::Type *type = constant.getType();
  if (auto *array = llvm::dyn_cast<llvm::ArrayType>(type)) {
    return llvm::ConstantArray::get(array, elements);
  }
  if (auto *structure = llvm::dyn_cast<llvm::StructType>(type)) {
    return llvm::ConstantStruct::get(structure, elements);
  }
  return llvm::ConstantVector::get(elements);
}

} // namespace nrs
