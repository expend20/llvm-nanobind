/**
 * Control Flow Flattening Tool
 *
 * Flattens control flow by converting all basic blocks into a switch-based
 * dispatcher pattern, making control flow analysis much harder.
 *
 * Implements control flow flattening with optional SipHash and opaque predicates.
 *
 * Usage:
 *   control_flow_flatten [options] <input.bc> <output.bc>
 *
 * Options:
 *   --iterations N              Number of iterations (default: 1)
 *   --use-func-resolver N       Percent chance to use function resolver (default: 0)
 *   --use-global-state N        Percent chance to use global variables for state (default: 0)
 *   --use-opaque N              Percent chance to use opaque predicates (default: 0)
 *   --use-global-opaque N       Percent chance to use globals in opaques (default: 0)
 *   --use-siphash N             Percent chance to use SipHash state transform (default: 0)
 *   --clone-siphash N           Percent chance to clone SipHash function (default: 0)
 *   --seed N                    Random seed for reproducibility (default: random)
 *   --help                      Show this help
 */

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Linker/Linker.h>

#include <random>
#include <vector>
#include <map>
#include <set>

using namespace llvm;

// Command line options
static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("<input bitcode>"),
                                          cl::Required);

static cl::opt<std::string> OutputFilename(cl::Positional,
                                           cl::desc("<output bitcode>"),
                                           cl::Required);

static cl::opt<int> Iterations("iterations",
                               cl::desc("Number of iterations (default: 1)"),
                               cl::init(1));

static cl::opt<int> UseFuncResolverChance("use-func-resolver",
                                          cl::desc("Percent chance to use function resolver (default: 0)"),
                                          cl::init(0));

static cl::opt<int> UseGlobalStateChance("use-global-state",
                                         cl::desc("Percent chance to use global state vars (default: 0)"),
                                         cl::init(0));

static cl::opt<int> UseOpaqueChance("use-opaque",
                                    cl::desc("Percent chance to use opaque predicates (default: 0)"),
                                    cl::init(0));

static cl::opt<int> UseGlobalOpaqueChance("use-global-opaque",
                                          cl::desc("Percent chance to use globals in opaques (default: 0)"),
                                          cl::init(0));

static cl::opt<int> UseSipHashChance("use-siphash",
                                     cl::desc("Percent chance to use SipHash state transform (default: 0)"),
                                     cl::init(0));

static cl::opt<int> CloneSipHashChance("clone-siphash",
                                       cl::desc("Percent chance to clone SipHash function (default: 0)"),
                                       cl::init(0));

static cl::opt<unsigned> Seed("seed",
                              cl::desc("Random seed (default: random)"),
                              cl::init(0));

// Transformation options struct
struct TransformationOptions {
    int useFunctionResolverChance;
    int useGlobalStateVariablesChance;
    int useOpaqueTransformationChance;
    int useGlobalVariableOpaquesChance;
    int useSipHashedStateChance;
    int cloneSipHashChance;
};

// Random number generator
class Random {
public:
    Random(unsigned seed) : rng_(seed ? seed : std::random_device{}()) {}

    template <typename T>
    T intRanged(T min, T max) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(rng_);
    }

    bool chance(int percent) {
        return intRanged(1, 100) <= percent;
    }

    uint64_t uint64() { return intRanged<uint64_t>(0, UINT64_MAX); }
    uint32_t uint32() { return intRanged<uint32_t>(0, UINT32_MAX); }

private:
    std::mt19937_64 rng_;
};

static Random* RNG = nullptr;
static Function* sipHashFn = nullptr;

