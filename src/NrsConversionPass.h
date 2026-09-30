// The module pass that converts all f32 code and data to an NRS format.

#pragma once

#include "NrsFormat.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

#include <string>

namespace nrs {

class NrsConversionPass : public llvm::PassInfoMixin<NrsConversionPass> {
public:
  // jars_dir: where nrssl.jar and the Scala library are.
  NrsConversionPass(Format format, std::string jars_dir)
      : format_(format), jars_dir_(std::move(jars_dir)) {}

  // NOLINTNEXTLINE(readability-identifier-naming): names required by LLVM
  llvm::PreservedAnalyses run(llvm::Module &module, llvm::ModuleAnalysisManager &analyses);

  // Runs even at -O0 and on optnone functions, the result is wrong without it.
  // NOLINTNEXTLINE(readability-identifier-naming)
  static bool isRequired() { return true; }

  // Module flag that records the format, a second conversion would corrupt
  // the constants.
  static constexpr const char *format_flag = "nrs.format";

private:
  Format format_;
  std::string jars_dir_;
};

} // namespace nrs
