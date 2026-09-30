# lit configuration for the NRS pass tests.
#
# %opt_nrs runs opt with the plugin loaded, e.g.
#   RUN: %opt_nrs -passes=ieee-to-posit --float_type=posit1 -S %s | FileCheck %s
# opt, FileCheck and not come from the LLVM build the pass is built against.

import os

import lit.formats

config.name = "NRS pass"
config.test_format = lit.formats.ShTest(execute_external=False)
config.suffixes = [".ll"]

config.environment["PATH"] = os.pathsep.join([config.llvm_tools_dir, config.environment.get("PATH", "")])

# The pass starts a JVM through JNI and finds the jars of the build itself.

config.substitutions.append(("%opt_nrs", "opt -load-pass-plugin={}".format(config.plugin)))