// SipHash LLVM IR - this gets parsed and linked into the module
// Note: Uses external linkage initially so linker doesn't drop it; we change to internal after linking
static const char* SipHashLlvmIR = R"(
define i64 @___siphash(i64 noundef %0, i64 noundef %1, i64 noundef %2, i64 noundef %3, i64 noundef %4, i64 noundef %5, i64 noundef %6) {
  %8 = xor i64 %6, %2
  %9 = xor i64 %5, %1
  %10 = xor i64 %4, %2
  %11 = xor i64 %3, %1
  %12 = xor i64 %8, %0
  br label %13

13:
  %14 = phi i64 [ %11, %7 ], [ %26, %13 ]
  %15 = phi i64 [ %10, %7 ], [ %31, %13 ]
  %16 = phi i64 [ %9, %7 ], [ %32, %13 ]
  %17 = phi i1 [ true, %7 ], [ false, %13 ]
  %18 = phi i64 [ %12, %7 ], [ %28, %13 ]
  %19 = add i64 %14, %15
  %20 = tail call i64 @llvm.fshl.i64(i64 %15, i64 %15, i64 13)
  %21 = xor i64 %19, %20
  %22 = tail call i64 @llvm.fshl.i64(i64 %19, i64 %19, i64 32)
  %23 = add i64 %16, %18
  %24 = tail call i64 @llvm.fshl.i64(i64 %18, i64 %18, i64 16)
  %25 = xor i64 %23, %24
  %26 = add i64 %22, %25
  %27 = tail call i64 @llvm.fshl.i64(i64 %25, i64 %25, i64 21)
  %28 = xor i64 %26, %27
  %29 = add i64 %21, %23
  %30 = tail call i64 @llvm.fshl.i64(i64 %21, i64 %21, i64 17)
  %31 = xor i64 %30, %29
  %32 = tail call i64 @llvm.fshl.i64(i64 %29, i64 %29, i64 32)
  br i1 %17, label %13, label %33

33:
  %34 = xor i64 %26, %0
  %35 = xor i64 %28, 576460752303423488
  br label %36

36:
  %37 = phi i64 [ %34, %33 ], [ %49, %36 ]
  %38 = phi i64 [ %31, %33 ], [ %54, %36 ]
  %39 = phi i64 [ %32, %33 ], [ %55, %36 ]
  %40 = phi i1 [ true, %33 ], [ false, %36 ]
  %41 = phi i64 [ %35, %33 ], [ %51, %36 ]
  %42 = add i64 %37, %38
  %43 = tail call i64 @llvm.fshl.i64(i64 %38, i64 %38, i64 13)
  %44 = xor i64 %42, %43
  %45 = tail call i64 @llvm.fshl.i64(i64 %42, i64 %42, i64 32)
  %46 = add i64 %39, %41
  %47 = tail call i64 @llvm.fshl.i64(i64 %41, i64 %41, i64 16)
  %48 = xor i64 %46, %47
  %49 = add i64 %45, %48
  %50 = tail call i64 @llvm.fshl.i64(i64 %48, i64 %48, i64 21)
  %51 = xor i64 %49, %50
  %52 = add i64 %44, %46
  %53 = tail call i64 @llvm.fshl.i64(i64 %44, i64 %44, i64 17)
  %54 = xor i64 %53, %52
  %55 = tail call i64 @llvm.fshl.i64(i64 %52, i64 %52, i64 32)
  br i1 %40, label %36, label %56

56:
  %57 = xor i64 %49, 576460752303423488
  %58 = xor i64 %55, 255
  br label %59

59:
  %60 = phi i64 [ %57, %56 ], [ %72, %59 ]
  %61 = phi i64 [ %54, %56 ], [ %77, %59 ]
  %62 = phi i64 [ %58, %56 ], [ %78, %59 ]
  %63 = phi i32 [ 0, %56 ], [ %79, %59 ]
  %64 = phi i64 [ %51, %56 ], [ %74, %59 ]
  %65 = add i64 %60, %61
  %66 = tail call i64 @llvm.fshl.i64(i64 %61, i64 %61, i64 13)
  %67 = xor i64 %65, %66
  %68 = tail call i64 @llvm.fshl.i64(i64 %65, i64 %65, i64 32)
  %69 = add i64 %62, %64
  %70 = tail call i64 @llvm.fshl.i64(i64 %64, i64 %64, i64 16)
  %71 = xor i64 %69, %70
  %72 = add i64 %68, %71
  %73 = tail call i64 @llvm.fshl.i64(i64 %71, i64 %71, i64 21)
  %74 = xor i64 %72, %73
  %75 = add i64 %67, %69
  %76 = tail call i64 @llvm.fshl.i64(i64 %67, i64 %67, i64 17)
  %77 = xor i64 %76, %75
  %78 = tail call i64 @llvm.fshl.i64(i64 %75, i64 %75, i64 32)
  %79 = add nuw nsw i32 %63, 1
  %80 = icmp eq i32 %79, 4
  br i1 %80, label %81, label %59

81:
  %82 = xor i64 %78, %74
  %83 = xor i64 %82, %77
  %84 = xor i64 %83, %72
  ret i64 %84
}

