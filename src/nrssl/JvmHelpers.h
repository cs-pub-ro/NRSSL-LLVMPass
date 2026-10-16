#pragma once

#include <concepts>
#include <string>
#include <string_view>

namespace nrs::nrssl {

template <typename T>
concept string_or_view =
    std::same_as<std::decay_t<T>, std::string> || std::same_as<std::decay_t<T>, std::string_view>;

template <typename... Args>
  requires(string_or_view<Args> && ...)
std::string concat(Args &&...args) {
  const std::size_t total_size = (args.size() + ... + 0);
  std::string result;
  result.reserve(total_size);
  (result.append(std::forward<Args>(args)), ...);
  return result;
}

namespace jvm_types {

inline constexpr std::string_view Object = "java/lang/Object";
inline constexpr std::string_view String = "java/lang/String";

} // namespace jvm_types

namespace jvm_signatures {

template <typename T>
  requires string_or_view<T>
std::string class_signature(T &&class_name) {
  return concat(std::string_view("L"), std::forward<T>(class_name), std::string_view(";"));
}

template <typename Ret, typename... Params>
  requires string_or_view<Ret> && (string_or_view<Params> && ...)
std::string method_signature(Ret &&return_type, Params &&...param_types) {
  return concat(std::string_view("("), std::forward<Params>(param_types)..., std::string_view(")"),
                std::forward<Ret>(return_type));
}

[[maybe_unused]] inline constexpr std::string_view void_t = "V";
[[maybe_unused]] inline constexpr std::string_view boolean_t = "Z";
[[maybe_unused]] inline constexpr std::string_view byte_t = "B";
[[maybe_unused]] inline constexpr std::string_view char_t = "C";
[[maybe_unused]] inline constexpr std::string_view short_t = "S";
[[maybe_unused]] inline constexpr std::string_view int_t = "I";
[[maybe_unused]] inline constexpr std::string_view long_t = "J";
[[maybe_unused]] inline constexpr std::string_view float_t = "F";
[[maybe_unused]] inline constexpr std::string_view double_t = "D";

[[maybe_unused]] inline constexpr std::string_view string_t = "Ljava/lang/String;";
[[maybe_unused]] inline constexpr std::string_view object_t = "Ljava/lang/Object;";

} // namespace jvm_signatures

} // namespace nrs::nrssl