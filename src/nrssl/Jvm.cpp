#include "nrssl/Jvm.h"

#include <llvm/Support/Error.h>

#include <jni.h>

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace nrs::nrssl {

namespace {

llvm::Error error(const llvm::Twine &message) {
  return llvm::createStringError(llvm::inconvertibleErrorCode(), message);
}

// A JVM that is already running in the process, e.g. started by the host.
JavaVM *existing_vm() {
  JavaVM *vm = nullptr;
  jsize count = 0;
  if (JNI_GetCreatedJavaVMs(&vm, 1, &count) != JNI_OK || count == 0) {
    return nullptr;
  }
  return vm;
}

llvm::Expected<JavaVM &> start_vm(const std::string &jars_dir) {
  // JavaVMOption wants a mutable string
  std::string class_path =
      "-Djava.class.path=" + jars_dir + "/scala-library-2.13.8.jar:" + jars_dir + "/nrssl.jar";
  std::array<JavaVMOption, 1> options{};
  options[0].optionString = class_path.data();

  JavaVMInitArgs args{};
  args.version = JNI_VERSION_10;
  args.nOptions = static_cast<jint>(options.size());
  args.options = options.data();
  args.ignoreUnrecognized = JNI_FALSE;

  JavaVM *vm = nullptr;
  void *env = nullptr;
  if (const jint rc = JNI_CreateJavaVM(&vm, &env, &args); rc != JNI_OK) {
    return error("could not start the JVM for NRSSL (JNI error " + llvm::Twine(rc) + ")");
  }
  return *vm;
}

} // namespace

llvm::Expected<Jvm &> Jvm::get(const std::string &jars_dir) {
  static std::optional<Jvm> jvm;
  if (!jvm) {
    JavaVM *vm = existing_vm();
    if (vm == nullptr) {
      auto started = start_vm(jars_dir);
      if (!started) {
        return started.takeError();
      }
      vm = &*started;
    }
    jvm.emplace(Key{}, *vm, jars_dir);
  } else if (jvm->jars_dir_ != jars_dir) {
    return error("the NRSSL JVM already runs with the jars from " + jvm->jars_dir_ +
                 ", can't switch to " + jars_dir);
  }
  return *jvm;
}

llvm::Expected<JNIEnv &> Jvm::env() const {
  void *env = nullptr;
  if (vm_->GetEnv(&env, JNI_VERSION_10) == JNI_OK) {
    return *static_cast<JNIEnv *>(env);
  }
  if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
    return error("could not attach the thread to the NRSSL JVM");
  }
  return *static_cast<JNIEnv *>(env);
}

llvm::Error take_exception(JNIEnv &env, const llvm::Twine &context) {
  if (env.ExceptionCheck() == JNI_FALSE) {
    return llvm::Error::success();
  }
  const LocalFrame frame(env);
  jthrowable exception = env.ExceptionOccurred();
  env.ExceptionClear();

  std::string message = "unknown Java exception";
  jclass object_class = env.FindClass("java/lang/Object");
  jmethodID to_string = object_class != nullptr
                            ? env.GetMethodID(object_class, "toString", "()Ljava/lang/String;")
                            : nullptr;
  if (to_string != nullptr) {
    jstring text = as_string(env.CallObjectMethodA(exception, to_string, nullptr));
    if (text != nullptr) {
      const Utf8Chars chars(env, text);
      if (chars.ok()) {
        message = chars.view();
      }
    }
  }
  env.ExceptionClear();
  return error(context + ": " + message);
}

void GlobalRef::reset() {
  if (ref_ == nullptr) {
    return;
  }
  void *env = nullptr;
  if (vm_->GetEnv(&env, JNI_VERSION_10) == JNI_OK) {
    static_cast<JNIEnv *>(env)->DeleteGlobalRef(ref_);
  }
  ref_ = nullptr;
}

} // namespace nrs::nrssl