declare i64 @llvm.fshl.i64(i64, i64, i64)
)";

// SipHash computation (compile-time version for finding non-colliding hashes)
static uint64_t sipHash(uint64_t in, uint64_t k0, uint64_t k1, 
                        uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3) {
    #define ROTL64(x, b) (((x) << (b)) | ((x) >> (64 - (b))))
    #define SIPROUND do { \
        v0 += v1; v1 = ROTL64(v1, 13); v1 ^= v0; v0 = ROTL64(v0, 32); \
        v2 += v3; v3 = ROTL64(v3, 16); v3 ^= v2; \
        v0 += v3; v3 = ROTL64(v3, 21); v3 ^= v0; \
        v2 += v1; v1 = ROTL64(v1, 17); v1 ^= v2; v2 = ROTL64(v2, 32); \
    } while(0)

    v3 ^= k1; v2 ^= k0; v1 ^= k1; v0 ^= k0;
    
    uint64_t b = (8ULL << 56) | in;
    v3 ^= in;
    SIPROUND; SIPROUND;
    v0 ^= in;
    
    v3 ^= b;
    SIPROUND; SIPROUND;
    v0 ^= b;
    
    v2 ^= 0xff;
    SIPROUND; SIPROUND; SIPROUND; SIPROUND;
    
    return v0 ^ v1 ^ v2 ^ v3;
    
    #undef ROTL64
    #undef SIPROUND
}

// Opaque transformer - applies reversible transformations to values
class OpaqueTransformer {
public:
    enum class OpType { XOR, ADD, SUB, ROL, ROR };

    OpaqueTransformer(bool is32Bit) : is32Bit_(is32Bit) {
        int numSteps = RNG->intRanged(2, 6);
        for (int i = 0; i < numSteps; i++) {
            OpType t = static_cast<OpType>(RNG->intRanged(0, 4));
            uint64_t c = RNG->intRanged<uint64_t>(0x000F0000, is32Bit ? UINT32_MAX : UINT64_MAX);
            if (t == OpType::ROL || t == OpType::ROR) {
                c = (c % 31) + 1;
            }
            ops_.push_back(t);
            constants_.push_back(c);
        }
    }

    uint64_t transformConstant(uint64_t input) {
        uint64_t current = input;
        unsigned bitWidth = is32Bit_ ? 32 : 64;
        uint64_t mask = is32Bit_ ? UINT32_MAX : UINT64_MAX;

        for (size_t i = 0; i < ops_.size(); i++) {
            uint64_t c = constants_[i];
            switch (ops_[i]) {
            case OpType::XOR: current ^= c; break;
            case OpType::ADD: current += c; break;
            case OpType::SUB: current -= c; break;
            case OpType::ROL:
                c %= bitWidth;
                current = ((current << c) | (current >> (bitWidth - c))) & mask;
                break;
            case OpType::ROR:
                c %= bitWidth;
                current = ((current >> c) | (current << (bitWidth - c))) & mask;
                break;
            }
            current &= mask;
        }
        return current;
    }

    Value* transform(Module& m, IRBuilder<>& builder, Value* input, int useGlobalChance) {
        IntegerType* targetType = is32Bit_ ? builder.getInt32Ty() : builder.getInt64Ty();
        Value* current = input;

        if (current->getType() != targetType) {
            current = is32Bit_ 
                ? builder.CreateTrunc(current, targetType)
                : builder.CreateZExtOrTrunc(current, targetType);
        }

        unsigned bitWidth = is32Bit_ ? 32 : 64;

        for (size_t i = 0; i < ops_.size(); i++) {
            uint64_t c = constants_[i];
            Value* cVal = getConstant(m, builder, c, useGlobalChance);

            switch (ops_[i]) {
            case OpType::XOR:
                current = builder.CreateXor(current, cVal);
                break;
            case OpType::ADD:
                current = builder.CreateAdd(current, cVal);
                break;
            case OpType::SUB:
                current = builder.CreateSub(current, cVal);
                break;
            case OpType::ROL: {
                c %= bitWidth;
                Value* shiftConst = getConstant(m, builder, c, useGlobalChance);
                Value* invShift = getConstant(m, builder, bitWidth - c, useGlobalChance);
                Value* left = builder.CreateShl(current, shiftConst);
                Value* right = builder.CreateLShr(current, invShift);
                current = builder.CreateOr(left, right);
                break;
            }
            case OpType::ROR: {
                c %= bitWidth;
                Value* shiftConst = getConstant(m, builder, c, useGlobalChance);
                Value* invShift = getConstant(m, builder, bitWidth - c, useGlobalChance);
                Value* right = builder.CreateLShr(current, shiftConst);
                Value* left = builder.CreateShl(current, invShift);
                current = builder.CreateOr(right, left);
                break;
            }
            }
        }
        return current;
    }

private:
    Value* getConstant(Module& m, IRBuilder<>& builder, uint64_t c, int useGlobalChance) {
        IntegerType* ty = is32Bit_ ? builder.getInt32Ty() : builder.getInt64Ty();
        
        if (RNG->chance(useGlobalChance)) {
            GlobalVariable* gv = new GlobalVariable(
                m, ty, false, GlobalValue::PrivateLinkage,
                ConstantInt::get(ty, c),
                "__state_var_" + std::to_string(c));
            return builder.CreateLoad(ty, gv);
        }
        return ConstantInt::get(ty, c);
    }

