#include <cstdint>
#include <iostream>
#include <jni.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include "NrsFormat.h"
#include "nrssl/Encoder.h"
#include "nrssl/Jvm.h"

#include <cstdlib>
#include <optional>

using namespace llvm;

namespace {

cl::opt<nrs::Format> selected_nrs(
    "float_type", cl::desc("Choose the floating point representation"),
    cl::init(nrs::Format::Posit1),
    cl::values(clEnumValN(nrs::Format::Posit1, "posit1", "Posit1 representation"),
               clEnumValN(nrs::Format::Posit2, "posit2", "Posit2 representation"),
               clEnumValN(nrs::Format::Morris, "morris", "Morris representation"),
               clEnumValN(nrs::Format::MorrisHeb, "morrisHeb", "Morris HEB representation"),
               clEnumValN(nrs::Format::MorrisUnaryHeb, "morrisUnaryHeb",
                          "Morris unary HEB representation"),
               clEnumValN(nrs::Format::MorrisBiasHeb, "morrisBiasHeb",
                          "Morris bias HEB representation")));

class NRSIntrinsicSelector {
  public:
    explicit NRSIntrinsicSelector(nrs::Format nrs, Module &M) {
        const auto intrinsic_prefix = "llvm.riscv.nrssl.f32." + std::string(nrs::info(nrs).name) + ".";

        // Map binary/unary ops to their corresponding intrinsics
        {
            static constexpr std::array op_names{
                std::make_pair("add", static_cast<unsigned>(Instruction::FAdd)),
                std::make_pair("sub", static_cast<unsigned>(Instruction::FSub)),
                std::make_pair("mul", static_cast<unsigned>(Instruction::FMul)),
                std::make_pair("div", static_cast<unsigned>(Instruction::FDiv)),
                std::make_pair("cvtToInt", static_cast<unsigned>(Instruction::FPToSI)),
                std::make_pair("cvtToUInt", static_cast<unsigned>(Instruction::FPToUI)),
                std::make_pair("cvtFromInt", static_cast<unsigned>(Instruction::SIToFP)),
                std::make_pair("cvtFromUInt", static_cast<unsigned>(Instruction::UIToFP))};

            for (const auto &[op_name, op_opcode] : op_names) {
                const auto intrinsic_name = intrinsic_prefix + op_name;
                const auto intrinsic_id = Function::lookupIntrinsicID(intrinsic_name);
                std::cout << "Looking up intrinsic: " << intrinsic_name << std::endl;
                std::cout << "Intrinsic id: " << intrinsic_id << " for " << intrinsic_name
                          << std::endl;
                const auto intrinsic_func = Intrinsic::getDeclaration(&M, intrinsic_id);
                if (intrinsic_func) {
                    op_to_intrinsic_[op_opcode] = intrinsic_func;
                    std::cout << "Mapped operation " << op_name
                              << " to intrinsic function " << intrinsic_func->getName().str()
                              << "\n";
                } else {
                    std::cerr << "Warning: Intrinsic " << intrinsic_name
                              << " not found in LLVM. This operation will not be converted.\n";
                }
            }
        }

        // Map fcmp ops to their corresponding intrinsics
        {
            static constexpr std::array fcmp_names{std::make_pair("eq", FCmpOpcode::FCMP_EQ),
                                                   std::make_pair("lt", FCmpOpcode::FCMP_LT),
                                                   std::make_pair("le", FCmpOpcode::FCMP_LE)};

            for (const auto &[fcmp_name, fcmp_opcode] : fcmp_names) {
                const auto intrinsic_name = intrinsic_prefix + fcmp_name;
                const auto intrinsic_id = Function::lookupIntrinsicID(intrinsic_name);
                const auto intrinsic_func = Intrinsic::getDeclaration(&M, intrinsic_id);
                if (intrinsic_func) {
                    fcmp_to_intrinsic_[fcmp_opcode] = intrinsic_func;
                    std::cout << "Mapped fcmp predicate " << fcmp_name << " to intrinsic function "
                              << intrinsic_func->getName().str() << "\n";
                } else {
                    std::cerr << "Warning: Intrinsic " << intrinsic_name
                              << " not found in LLVM. This operation will not be converted.\n";
                }
            }
        }

        // Map intrinsics (special ops) to their corresponding intrinsics
        {
            static const std::array intrinsic_names{
                std::make_pair("min", Intrinsic::minnum),
                std::make_pair("max", Intrinsic::maxnum),
                std::make_pair("sqrt", Intrinsic::sqrt),
            };

            for (const auto &[intrinsic_name, src_intrinsic_id] : intrinsic_names) {
                const auto full_intrinsic_name = intrinsic_prefix + intrinsic_name;
                const auto target_intrinsic_id = Function::lookupIntrinsicID(full_intrinsic_name);
                const auto target_intrinsic_func =
                    Intrinsic::getDeclaration(&M, target_intrinsic_id);
                if (target_intrinsic_func) {
                    intrinsic_to_intrinsic_[src_intrinsic_id] = target_intrinsic_func;
                    std::cout << "Mapped intrinsic " << full_intrinsic_name
                              << " to intrinsic function " << target_intrinsic_func->getName().str()
                              << "\n";
                } else {
                    std::cerr << "Warning: Intrinsic " << full_intrinsic_name
                              << " not found in LLVM. This operation will not be converted.\n";
                }
            }
        }
    }

