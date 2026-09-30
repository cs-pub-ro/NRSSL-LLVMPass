// Converts between IEEE doubles and the bits of an NRS format, with NRSSL.

#pragma once

#include "NrsFormat.h"
#include "nrssl/Jvm.h"

#include <llvm/Support/Error.h>

#include <jni.h>

#include <cstdint>

namespace nrs::nrssl {

class Encoder {
public:
  // Values are 32 bits wide, the only width the pass supports.
  static constexpr int width = 32;

  // Looks up the NRSSL classes and methods of the format once.
  static llvm::Expected<Encoder> create(Format format, Jvm &jvm);

  // The bits of the format closest to value, rounded like NRSSL does by default.
  [[nodiscard]] llvm::Expected<std::uint64_t> encode(double value) const;

  // The value of the given bits.
  [[nodiscard]] llvm::Expected<double> decode(std::uint64_t bits) const;

  [[nodiscard]] const FormatInfo &format() const { return *info_; }

private:
  Encoder(const FormatInfo &info, Jvm &jvm) : info_(&info), jvm_(&jvm) {}

  // Calls the static apply(<value>, [parameter,] size, rounding) of the format.
  [[nodiscard]] jobject apply(JNIEnv &env, jmethodID method, jvalue value) const;

  const FormatInfo *info_;
  Jvm *jvm_;
  GlobalRef class_;
  GlobalRef rounding_;
  jmethodID apply_double_ = nullptr;
  jmethodID apply_string_ = nullptr;
  jmethodID to_binary_string_ = nullptr;
  jmethodID to_double_ = nullptr;
};

} // namespace nrs::nrssl