    bool is32Bit_;
    std::vector<OpType> ops_;
    std::vector<uint64_t> constants_;
};

// Check if function has C++ exceptions
static bool hasCXXExceptions(Function& f) {
    for (BasicBlock& bb : f) {
        Instruction* term = bb.getTerminator();
        if (bb.isEHPad() || bb.isLandingPad() || 
            isa<ResumeInst>(term) || isa<InvokeInst>(term)) {
            return true;
        }
    }
    return false;
}

// Demote PHI nodes to stack
static void demotePHIToStack(Function& f) {
    std::vector<PHINode*> phiNodes;
    for (BasicBlock& bb : f) {
        for (Instruction& i : bb) {
            if (PHINode* phi = dyn_cast<PHINode>(&i)) {
                phiNodes.push_back(phi);
            }
        }
    }
    for (PHINode* phi : phiNodes) {
        DemotePHIToStack(phi, std::nullopt);
    }
}

// Demote registers to stack where dominance is broken
static void demoteRegToStack(Function& f) {
    DominatorTree dt;
    dt.recalculate(f);
    std::vector<Instruction*> toDemote;
    
    for (BasicBlock& bb : f) {
        for (Instruction& i : bb) {
            if (i.getType()->isVoidTy() || isa<AllocaInst>(i) || i.isTerminator())
                continue;
            for (User* u : i.users()) {
                if (Instruction* ui = dyn_cast<Instruction>(u)) {
                    if (!dt.dominates(&i, ui)) {
                        toDemote.push_back(&i);
                        break;
                    }
                }
            }
        }
    }
    for (Instruction* i : toDemote) {
        DemoteRegToStack(*i, false);
    }
}

// Shuffle basic blocks randomly
static void shuffleBlocks(Function& f) {
    if (f.empty()) return;

    std::vector<BasicBlock*> bbs;
    BasicBlock& entry = f.getEntryBlock();

    for (BasicBlock& bb : f) {
        if (&bb != &entry) {
            bbs.push_back(&bb);
        }
    }

    if (bbs.empty()) return;

    std::shuffle(bbs.begin(), bbs.end(), std::mt19937(RNG->uint32()));

    BasicBlock* insertPoint = &entry;
    for (BasicBlock* bb : bbs) {
        bb->moveAfter(insertPoint);
        insertPoint = bb;
    }
}

// Ensure all allocas are in entry block
static void ensureAllocasInEntryBlocks(Function& f) {
    BasicBlock::iterator insertPt = f.getEntryBlock().begin();
    std::vector<AllocaInst*> toMove;

    for (BasicBlock& bb : f) {
        if (bb.isEntryBlock()) continue;
        for (Instruction& i : bb) {
            if (AllocaInst* ai = dyn_cast<AllocaInst>(&i)) {
                toMove.push_back(ai);
            }
        }
    }

    for (AllocaInst* ai : toMove) {
        ai->moveBefore(insertPt);
    }
}

// Get or create the SipHash function in the module
static Function* getOrCreateSipHashFunction(Module& m) {
    Function* fn = m.getFunction("___siphash");
    if (fn) return fn;

    SMDiagnostic err;
    std::unique_ptr<Module> sipHashMod = parseAssemblyString(SipHashLlvmIR, err, m.getContext());
    if (!sipHashMod) {
        errs() << "Error parsing SipHash IR: " << err.getMessage() << "\n";
        return nullptr;
    }

    // Link the SipHash module into target module
    if (Linker::linkModules(m, std::move(sipHashMod))) {
        errs() << "Error linking SipHash module\n";
        return nullptr;
    }

    fn = m.getFunction("___siphash");
    if (fn) {
        fn->setLinkage(GlobalValue::InternalLinkage);
    }
    return fn;
}

