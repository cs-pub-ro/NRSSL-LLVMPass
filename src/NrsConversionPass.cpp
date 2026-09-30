#include "NrsConversionPass.h"

#include "ConstantConverter.h"
#include "InstructionLowering.h"
#include "NrsFormat.h"
#include "nrssl/Encoder.h"
#include "nrssl/Jvm.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Error.h>

#define DEBUG_TYPE "nrs-convert"

namespace nrs {

namespace {

void diagnose(const llvm::Instruction &inst, const llvm::Twine &message,
              llvm::DiagnosticSeverity severity) {
  inst.getContext().diagnose(llvm::DiagnosticInfoUnsupported(
      *inst.getFunction(), message, inst.getDebugLoc(), severity));
}

// Errors that are not about one instruction (JVM, NRSSL, the module flag).
void module_error(llvm::Module &module, const llvm::Twine &message) {
  module.getContext().emitError("nrs-convert: " + message);
}

} // namespace

llvm::PreservedAnalyses NrsConversionPass::run(llvm::Module &module,
                                               llvm::ModuleAnalysisManager & /*analyses*/) {
  const FormatInfo &format = info(format_);

  if (const auto *flag = llvm::dyn_cast_or_null<llvm::MDString>(module.getModuleFlag(format_flag))) {
    module_error(module, "the module was already converted to " + flag->getString());
    return llvm::PreservedAnalyses::all();
  }

  auto jvm = nrssl::Jvm::get(jars_dir_);
  if (!jvm) {
    module_error(module, llvm::toString(jvm.takeError()));
    return llvm::PreservedAnalyses::all();
  }
  auto encoder = nrssl::Encoder::create(format_, *jvm);
  if (!encoder) {
    module_error(module, llvm::toString(encoder.takeError()));
    return llvm::PreservedAnalyses::all();
  }
  ConstantConverter constants(*encoder);
  InstructionLowering lowering(format, constants);

  for (llvm::GlobalVariable &global : module.globals()) {
    if (!global.hasInitializer()) {
      continue;
    }
    auto converted = constants.convert(*global.getInitializer());
    if (!converted) {
      module_error(module, "initializer of @" + global.getName() + ": " +
                               llvm::toString(converted.takeError()));
      return llvm::PreservedAnalyses::none();
    }
    global.setInitializer(*converted);
  }

  for (llvm::Function &function : module) {
    bool warned_ieee = false;
    for (llvm::Instruction &inst : llvm::make_early_inc_range(llvm::instructions(function))) {
      auto result = lowering.lower(inst);
      if (!result) {
        diagnose(inst, llvm::toString(result.takeError()), llvm::DS_Error);
        continue;
      }
      switch (*result) {
      case Lowering::KeptIeee:
        if (!warned_ieee) {
          diagnose(inst, "floating point code that is not f32 stays IEEE", llvm::DS_Warning);
          warned_ieee = true;
        }
        break;
      case Lowering::ExternalCall:
        diagnose(inst, "the called function takes or returns f32 but is not in this module, "
                       "it computes in IEEE unless it was converted too",
                 llvm::DS_Warning);
        break;
      case Lowering::Replaced:
        LLVM_DEBUG(llvm::dbgs() << "nrs-convert: replaced an instruction in " << function.getName()
                                << "\n");
        break;
      case Lowering::Unchanged:
        break;
      }
    }
  }

  module.addModuleFlag(llvm::Module::Error, format_flag,
                       llvm::MDString::get(module.getContext(), format.name));
  return llvm::PreservedAnalyses::none();
}

} // namespace nrs
