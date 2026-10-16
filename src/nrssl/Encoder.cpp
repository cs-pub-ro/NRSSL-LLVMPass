#include "nrssl/Encoder.h"

#include "nrs/NrsFormat.h"
#include "nrssl/Jvm.h"
#include "nrssl/JvmHelpers.h"

#include <llvm/ADT/Twine.h>
#include <llvm/Support/Error.h>

#include <jni.h>

#include <array>
#include <bitset>
#include <charconv>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <system_error>

namespace nrs::nrssl {

namespace {

llvm::Error error(const llvm::Twine &message) {
  return llvm::createStringError(llvm::inconvertibleErrorCode(), message);
}

template <typename T>
std::string apply_signature(const FormatInfo &info, T &&input_type) {
  const std::string_view extra_param = info.nrs_apply_extra_param ? jvm_signatures::int_t : "";
  return jvm_signatures::method_signature(
      jvm_signatures::class_signature(info.nrs_value_class), std::forward<T>(input_type),
      jvm_signatures::int_t, extra_param, jvm_signatures::class_signature(rounding_type_class));
}

std::string to_binary_string(std::uint64_t bits) {
  return std::bitset<Encoder::width>(bits).to_string();
}

} // namespace

llvm::Expected<Encoder> Encoder::create(Format format, Jvm &jvm) {
  auto env_or_error = jvm.env();
  if (!env_or_error) {
    return env_or_error.takeError();
  }
  JNIEnv &env = *env_or_error;
  const LocalFrame frame(env);
  const FormatInfo &info = nrs::get_format_info(format);
  Encoder encoder(info, jvm);

  const std::string nrs_class(info.nrs_class);
  const std::string value_class(info.nrs_value_class);
  jclass cls = env.FindClass(nrs_class.c_str());
  jclass value_cls = env.FindClass(value_class.c_str());
  if (auto err = take_exception(env, llvm::Twine("NRSSL class ") + nrs_class)) {
    return std::move(err);
  }

  jmethodID default_rounding = env.GetStaticMethodID(
      cls, "default_rounding",
      jvm_signatures::method_signature(jvm_signatures::class_signature(rounding_type_class))
          .c_str());
  encoder.apply_double_ =
      env.GetStaticMethodID(cls, "apply", apply_signature(info, jvm_signatures::double_t).c_str());
  encoder.apply_string_ =
      env.GetStaticMethodID(cls, "apply", apply_signature(info, jvm_signatures::string_t).c_str());
  encoder.to_binary_string_ =
      env.GetMethodID(value_cls, "toBinaryString",
                      jvm_signatures::method_signature(jvm_signatures::string_t).c_str());
  encoder.to_double_ = env.GetMethodID(
      value_cls, "toDouble", jvm_signatures::method_signature(jvm_signatures::double_t).c_str());
  if (auto err = take_exception(env, llvm::Twine("NRSSL methods of ") + nrs_class)) {
    return std::move(err);
  }

  jobject rounding = env.CallStaticObjectMethodA(cls, default_rounding, nullptr);
  if (auto err = take_exception(env, llvm::Twine("NRSSL default rounding of ") + nrs_class)) {
    return std::move(err);
  }
  encoder.class_ = GlobalRef(jvm.vm(), env, cls);
  encoder.rounding_ = GlobalRef(jvm.vm(), env, rounding);
  return encoder;
}

jobject Encoder::apply(JNIEnv &env, jmethodID method, jvalue value) const {
  std::array<jvalue, 4> args{};
  std::size_t n = 0;
  args[n++] = value;
  if (info_->nrs_apply_extra_param) {
    args[n++].i = *info_->nrs_apply_extra_param;
  }
  args[n++].i = width;
  args[n].l = rounding_.get();
  return env.CallStaticObjectMethodA(class_.get_class(), method, args.data());
}

llvm::Expected<std::uint64_t> Encoder::encode(double value) const {
  auto env_or_error = jvm_->env();
  if (!env_or_error) {
    return env_or_error.takeError();
  }
  JNIEnv &env = *env_or_error;
  const LocalFrame frame(env);

  jvalue input{};
  input.d = value;
  jobject nrs_value = apply(env, apply_double_, input);
  jstring binary = nrs_value != nullptr
                       ? as_string(env.CallObjectMethodA(nrs_value, to_binary_string_, nullptr))
                       : nullptr;
  if (auto err = take_exception(env, llvm::Twine("encoding ") + std::to_string(value) + " as " +
                                         llvm::StringRef(info_->name))) {
    return std::move(err);
  }
  if (binary == nullptr) {
    return error(llvm::Twine("NRSSL returned nothing encoding ") + std::to_string(value));
  }

  const Utf8Chars chars(env, binary);
  const std::string_view digits = chars.view();
  std::uint64_t bits = 0;
  const auto [end, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), bits, 2);
  if (!chars.ok() || ec != std::errc{} || end != digits.data() + digits.size() ||
      digits.size() != static_cast<std::size_t>(width)) {
    return error("NRSSL returned a malformed bit string: " + llvm::Twine(digits));
  }
  return bits;
}

llvm::Expected<double> Encoder::decode(std::uint64_t bits) const {
  auto env_or_error = jvm_->env();
  if (!env_or_error) {
    return env_or_error.takeError();
  }
  JNIEnv &env = *env_or_error;
  const LocalFrame frame(env);

  const std::string binary = to_binary_string(bits);
  jvalue input{};
  input.l = env.NewStringUTF(binary.c_str());
  jobject nrs_value = input.l != nullptr ? apply(env, apply_string_, input) : nullptr;
  const double value =
      nrs_value != nullptr ? env.CallDoubleMethodA(nrs_value, to_double_, nullptr) : 0.0;
  if (auto err = take_exception(env, llvm::Twine("decoding ") + binary + " as " +
                                         llvm::StringRef(info_->name))) {
    return std::move(err);
  }
  if (nrs_value == nullptr) {
    return error(llvm::Twine("NRSSL returned nothing decoding ") + binary);
  }
  return value;
}

} // namespace nrs::nrssl