// Get state value, optionally from global variable
static Value* getTargetState(Module& m, IRBuilder<>& builder, uint64_t targetState,
                             bool is32Bit, TransformationOptions* options) {
    IntegerType* intTy = is32Bit ? builder.getInt32Ty() : builder.getInt64Ty();
    
    if (RNG->chance(options->useGlobalStateVariablesChance)) {
        GlobalVariable* gv = new GlobalVariable(
            m, intTy, false, GlobalValue::PrivateLinkage,
            ConstantInt::get(intTy, targetState),
            "__state_" + std::to_string(targetState));
        return builder.CreateLoad(intTy, gv, true);
    }
    return ConstantInt::get(intTy, targetState);
}

// Maybe transform dispatcher state using SipHash and/or opaque predicates
static void maybeTransformDispatcherState(Module& m, IRBuilder<>& builder,
                                          Value*& dispatcherState, uint64_t& targetState,
                                          TransformationOptions* options,
                                          std::set<uint64_t>& states, bool is32Bit) {
    // SipHash transformation
    if (RNG->chance(options->useSipHashedStateChance)) {
        uint64_t hashedState = 0;
        
        while (true) {
            uint64_t sipHashOptions[6] = {
                RNG->intRanged<uint64_t>(0x000F0000, UINT64_MAX), // k0
                RNG->intRanged<uint64_t>(0x000F0000, UINT64_MAX), // k1
                RNG->intRanged<uint64_t>(0x000F0000, UINT64_MAX), // v0
                RNG->intRanged<uint64_t>(0x000F0000, UINT64_MAX), // v1
                RNG->intRanged<uint64_t>(0x000F0000, UINT64_MAX), // v2
                RNG->intRanged<uint64_t>(0x000F0000, UINT64_MAX), // v3
            };

            uint64_t mask = is32Bit ? UINT32_MAX : UINT64_MAX;
            hashedState = sipHash(targetState, sipHashOptions[0], sipHashOptions[1],
                                  sipHashOptions[2], sipHashOptions[3],
                                  sipHashOptions[4], sipHashOptions[5]) & mask;

            // Check for collisions
            int collisions = 0;
            for (uint64_t state : states) {
                uint64_t h = sipHash(state, sipHashOptions[0], sipHashOptions[1],
                                     sipHashOptions[2], sipHashOptions[3],
                                     sipHashOptions[4], sipHashOptions[5]) & mask;
                if (h == hashedState) {
                    collisions++;
                }
            }

            if (collisions == 1 && !states.count(hashedState)) {
                // Only targetState matches this hash - use it
                targetState = hashedState;

                Function* fn = sipHashFn;
                if (RNG->chance(options->cloneSipHashChance)) {
                    ValueToValueMapTy vmap;
                    fn = CloneFunction(sipHashFn, vmap);
                    fn->setLinkage(GlobalValue::InternalLinkage);
                    fn->addFnAttr(Attribute::AlwaysInline);
                    fn->removeFnAttr(Attribute::NoInline);
                }

                Value* stateArg = is32Bit 
                    ? builder.CreateZExt(dispatcherState, builder.getInt64Ty())
                    : dispatcherState;

                Value* result = builder.CreateCall(fn, {
                    stateArg,
                    builder.getInt64(sipHashOptions[0]),
                    builder.getInt64(sipHashOptions[1]),
                    builder.getInt64(sipHashOptions[2]),
                    builder.getInt64(sipHashOptions[3]),
                    builder.getInt64(sipHashOptions[4]),
                    builder.getInt64(sipHashOptions[5])
                });

                dispatcherState = is32Bit
                    ? builder.CreateTrunc(result, builder.getInt32Ty())
                    : result;
                break;
            }
        }
    }

    // Opaque transformation
    if (RNG->chance(options->useOpaqueTransformationChance)) {
        OpaqueTransformer transformer(is32Bit);
        dispatcherState = transformer.transform(m, builder, dispatcherState,
                                                options->useGlobalVariableOpaquesChance);
        targetState = transformer.transformConstant(targetState);
    }
}