    Function *sub() const { return op_to_intrinsic_.at(Instruction::FSub); }
    Function *eq() const { return fcmp_to_intrinsic_.at(FCmpOpcode::FCMP_EQ); }
    Function *lt() const { return fcmp_to_intrinsic_.at(FCmpOpcode::FCMP_LT); }
    Function *le() const { return fcmp_to_intrinsic_.at(FCmpOpcode::FCMP_LE); }

    Function *get_intrinsic_replacement(const Instruction &I) const {
        if (I.isBinaryOp() || isa<CastInst>(I)) {
            auto it = op_to_intrinsic_.find(I.getOpcode());
            if (it != op_to_intrinsic_.end()) {
                return it->second;
            }
        } else if (const auto *FCmp = dyn_cast<FCmpInst>(&I)) {
            FCmpOpcode fcmp_opcode;
            switch (FCmp->getPredicate()) {
            case FCmpInst::FCMP_OEQ:
            case FCmpInst::FCMP_UEQ:
                fcmp_opcode = FCmpOpcode::FCMP_EQ;
                break;
            case FCmpInst::FCMP_OLT:
            case FCmpInst::FCMP_ULT:
                fcmp_opcode = FCmpOpcode::FCMP_LT;
                break;
            case FCmpInst::FCMP_OLE:
            case FCmpInst::FCMP_ULE:
                fcmp_opcode = FCmpOpcode::FCMP_LE;
                break;
            default:
                fcmp_opcode = FCmpOpcode::FCMP_INVALID;
                break;
            }
            auto it = fcmp_to_intrinsic_.find(fcmp_opcode);
            if (it != fcmp_to_intrinsic_.end()) {
                return it->second;
            }
        } else if (const auto *Call = dyn_cast<CallBase>(&I)) {
            Function *called_fun = Call->getCalledFunction();
            if (called_fun && called_fun->isIntrinsic()) {
                auto it = intrinsic_to_intrinsic_.find(called_fun->getIntrinsicID());
                if (it != intrinsic_to_intrinsic_.end()) {
                    return it->second;
                }
            }
        }
        return nullptr; // No replacement found
    }

  private:
    NRSIntrinsicSelector(const NRSIntrinsicSelector &) = delete;
    NRSIntrinsicSelector &operator=(const NRSIntrinsicSelector &) = delete;

    std::unordered_map<unsigned, Function *> op_to_intrinsic_{};
    enum class FCmpOpcode { FCMP_EQ, FCMP_LT, FCMP_LE, FCMP_INVALID };
    std::unordered_map<FCmpOpcode, Function *> fcmp_to_intrinsic_{};
    std::unordered_map<Intrinsic::ID, Function *> intrinsic_to_intrinsic_{};
};

} // namespace

class NRSConversionPass : public PassInfoMixin<NRSConversionPass> {

    // set up by run(), only valid while it runs
    const nrs::nrssl::Encoder *encoder = nullptr;

  public:
    NRSConversionPass() { std::cout << "Creating NRSConversionPass\n"; }

    ~NRSConversionPass() { std::cout << "Destroying NRSConversionPass\n"; }

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
        std::cout << "Running NRSConversionPass on " << M.getName().str() << "\n";

        bool Modified = false;

        // NRSSL_JARS overrides the jars of the build
        const char *jars = std::getenv("NRSSL_JARS");
        auto jvm = nrs::nrssl::Jvm::get(jars != nullptr ? jars : NRS_DEFAULT_JARS_DIR);
        if (!jvm)
            report_fatal_error(jvm.takeError());
        auto created = nrs::nrssl::Encoder::create(selected_nrs, *jvm);
        if (!created)
            report_fatal_error(created.takeError());
        encoder = &*created;

        for (auto &Global : M.globals()) {
            if (Global.hasInitializer()) {
                Constant *Initializer = Global.getInitializer();
                std::cout << "Processing global variable: " << Global.getName().str() << "\n";

                Constant *NewInitializer = processConstant(Initializer, M.getContext());
                if (NewInitializer != Initializer) {
                    Global.setInitializer(NewInitializer);
                    Modified = true;
                }
            }
        }

