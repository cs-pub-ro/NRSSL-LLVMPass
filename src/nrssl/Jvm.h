// The JVM that runs NRSSL, and small RAII helpers for JNI.

#pragma once

#include <llvm/ADT/Twine.h>
#include <llvm/Support/Error.h>

#include <jni.h>

#include <string>
#include <string_view>
#include <utility>

namespace nrs::nrssl {

// JNI allows one JVM per process and it can't be started again after
// DestroyJavaVM, so the JVM is created on first use and lives until the process
// exits.
class Jvm {
public:
  // The JVM with nrssl.jar and the Scala library from jars_dir on its class
  // path. Only the first call starts it, later calls must use the same
  // directory.
  static llvm::Expected<Jvm &> get(const std::string &jars_dir);

  // Only get() can create a Jvm (passkey).
  class Key {
    friend class Jvm;
    Key() = default;
  };
  Jvm(Key /*key*/, JavaVM &vm, std::string jars_dir) : vm_(&vm), jars_dir_(std::move(jars_dir)) {}

  // The JNI environment of the calling thread, attaching it if needed.
  [[nodiscard]] llvm::Expected<JNIEnv &> env() const;

  [[nodiscard]] JavaVM &vm() const { return *vm_; }

private:
  JavaVM *vm_;
  std::string jars_dir_;
};

// If a Java exception is pending, clears it and returns it as an error
// (with its toString()), otherwise success.
[[nodiscard]] llvm::Error take_exception(JNIEnv &env, const llvm::Twine &context);

// All local references created while a LocalFrame is alive are freed with it.
class LocalFrame {
public:
  static constexpr jint default_capacity = 16;

  explicit LocalFrame(JNIEnv &env, jint capacity = default_capacity)
      : env_(&env), ok_(env.PushLocalFrame(capacity) == JNI_OK) {}
  ~LocalFrame() {
    if (ok_) {
      env_->PopLocalFrame(nullptr);
    }
  }
  LocalFrame(const LocalFrame &) = delete;
  LocalFrame &operator=(const LocalFrame &) = delete;
  LocalFrame(LocalFrame &&) = delete;
  LocalFrame &operator=(LocalFrame &&) = delete;

  [[nodiscard]] bool ok() const { return ok_; }

private:
  JNIEnv *env_;
  bool ok_ = false;
};

// Owns a JNI global reference.
class GlobalRef {
public:
  GlobalRef() = default;
  GlobalRef(JavaVM &vm, JNIEnv &env, jobject local)
      : vm_(&vm), ref_(local != nullptr ? env.NewGlobalRef(local) : nullptr) {}
  ~GlobalRef() { reset(); }

  GlobalRef(const GlobalRef &) = delete;
  GlobalRef &operator=(const GlobalRef &) = delete;
  GlobalRef(GlobalRef &&other) noexcept
      : vm_(std::exchange(other.vm_, nullptr)), ref_(std::exchange(other.ref_, nullptr)) {}
  GlobalRef &operator=(GlobalRef &&other) noexcept {
    if (this != &other) {
      reset();
      vm_ = std::exchange(other.vm_, nullptr);
      ref_ = std::exchange(other.ref_, nullptr);
    }
    return *this;
  }

  [[nodiscard]] jobject get() const { return ref_; }
  [[nodiscard]] jclass get_class() const { return static_cast<jclass>(ref_); }
  [[nodiscard]] explicit operator bool() const { return ref_ != nullptr; }

private:
  void reset();

  JavaVM *vm_ = nullptr;
  jobject ref_ = nullptr;
};

// A JNI call returns jobject where the Java method returns a string.
[[nodiscard]] inline jstring as_string(jobject object) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): that's how JNI types work
  return static_cast<jstring>(object);
}

// The characters of a Java string, released when it goes out of scope.
class Utf8Chars {
public:
  Utf8Chars(JNIEnv &env, jstring string)
      : env_(&env), string_(string), chars_(env.GetStringUTFChars(string, nullptr)) {}
  ~Utf8Chars() {
    if (chars_ != nullptr) {
      env_->ReleaseStringUTFChars(string_, chars_);
    }
  }
  Utf8Chars(const Utf8Chars &) = delete;
  Utf8Chars &operator=(const Utf8Chars &) = delete;
  Utf8Chars(Utf8Chars &&) = delete;
  Utf8Chars &operator=(Utf8Chars &&) = delete;

  [[nodiscard]] bool ok() const { return chars_ != nullptr; }
  [[nodiscard]] std::string_view view() const { return chars_; }

private:
  JNIEnv *env_;
  jstring string_;
  const char *chars_;
};

} // namespace nrs::nrssl