// Create a function that checks if state matches target
static Function* createFunctionForStateResolverCheck(Module& m, uint64_t targetState,
                                                     TransformationOptions* options,
                                                     std::set<uint64_t>& states, bool is32Bit) {
    LLVMContext& ctx = m.getContext();
    IntegerType* intTy = is32Bit ? Type::getInt32Ty(ctx) : Type::getInt64Ty(ctx);
    
    FunctionType* fnTy = FunctionType::get(Type::getInt1Ty(ctx), {intTy}, false);
    Function* fn = Function::Create(fnTy, GlobalValue::InternalLinkage,
                                    "cff_resolve_state_check", &m);

    BasicBlock* bb = BasicBlock::Create(ctx, "resolver.entry", fn);
    IRBuilder<> builder(bb);

    Value* stateArg = fn->getArg(0);
    uint64_t target = targetState;
    
    maybeTransformDispatcherState(m, builder, stateArg, target, options, states, is32Bit);

    Value* cmp = builder.CreateICmpEQ(stateArg, 
                                      getTargetState(m, builder, target, is32Bit, options));
    builder.CreateRet(cmp);

    return fn;
}

static void obfuscateFunction(Function& f, TransformationOptions* options) {
    if (hasCXXExceptions(f)) {
        return;
    }

    if (f.size() < 2) {
        return;
    }

    LLVMContext& ctx = f.getContext();
    Module* m = f.getParent();
    const DataLayout& dl = m->getDataLayout();
    unsigned ptrSize = dl.getPointerSize(0);
    bool is32Bit = (ptrSize == 4);

    IRBuilder<> builder(&*f.getEntryBlock().getFirstInsertionPt());
    IntegerType* intTy = is32Bit ? builder.getInt32Ty() : builder.getInt64Ty();

    // Create dispatcher state variable
    AllocaInst* dispatcherState = builder.CreateAlloca(intTy, nullptr, "state");
    builder.CreateStore(ConstantInt::get(intTy, 0), dispatcherState, true);

    // Collect original blocks (except entry)
    std::vector<BasicBlock*> originalBlocks;
    for (BasicBlock& bb : f) {
        if (&bb != &f.getEntryBlock()) {
            originalBlocks.push_back(&bb);
        }
    }

    // Assign random states to each block
    std::map<BasicBlock*, uint64_t> blockStateMap;
    std::set<uint64_t> states;
    
    for (BasicBlock* bb : originalBlocks) {
        uint64_t state;
        do {
            state = is32Bit 
                ? RNG->intRanged<uint64_t>(0x000F0000, UINT32_MAX)
                : RNG->intRanged<uint64_t>(0x000F0000, UINT64_MAX);
        } while (states.count(state));
        states.insert(state);
        blockStateMap[bb] = state;
    }

    // Create condition check blocks
    std::vector<BasicBlock*> conditionBlocks;
    for (size_t i = 0; i < originalBlocks.size(); ++i) {
        BasicBlock* bb = BasicBlock::Create(ctx, "cond_check." + std::to_string(i), &f);
        conditionBlocks.push_back(bb);
    }

    // Create dispatcher block
    BasicBlock* dispatchBB = BasicBlock::Create(ctx, "dispatch", &f);
    builder.SetInsertPoint(dispatchBB);
    builder.CreateBr(conditionBlocks.front());

    // Build condition blocks
    for (size_t i = 0; i < conditionBlocks.size(); ++i) {
        builder.SetInsertPoint(conditionBlocks[i]);

        uint64_t targetState = blockStateMap[originalBlocks[i]];
        Value* stateVal = builder.CreateLoad(intTy, dispatcherState, true, "state_val");

        Value* cmp;
        
        if (RNG->chance(options->useFunctionResolverChance)) {
            // Use function call to check state
            Function* resolver = createFunctionForStateResolverCheck(
                *m, targetState, options, states, is32Bit);
            cmp = builder.CreateCall(resolver, {stateVal});
        } else {
            // Inline state check
            maybeTransformDispatcherState(*m, builder, stateVal, targetState,
                                          options, states, is32Bit);
            cmp = builder.CreateICmpEQ(stateVal,
                                       getTargetState(*m, builder, targetState, is32Bit, options));
        }

        if (i < conditionBlocks.size() - 1) {
            builder.CreateCondBr(cmp, originalBlocks[i], conditionBlocks[i + 1]);
        } else {
            // Last block: branch to target or loop back to dispatcher
            BasicBlock* defaultBB = BasicBlock::Create(ctx, "default", &f);
            IRBuilder<> defaultBuilder(defaultBB);
            defaultBuilder.CreateBr(dispatchBB);
            builder.CreateCondBr(cmp, originalBlocks[i], defaultBB);
        }
    }

    // Modify terminators to update state and jump to dispatcher
    originalBlocks.push_back(&f.getEntryBlock());
    
    for (BasicBlock* bb : originalBlocks) {
        Instruction* terminator = bb->getTerminator();
        builder.SetInsertPoint(terminator);

        if (auto* br = dyn_cast<BranchInst>(terminator)) {
            if (br->isUnconditional()) {
                BasicBlock* target = br->getSuccessor(0);
                builder.CreateStore(
                    ConstantInt::get(intTy, blockStateMap[target]),
                    dispatcherState, true);
                builder.CreateBr(dispatchBB);
                terminator->eraseFromParent();
            } else {
                BasicBlock* trueBB = br->getSuccessor(0);
                BasicBlock* falseBB = br->getSuccessor(1);

                BasicBlock* trueState = BasicBlock::Create(ctx, "cff.true_state", &f);
                BasicBlock* falseState = BasicBlock::Create(ctx, "cff.false_state", &f);

                builder.SetInsertPoint(trueState);
                builder.CreateStore(
                    ConstantInt::get(intTy, blockStateMap[trueBB]),
                    dispatcherState, true);
                builder.CreateBr(dispatchBB);

                builder.SetInsertPoint(falseState);
                builder.CreateStore(
                    ConstantInt::get(intTy, blockStateMap[falseBB]),
                    dispatcherState, true);
                builder.CreateBr(dispatchBB);

                builder.SetInsertPoint(terminator);
                builder.CreateCondBr(br->getCondition(), trueState, falseState);
                terminator->eraseFromParent();
            }
        }
    }

    demoteRegToStack(f);
}

