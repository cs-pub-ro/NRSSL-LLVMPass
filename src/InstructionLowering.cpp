#include "InstructionLowering.h"

#include "ConstantConverter.h"
#include "NrsFormat.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>

namespace nrs {

namespace {

llvm::Error unsupported(const llvm::Twine &what) {
  return llvm::createStringError(llvm::inconvertibleErrorCode(), what);
}

bool is_f32(const llvm::Type &type) { return type.isFloatTy(); }

bool is_f32_vector(const llvm::Type &type) {
  return type.isVectorTy() && type.getScalarType()->isFloatTy();
}

// The floating point type an instruction computes with, if any.
const llvm::Type *fp_type(const llvm::Instruction &inst) {
  switch (inst.getOpcode()) {
  case llvm::Instruction::FAdd:
  case llvm::Instruction::FSub:
  case llvm::Instruction::FMul:
  case llvm::Instruction::FDiv:
  case llvm::Instruction::FRem:
  case llvm::Instruction::FNeg:
  case llvm::Instruction::SIToFP:
  case llvm::Instruction::UIToFP:
    return inst.getType();
  case llvm::Instruction::FCmp:
  case llvm::Instruction::FPToSI:
  case llvm::Instruction::FPToUI:
    return inst.getOperand(0)->getType();
  default:
    break;
  }
  if (const auto *intrinsic = llvm::dyn_cast<llvm::IntrinsicInst>(&inst)) {
    if (intrinsic->getType()->isFloatingPointTy()) {
      return intrinsic->getType();
    }
  }
  return nullptr;
}

bool uses_f32(const llvm::CallBase &call) {
  if (contains_float(*call.getType())) {
    return true;
  }
  for (const llvm::Value *arg : call.args()) {
    if (contains_float(*arg->getType())) {
      return true;
    }
  }
  return false;
}

} // namespace

llvm::Expected<Lowering> InstructionLowering::lower(llvm::Instruction &inst) {
  if (auto err = convert_constant_operands(inst)) {
    return std::move(err);
  }

  auto value = replacement(inst);
  if (!value) {
    return value.takeError();
  }
  if (*value != nullptr) {
    inst.replaceAllUsesWith(*value);
    inst.eraseFromParent();
    return Lowering::Replaced;
  }

  if (const auto *call = llvm::dyn_cast<llvm::CallBase>(&inst)) {
    const llvm::Function *callee = call->getCalledFunction();
    const bool external = callee == nullptr || callee->isDeclaration();
    if (external && !call->isInlineAsm() && (callee == nullptr || !callee->isIntrinsic()) &&
        uses_f32(*call)) {
      return Lowering::ExternalCall;
    }
  }
  if (const llvm::Type *type = fp_type(inst); type != nullptr && !is_f32(*type)) {
    return Lowering::KeptIeee;
  }
  return Lowering::Unchanged;
}

llvm::Error InstructionLowering::convert_constant_operands(llvm::Instruction &inst) {
  for (llvm::Use &use : inst.operands()) {
    auto *constant = llvm::dyn_cast<llvm::Constant>(use.get());
    if (constant == nullptr || llvm::isa<llvm::GlobalValue>(constant)) {
      continue;
    }
    auto converted = constants_->convert(*constant);
    if (!converted) {
      return converted.takeError();
    }
    if (*converted != constant) {
      use.set(*converted);
    }
  }
  return llvm::Error::success();
}

// The value that replaces inst, nullptr if inst is not an f32 operation.
llvm::Expected<llvm::Value *> InstructionLowering::replacement(llvm::Instruction &inst) {
  const llvm::Type *type = fp_type(inst);
  if (type != nullptr && is_f32_vector(*type)) {
    return unsupported("vector f32 operations are not supported");
  }

  llvm::IRBuilder<> builder(&inst);
  llvm::Module &module = *inst.getModule();
  auto call = [&](Op op, llvm::ArrayRef<llvm::Value *> args) -> llvm::Value * {
    llvm::Function *intrinsic = llvm::Intrinsic::getDeclaration(&module, format_->intrinsic(op));
    return builder.CreateCall(intrinsic, args);
  };
  // the compare intrinsics return 0 or 1 as i64
  auto compare = [&](Op op, llvm::Value *lhs, llvm::Value *rhs) {
    return builder.CreateTrunc(call(op, {lhs, rhs}), builder.getInt1Ty());
  };
  auto zero = [&]() { return constants_->zero(inst.getContext()); };

  switch (inst.getOpcode()) {
  case llvm::Instruction::FAdd:
  case llvm::Instruction::FSub:
  case llvm::Instruction::FMul:
  case llvm::Instruction::FDiv: {
    if (!is_f32(*type)) {
      return nullptr;
    }
    const auto op = [&] {
      switch (inst.getOpcode()) {
      case llvm::Instruction::FAdd:
        return Op::Add;
      case llvm::Instruction::FSub:
        return Op::Sub;
      case llvm::Instruction::FMul:
        return Op::Mul;
      default:
        return Op::Div;
      }
    }();
    return call(op, {inst.getOperand(0), inst.getOperand(1)});
  }

  case llvm::Instruction::FRem:
    if (is_f32(*type)) {
      return unsupported("frem has no NRS equivalent");
    }
    return nullptr;

  case llvm::Instruction::FNeg: {
    if (!is_f32(*type)) {
      return nullptr;
    }
    // -x = 0 - x, the sign bit alone is not the sign in every format
    auto nrs_zero = zero();
    if (!nrs_zero) {
      return nrs_zero.takeError();
    }
    return call(Op::Sub, {*nrs_zero, inst.getOperand(0)});
  }

  case llvm::Instruction::FCmp: {
    if (!is_f32(*type)) {
      return nullptr;
    }
    // There is no NaN in the NRS formats, ordered and unordered compares are
    // the same.
    auto &cmp = llvm::cast<llvm::FCmpInst>(inst);
    llvm::Value *lhs = cmp.getOperand(0);
    llvm::Value *rhs = cmp.getOperand(1);
    switch (cmp.getPredicate()) {
    case llvm::FCmpInst::FCMP_OEQ:
    case llvm::FCmpInst::FCMP_UEQ:
      return compare(Op::Eq, lhs, rhs);
    case llvm::FCmpInst::FCMP_ONE:
    case llvm::FCmpInst::FCMP_UNE:
      return builder.CreateNot(compare(Op::Eq, lhs, rhs));
    case llvm::FCmpInst::FCMP_OLT:
    case llvm::FCmpInst::FCMP_ULT:
      return compare(Op::Lt, lhs, rhs);
    case llvm::FCmpInst::FCMP_OLE:
    case llvm::FCmpInst::FCMP_ULE:
      return compare(Op::Le, lhs, rhs);
    case llvm::FCmpInst::FCMP_OGT:
    case llvm::FCmpInst::FCMP_UGT:
      return compare(Op::Lt, rhs, lhs);
    case llvm::FCmpInst::FCMP_OGE:
    case llvm::FCmpInst::FCMP_UGE:
      return compare(Op::Le, rhs, lhs);
    case llvm::FCmpInst::FCMP_TRUE:
      return builder.getTrue();
    case llvm::FCmpInst::FCMP_FALSE:
      return builder.getFalse();
    default:
      return unsupported("fcmp " + llvm::CmpInst::getPredicateName(cmp.getPredicate()) +
                         " (NaN check) has no NRS equivalent");
    }
  }

  // the conversion intrinsics work on i64
  case llvm::Instruction::FPToSI:
  case llvm::Instruction::FPToUI: {
    if (!is_f32(*type)) {
      return nullptr;
    }
    const bool is_signed = inst.getOpcode() == llvm::Instruction::FPToSI;
    llvm::Value *result = call(is_signed ? Op::CvtToInt : Op::CvtToUInt, {inst.getOperand(0)});
    return is_signed ? builder.CreateSExtOrTrunc(result, inst.getType())
                     : builder.CreateZExtOrTrunc(result, inst.getType());
  }

  case llvm::Instruction::SIToFP:
  case llvm::Instruction::UIToFP: {
    if (!is_f32(*type)) {
      return nullptr;
    }
    llvm::Value *input = inst.getOperand(0);
    if (input->getType()->getIntegerBitWidth() > 64) {
      return unsupported("int to f32 conversions work on at most 64 bits");
    }
    const bool is_signed = inst.getOpcode() == llvm::Instruction::SIToFP;
    llvm::Value *wide = is_signed ? builder.CreateSExt(input, builder.getInt64Ty())
                                  : builder.CreateZExt(input, builder.getInt64Ty());
    return call(is_signed ? Op::CvtFromInt : Op::CvtFromUInt, {wide});
  }

  // NRS f32 bits reinterpreted as IEEE (or the other way around) are garbage.
  case llvm::Instruction::FPExt:
    if (is_f32(*inst.getOperand(0)->getType())) {
      return unsupported("fpext of an NRS f32 value to an IEEE type");
    }
    return nullptr;
  case llvm::Instruction::FPTrunc:
    if (is_f32(*inst.getType())) {
      return unsupported("fptrunc of an IEEE value to an NRS f32 value");
    }
    return nullptr;

  default:
    break;
  }

  auto *intrinsic = llvm::dyn_cast<llvm::IntrinsicInst>(&inst);
  if (intrinsic == nullptr || !uses_f32(*intrinsic)) {
    return nullptr;
  }
  switch (intrinsic->getIntrinsicID()) {
  case llvm::Intrinsic::minnum:
    return call(Op::Min, {intrinsic->getArgOperand(0), intrinsic->getArgOperand(1)});
  case llvm::Intrinsic::maxnum:
    return call(Op::Max, {intrinsic->getArgOperand(0), intrinsic->getArgOperand(1)});
  case llvm::Intrinsic::sqrt:
    return call(Op::Sqrt, {intrinsic->getArgOperand(0)});
  case llvm::Intrinsic::fabs: {
    // |x| = x < 0 ? 0 - x : x
    auto nrs_zero = zero();
    if (!nrs_zero) {
      return nrs_zero.takeError();
    }
    llvm::Value *x = intrinsic->getArgOperand(0);
    llvm::Value *negated = call(Op::Sub, {*nrs_zero, x});
    return builder.CreateSelect(compare(Op::Lt, x, *nrs_zero), negated, x);
  }
  default:
    // our own intrinsics are fine, anything else would compute in IEEE
    if (intrinsic->getCalledFunction()->getName().starts_with("llvm.riscv.nrssl.")) {
      return nullptr;
    }
    return unsupported(intrinsic->getCalledFunction()->getName() + " has no NRS equivalent");
  }
}

} // namespace nrs
