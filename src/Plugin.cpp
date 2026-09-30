// Registers the pass as nrs-convert<format>, e.g.
//   opt -load-pass-plugin=libNRSConversionPass.so -passes='nrs-convert<posit1>'

#include "NrsConversionPass.h"
#include "NrsFormat.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdlib>
#include <string>

namespace {

constexpr llvm::StringRef pass_name = "nrs-convert";

llvm::Expected<nrs::Format> parse_format(llvm::StringRef parameter) {
  if (auto format = nrs::format_from_name(parameter)) {
    return *format;
  }
  std::string names;
  for (const auto &format : nrs::formats) {
    names += names.empty() ? "" : ", ";
    names += format.name;
  }
  return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                 "unknown NRS format '" + parameter + "', expected one of: " +
                                     names);
}

// NRSSL_JARS overrides the jars of the build.
std::string jars_dir() {
  const char *override = std::getenv("NRSSL_JARS");
  return override != nullptr ? override : NRS_DEFAULT_JARS_DIR;
}

bool add_pass(llvm::StringRef name, llvm::ModulePassManager &passes,
              llvm::ArrayRef<llvm::PassBuilder::PipelineElement> /*inner*/) {
  if (!llvm::PassBuilder::checkParametrizedPassName(name, pass_name)) {
    return false;
  }
  auto format = llvm::PassBuilder::parsePassParameters(parse_format, name, pass_name);
  if (!format) {
    llvm::errs() << pass_name << ": " << llvm::toString(format.takeError()) << "\n";
    return false;
  }
  passes.addPass(nrs::NrsConversionPass(*format, jars_dir()));
  return true;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming): entry point looked up by LLVM
extern "C" LLVM_ATTRIBUTE_WEAK llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "NRSConversion",
          .PluginVersion = LLVM_VERSION_STRING,
          .RegisterPassBuilderCallbacks = [](llvm::PassBuilder &builder) {
            builder.registerPipelineParsingCallback(add_pass);
          }};
}