static void obfuscateModule(Module& mod, int iterations, TransformationOptions* options) {
    // Initialize SipHash function if needed
    if (options->useSipHashedStateChance > 0) {
        sipHashFn = getOrCreateSipHashFunction(mod);
        if (!sipHashFn) {
            errs() << "Warning: Failed to create SipHash function, disabling SipHash\n";
            options->useSipHashedStateChance = 0;
        } else {
            // Prepare SipHash function
            demoteRegToStack(*sipHashFn);
            demotePHIToStack(*sipHashFn);
        }
    }

    for (int i = 0; i < iterations; i++) {
        for (Function& func : mod) {
            if (!func.isDeclaration() && &func != sipHashFn) {
                obfuscateFunction(func, options);
            }
        }
    }

    // Post-processing
    for (Function& func : mod) {
        if (!func.isDeclaration()) {
            shuffleBlocks(func);
            ensureAllocasInEntryBlocks(func);
            demoteRegToStack(func);
            demotePHIToStack(func);
        }
    }
}

int main(int argc, char** argv) {
    cl::ParseCommandLineOptions(argc, argv,
                                "Control Flow Flattening Obfuscator\n\n"
                                "Flattens control flow using a switch-based dispatcher.\n");

    RNG = new Random(Seed);

    TransformationOptions options = {
        .useFunctionResolverChance = UseFuncResolverChance,
        .useGlobalStateVariablesChance = UseGlobalStateChance,
        .useOpaqueTransformationChance = UseOpaqueChance,
        .useGlobalVariableOpaquesChance = UseGlobalOpaqueChance,
        .useSipHashedStateChance = UseSipHashChance,
        .cloneSipHashChance = CloneSipHashChance,
    };

    LLVMContext context;
    SMDiagnostic err;

    std::unique_ptr<Module> mod = parseIRFile(InputFilename, err, context);
    if (!mod) {
        err.print(argv[0], errs());
        return 1;
    }

    obfuscateModule(*mod, Iterations, &options);

    std::error_code ec;
    raw_fd_ostream out(OutputFilename, ec, sys::fs::OF_None);
    if (ec) {
        errs() << "Error opening output file: " << ec.message() << "\n";
        return 1;
    }

    WriteBitcodeToFile(*mod, out);

    delete RNG;
    return 0;
}
