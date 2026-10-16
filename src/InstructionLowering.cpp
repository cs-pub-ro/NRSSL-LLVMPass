#include "InstructionLowering.h"

#include "ConstantConverter.h"
#include "nrs/NrsFormat.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/STLExtras.h>
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
const llvm::Type *get_fp_type(const llvm::Instruction &inst) {
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
  return contains_float(*call.getType()) || llvm::any_of(call.args(), [](const llvm::Value *arg) {
           return contains_float(*arg->getType());
         });
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
  if (const llvm::Type *type = get_fp_type(inst); type != nullptr && !is_f32(*type)) {
    return Lowering::KeptIeee;
  }
  return Lowering::Unchanged;
}

llvm::Error InstructionLowering::convert_constant_operands(llvm::Instruction &inst) {
  for (llvm::Use &use : inst.operands()) {
    auto *constant = llvm::dyn_cast<llvm::Constant>(use.get());
    // Globals were already converted separately
    if (constant == nullptr || llvm::isa<llvm::GlobalValue>(constant)) {
      continue;
    }
    auto converted = const_converter_->convert(*constant);
    if (!converted) {
      return converted.takeError();
    }
    if (*converted != constant) {
      use.set(*converted);
    }
  }
  return llvm::Error::success();
}

namespace {

// Emits calls to the NRS intrinsics of one format, in front of an instruction.
class Emitter {
public:
  Emitter(llvm::Instruction &inst, const FormatInfo &format, ConstantConverter &constants)
      : builder_(&inst), module_(inst.getModule()), format_(&format), const_converter_(&constants),
        context_(&inst.getContext()) {}

  llvm::Value *call(Op op, llvm::ArrayRef<llvm::Value *> args) {
    llvm::Function *intrinsic = llvm::Intrinsic::getDeclaration(module_, format_->intrinsic(op));
    return builder_.CreateCall(intrinsic, args);
  }

  // The compare intrinsics return 0 or 1 as i64.
  llvm::Value *compare(Op op, llvm::Value *first, llvm::Value *second) {
    return builder_.CreateTrunc(call(op, {first, second}), builder_.getInt1Ty());
  }

  // NRS zero as an f32 constant.
  llvm::Expected<llvm::Constant *> zero() { return const_converter_->zero_f32(*context_); }

  // 0 - x, the sign bit alone is not the sign in every format.
  llvm::Expected<llvm::Value *> negate(llvm::Value *x) {
    auto nrs_zero = zero();
    if (!nrs_zero) {
      return nrs_zero.takeError();
    }
    return call(Op::Sub, {*nrs_zero, x});
  }