        NRSIntrinsicSelector intrinsic_selector(selected_nrs, M);

        for (auto &F : M) {
            for (auto &BB : F) {
                for (auto it = BB.begin(); it != BB.end();) {
                    Instruction &I = *it;
                    // Advance the iterator before potential erases
                    ++it;

                    std::cout << "Processing instruction: " << I.getOpcodeName() << "\n";
                    if (auto *Store = dyn_cast<StoreInst>(&I)) {
                        Value *Value = Store->getValueOperand();
                        if (auto *Const = dyn_cast<Constant>(Value)) {
                            Constant *NewConst = processConstant(Const, M.getContext());
                            if (NewConst != Const) {
                                Store->setOperand(0, NewConst);
                                Modified = true;
                            }
                        }
                        continue;
                    }

                    // float literals used directly by any instruction (x + 1.0f, f(2.0f), ret ...)
                    Modified |= convertFloatOperands(I, M.getContext());

                    // double (and everything else) is left alone, the intrinsics are f32 only
                    if (!isF32Op(I))
                        continue;

                    if (Value *expanded = expandUnsupported(I, intrinsic_selector, M.getContext())) {
                        std::cout << "Expanding instruction " << I.getOpcodeName() << "\n";
                        I.replaceAllUsesWith(expanded);
                        I.eraseFromParent();
                        Modified = true;
                    } else if (auto *intrinsic_replacement =
                                   intrinsic_selector.get_intrinsic_replacement(I)) {
                        IRBuilder builder(&I);

                        std::vector<Value *> operands =
                            [&I, arg_count = intrinsic_replacement->arg_size()] {
                                const auto operands = I.operands();
                                std::vector<Value *> operand_vec(
                                    operands.begin(), std::next(operands.begin(), arg_count));
                                return operand_vec;
                            }();

                        Value *new_node{};

                        switch (I.getOpcode()) {
                        case Instruction::FPToSI:
                            // the intrinsics return i64, fit it to the actual result type
                            new_node = builder.CreateCall(intrinsic_replacement, operands);
                            new_node = builder.CreateSExtOrTrunc(new_node, I.getType());
                            break;
                        case Instruction::FPToUI:
                            new_node = builder.CreateCall(intrinsic_replacement, operands);
                            new_node = builder.CreateZExtOrTrunc(new_node, I.getType());
                            break;

                        case Instruction::SIToFP:
                            // the intrinsics take an i64 operand
                            operands[0] = builder.CreateSExtOrTrunc(operands[0], builder.getInt64Ty());
                            new_node = builder.CreateCall(intrinsic_replacement, operands);
                            break;
                        case Instruction::UIToFP:
                            operands[0] = builder.CreateZExtOrTrunc(operands[0], builder.getInt64Ty());
                            new_node = builder.CreateCall(intrinsic_replacement, operands);
                            break;

                        case Instruction::FCmp: {
                            // For FCmp, we need to convert the result to i1 (aka bool)
                            new_node = builder.CreateCall(intrinsic_replacement, operands);
                            new_node = builder.CreateTrunc(new_node, builder.getInt1Ty());
                            break;
                        }

                        default:
                            new_node = builder.CreateCall(intrinsic_replacement, operands);
                            break;
                        }

                        std::cout << "Replacing instruction " << I.getOpcodeName()
                                  << " with intrinsic: " << intrinsic_replacement->getName().str()
                                  << "\n";

                        I.replaceAllUsesWith(new_node);
                        I.eraseFromParent();
                        Modified = true;
                    }
                }
            }
        }

        return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }

    static bool isRequired() { return true; }


  private:
    // True if I computes on f32 values, i.e. is something the intrinsics can replace.
    static bool isF32Op(const Instruction &I) {
        if (isa<FPToSIInst>(I) || isa<FPToUIInst>(I) || isa<FCmpInst>(I))
            return I.getOperand(0)->getType()->isFloatTy();
        return I.getType()->isFloatTy();
    }

    bool convertFloatOperands(Instruction &I, LLVMContext &Context) {
        bool changed = false;
        for (Use &U : I.operands()) {
            auto *C = dyn_cast<ConstantFP>(U.get());
            if (C && C->getType()->isFloatTy()) {
                U.set(convertConstantFP(C, Context));
                changed = true;
            }
        }
        return changed;
    }

    // Ops without a matching intrinsic, built from the ones we have:
    //   fneg x      -> 0 - x
    //   fabs x      -> x < 0 ? 0 - x : x
    //   a > b       -> b < a,  a >= b -> b <= a,  a != b -> !(a == b)
    // Returns nullptr if I is not one of them.
    Value *expandUnsupported(Instruction &I, const NRSIntrinsicSelector &sel, LLVMContext &Context) {
        IRBuilder<> b(&I);
        auto zero = [&] {
            return convertConstantFP(cast<ConstantFP>(ConstantFP::get(Type::getFloatTy(Context), 0.0)), Context);
        };
        auto cmp = [&](Function *f, Value *x, Value *y) {
            return b.CreateTrunc(b.CreateCall(f, {x, y}), b.getInt1Ty());
        };

        if (I.getOpcode() == Instruction::FNeg)
            return b.CreateCall(sel.sub(), {zero(), I.getOperand(0)});

        if (auto *Call = dyn_cast<CallBase>(&I)) {
            Function *callee = Call->getCalledFunction();
            if (callee && callee->getIntrinsicID() == Intrinsic::fabs) {
                Value *x = Call->getArgOperand(0);
                Value *neg = b.CreateCall(sel.sub(), {zero(), x});
                return b.CreateSelect(cmp(sel.lt(), x, zero()), neg, x);
            }
            return nullptr;
        }

        auto *FCmp = dyn_cast<FCmpInst>(&I);
        if (!FCmp)
            return nullptr;
        Value *x = FCmp->getOperand(0), *y = FCmp->getOperand(1);
        switch (FCmp->getPredicate()) {
        case FCmpInst::FCMP_OGT:
        case FCmpInst::FCMP_UGT:
            return cmp(sel.lt(), y, x);
        case FCmpInst::FCMP_OGE:
        case FCmpInst::FCMP_UGE:
            return cmp(sel.le(), y, x);
        case FCmpInst::FCMP_ONE:
        case FCmpInst::FCMP_UNE:
            return b.CreateNot(cmp(sel.eq(), x, y));
        default:
            return nullptr;
        }
    }

    Constant *convertConstantFP(ConstantFP *FpConst, LLVMContext &Context) {
        // only f32 is converted, double stays IEEE
        if (!FpConst->getType()->isFloatTy())
            return FpConst;

        auto bits = encoder->encode(FpConst->getValueAPF().convertToFloat());
        if (!bits)
            report_fatal_error(bits.takeError());
        APFloat NewAPF(APFloat::IEEEsingle(), APInt(32, *bits));
        return ConstantFP::get(Context, NewAPF);
    }

    Constant *processConstant(Constant *C, LLVMContext &Context) {

        std::cout << "Processing constant" << "\n";

        if (ConstantFP *FpConst = dyn_cast<ConstantFP>(C)) {
            return convertConstantFP(FpConst, Context);
        }

        if (ConstantDataArray *DataArrayConst = dyn_cast<ConstantDataArray>(C)) {
            std::cout << "Processing ConstantDataArray with " << DataArrayConst->getNumElements()
                      << " elements\n";
            std::vector<Constant *> NewElements;
            for (int i = 0; i < DataArrayConst->getNumElements(); ++i) {
                Constant *Elem = DataArrayConst->getElementAsConstant(i);
                Constant *NewElem = processConstant(Elem, Context);
                if (NewElem != Elem) {
                    NewElements.push_back(NewElem);
                } else {
                    NewElements.push_back(Elem);
                }
            }
            return ConstantArray::get(DataArrayConst->getType(), NewElements);
        }

        if (ConstantArray *ArrConst = dyn_cast<ConstantArray>(C)) {
            std::cout << "Processing ConstantArray with " << ArrConst->getNumOperands()
                      << " elements\n";
            std::vector<Constant *> NewElements;
            for (int i = 0; i < ArrConst->getNumOperands(); ++i) {
                Constant *Elem = ArrConst->getOperand(i);
                Constant *NewElem = processConstant(Elem, Context);
                if (NewElem != Elem) {
                    NewElements.push_back(NewElem);
                } else {
                    NewElements.push_back(Elem);
                }
            }
            return ConstantArray::get(ArrConst->getType(), NewElements);
        }

        return C;
    }
};

PassPluginLibraryInfo getNRSConversionPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "NRSConversionPass", LLVM_VERSION_STRING, [](PassBuilder &PB) {
                // For clang integration - adds the pass at the start of the pipeline
                PB.registerPipelineStartEPCallback(
                    [](ModulePassManager &MPM, llvm::OptimizationLevel Level) {
                        std::cout << "Starting pipeline for NRSConversionPass\n";
                        MPM.addPass(NRSConversionPass());
                    });

                // For opt integration - allows the pass to be found by name
                PB.registerPipelineParsingCallback([](StringRef Name, ModulePassManager &MPM,
                                                      ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "ieee-to-posit") {
                        std::cout << "Adding NRSConversionPass to pipeline\n";
                        MPM.addPass(NRSConversionPass());
                        return true;
                    }
                    return false;
                });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getNRSConversionPassPluginInfo();
}