  llvm::IRBuilder<> &builder() { return builder_; }

private:
  llvm::IRBuilder<> builder_;
  llvm::Module *module_;
  const FormatInfo *format_;
  ConstantConverter *const_converter_;
  llvm::LLVMContext *context_;
};

llvm::Expected<llvm::Value *> lower_arithmetic(llvm::Instruction &inst, Emitter &emit) {
  switch (inst.getOpcode()) {
  case llvm::Instruction::FAdd:
    return emit.call(Op::Add, {inst.getOperand(0), inst.getOperand(1)});
  case llvm::Instruction::FSub:
    return emit.call(Op::Sub, {inst.getOperand(0), inst.getOperand(1)});
  case llvm::Instruction::FMul:
    return emit.call(Op::Mul, {inst.getOperand(0), inst.getOperand(1)});
  case llvm::Instruction::FDiv:
    return emit.call(Op::Div, {inst.getOperand(0), inst.getOperand(1)});
  case llvm::Instruction::FNeg:
    return emit.negate(inst.getOperand(0));
  default:
    return unsupported("frem has no NRS equivalent");
  }
}

// There is no NaN in the NRS formats, ordered and unordered compares are the same.
llvm::Expected<llvm::Value *> lower_compare(llvm::FCmpInst &cmp, Emitter &emit) {
  llvm::Value *lhs = cmp.getOperand(0);
  llvm::Value *rhs = cmp.getOperand(1);
  switch (cmp.getPredicate()) {
  case llvm::FCmpInst::FCMP_OEQ:
  case llvm::FCmpInst::FCMP_UEQ:
    return emit.compare(Op::Eq, lhs, rhs);
  case llvm::FCmpInst::FCMP_ONE:
  case llvm::FCmpInst::FCMP_UNE:
    return emit.builder().CreateNot(emit.compare(Op::Eq, lhs, rhs));
  case llvm::FCmpInst::FCMP_OLT:
  case llvm::FCmpInst::FCMP_ULT:
    return emit.compare(Op::Lt, lhs, rhs);
  case llvm::FCmpInst::FCMP_OLE:
  case llvm::FCmpInst::FCMP_ULE:
    return emit.compare(Op::Le, lhs, rhs);
  // x > y is y < x, x >= y is y <= x
  case llvm::FCmpInst::FCMP_OGT:
  case llvm::FCmpInst::FCMP_UGT:
    return emit.compare(Op::Lt, rhs, lhs);
  case llvm::FCmpInst::FCMP_OGE:
  case llvm::FCmpInst::FCMP_UGE:
    return emit.compare(Op::Le, rhs, lhs);
  case llvm::FCmpInst::FCMP_TRUE:
    return emit.builder().getTrue();
  case llvm::FCmpInst::FCMP_FALSE:
    return emit.builder().getFalse();
  default:
    return unsupported("fcmp " + llvm::CmpInst::getPredicateName(cmp.getPredicate()) +
                       " (NaN check) has no NRS equivalent");
  }
}

// The conversion intrinsics work on i64.
llvm::Expected<llvm::Value *> lower_conversion(llvm::Instruction &inst, Emitter &emit) {
  llvm::IRBuilder<> &builder = emit.builder();
  llvm::Value *input = inst.getOperand(0);
  switch (inst.getOpcode()) {
  case llvm::Instruction::FPToSI:
    return builder.CreateSExtOrTrunc(emit.call(Op::CvtToInt, {input}), inst.getType());
  case llvm::Instruction::FPToUI:
    return builder.CreateZExtOrTrunc(emit.call(Op::CvtToUInt, {input}), inst.getType());
  default:
    break;
  }
  if (input->getType()->getIntegerBitWidth() > 64) {
    return unsupported("int to f32 conversions work on at most 64 bits");
  }
  if (inst.getOpcode() == llvm::Instruction::SIToFP) {
    return emit.call(Op::CvtFromInt, {builder.CreateSExt(input, builder.getInt64Ty())});
  }
  return emit.call(Op::CvtFromUInt, {builder.CreateZExt(input, builder.getInt64Ty())});
}

llvm::Expected<llvm::Value *> lower_intrinsic(llvm::IntrinsicInst &intrinsic, Emitter &emit) {
  switch (intrinsic.getIntrinsicID()) {
  case llvm::Intrinsic::minnum:
    return emit.call(Op::Min, {intrinsic.getArgOperand(0), intrinsic.getArgOperand(1)});
  case llvm::Intrinsic::maxnum:
    return emit.call(Op::Max, {intrinsic.getArgOperand(0), intrinsic.getArgOperand(1)});
  case llvm::Intrinsic::sqrt:
    return emit.call(Op::Sqrt, {intrinsic.getArgOperand(0)});
  case llvm::Intrinsic::fabs: {
    // |x| = x < 0 ? 0 - x : x
    llvm::Value *x = intrinsic.getArgOperand(0);
    auto nrs_zero = emit.zero();
    if (!nrs_zero) {
      return nrs_zero.takeError();
    }
    llvm::Value *negated = emit.call(Op::Sub, {*nrs_zero, x});
    return emit.builder().CreateSelect(emit.compare(Op::Lt, x, *nrs_zero), negated, x);
  }
  default:
    // our own intrinsics are fine, anything else would compute in IEEE
    if (intrinsic.getCalledFunction()->getName().starts_with("llvm.riscv.nrssl.")) {
      return nullptr;
    }
    return unsupported(intrinsic.getCalledFunction()->getName() + " has no NRS equivalent");
  }
}

} // namespace

// The value that replaces inst, nullptr if inst is not an f32 operation.
llvm::Expected<llvm::Value *> InstructionLowering::replacement(llvm::Instruction &inst) {
  const llvm::Type *type = get_fp_type(inst);
  if (type != nullptr && is_f32_vector(*type)) {
    return unsupported("vector f32 operations are not supported");
  }
  Emitter emit(inst, *format_, *const_converter_);

  switch (inst.getOpcode()) {
  case llvm::Instruction::FAdd:
  case llvm::Instruction::FSub:
  case llvm::Instruction::FMul:
  case llvm::Instruction::FDiv:
  case llvm::Instruction::FRem:
  case llvm::Instruction::FNeg:
    return is_f32(*type) ? lower_arithmetic(inst, emit) : nullptr;
  case llvm::Instruction::FCmp:
    return is_f32(*type) ? lower_compare(llvm::cast<llvm::FCmpInst>(inst), emit) : nullptr;
  case llvm::Instruction::FPToSI:
  case llvm::Instruction::FPToUI:
  case llvm::Instruction::SIToFP:
  case llvm::Instruction::UIToFP:
    return is_f32(*type) ? lower_conversion(inst, emit) : nullptr;
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
  return lower_intrinsic(*intrinsic, emit);
}

} // namespace nrs
