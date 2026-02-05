/**
 * String Encryption Tool
 *
 * Encrypts string constants in the module and generates decryption code
 * that runs at program startup (via .ctors) or on first use (stack-based).
 *
 * Implements XOR-based string encryption with SplitMix32 PRNG.
 *
 * Usage:
 *   string_encrypt [options] <input.bc> <output.bc>
 *
 * Options:
 *   --mode MODE       Encryption mode: "global" (ctor-based) or "stack" (on-use) (default: global)
 *   --skip-prefix P   Skip strings starting with this prefix
 *   --seed N          Random seed for reproducibility (default: random)
 *   --help            Show this help
 */

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>

#include <random>
#include <vector>
#include <string>

using namespace llvm;

// Command line options
static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("<input bitcode>"),
                                          cl::Required);

static cl::opt<std::string> OutputFilename(cl::Positional,
                                           cl::desc("<output bitcode>"),
                                           cl::Required);

static cl::opt<std::string> Mode("mode",
                                 cl::desc("Encryption mode: 'global' or 'stack' (default: global)"),
                                 cl::init("global"));

static cl::opt<std::string> SkipPrefix("skip-prefix",
                                       cl::desc("Skip strings starting with this prefix"),
                                       cl::init(""));

static cl::opt<unsigned> Seed("seed",
                              cl::desc("Random seed (default: random)"),
                              cl::init(0));

// Random number generator
class Random {
public:
    Random(unsigned seed) : rng_(seed ? seed : std::random_device{}()) {}

    uint32_t uint32() {
        std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
        return dist(rng_);
    }

private:
    std::mt19937 rng_;
};

static Random* RNG = nullptr;

// SplitMix32 PRNG for encryption
static uint32_t splitMix32(uint32_t& state) {
    state += 0x9E3779B9u;
    uint32_t z = state;
    z ^= z >> 16;
    z *= 0x85EBCA6Bu;
    z ^= z >> 13;
    z *= 0xC2B2AE35u;
    z ^= z >> 16;
    return z;
}

// XOR encrypt strings using SplitMix32
static void xorEncryptStrings(std::vector<std::string>& strings, uint32_t masterSeed) {
    for (size_t i = 0; i < strings.size(); ++i) {
        uint32_t seed = masterSeed ^ static_cast<uint32_t>(i);
        uint32_t state = seed;
        std::string& s = strings[i];
        size_t len = s.size();
        size_t offset = 0;
        
        while (offset < len) {
            uint32_t keyStream = splitMix32(state);
            size_t chunk = std::min(len - offset, sizeof(uint32_t));
            for (size_t j = 0; j < chunk; ++j) {
                s[offset + j] ^= ((keyStream >> (j * 8)) & 0xFF);
            }
            offset += chunk;
        }
    }
}

// Emit SplitMix32 in IR
static std::pair<Value*, Value*> emitSplitMix32(IRBuilder<>& builder, Value* state) {
    Value* cAdd = builder.getInt32(0x9E3779B9);
    Value* cMul1 = builder.getInt32(0x85EBCA6B);
    Value* cMul2 = builder.getInt32(0xC2B2AE35);
    
    Value* newState = builder.CreateAdd(state, cAdd);
    Value* z = newState;
    z = builder.CreateXor(z, builder.CreateLShr(z, 16));
    z = builder.CreateMul(z, cMul1);
    z = builder.CreateXor(z, builder.CreateLShr(z, 13));
    z = builder.CreateMul(z, cMul2);
    z = builder.CreateXor(z, builder.CreateLShr(z, 16));
    
    return {newState, z};
}

// Emit decryption loop for a buffer
static void emitDecryptBuffer(IRBuilder<>& builder, Value* stateSeed,
                              Value* inPtr, Value* outPtr, Value* strLen,
                              AllocaInst* offVar, AllocaInst* stateVar, AllocaInst* jVar) {
    LLVMContext& ctx = builder.getContext();
    Type* i32 = Type::getInt32Ty(ctx);
    Type* i8 = Type::getInt8Ty(ctx);

    Function* f = builder.GetInsertBlock()->getParent();

    builder.CreateStore(ConstantInt::get(i32, 0), offVar, true);
    builder.CreateStore(stateSeed, stateVar, true);

    BasicBlock* loopOffBB = BasicBlock::Create(ctx, "dec.loop.off", f);
    BasicBlock* bodyOffBB = BasicBlock::Create(ctx, "dec.body.off", f);
    BasicBlock* afterOffBB = BasicBlock::Create(ctx, "dec.after.off", f);

    builder.CreateBr(loopOffBB);
    builder.SetInsertPoint(loopOffBB);

    Value* currentOff = builder.CreateLoad(i32, offVar, true);
    Value* currentState = builder.CreateLoad(i32, stateVar, true);

    Value* cmpOff = builder.CreateICmpULT(currentOff, strLen);
    builder.CreateCondBr(cmpOff, bodyOffBB, afterOffBB);

    builder.SetInsertPoint(bodyOffBB);
    auto [newState, keyStream] = emitSplitMix32(builder, currentState);
    
    Value* rem = builder.CreateSub(strLen, currentOff);
    Value* c4 = ConstantInt::get(i32, 4);
    Value* chunk = builder.CreateSelect(builder.CreateICmpULT(rem, c4), rem, c4);

    BasicBlock* loopJBB = BasicBlock::Create(ctx, "dec.loop.j", f);
    BasicBlock* bodyJBB = BasicBlock::Create(ctx, "dec.body.j", f);
    BasicBlock* afterJBB = BasicBlock::Create(ctx, "dec.after.j", f);

    builder.CreateStore(ConstantInt::get(i32, 0), jVar, true);
    builder.CreateBr(loopJBB);

    builder.SetInsertPoint(loopJBB);
    Value* currentJ = builder.CreateLoad(i32, jVar, true);
    Value* cmpJ = builder.CreateICmpULT(currentJ, chunk);
    builder.CreateCondBr(cmpJ, bodyJBB, afterJBB);

    builder.SetInsertPoint(bodyJBB);
    Value* offPlusJ = builder.CreateAdd(currentOff, currentJ);
    Value* inByte = builder.CreateInBoundsGEP(i8, inPtr, offPlusJ);
    Value* orig = builder.CreateLoad(i8, inByte, true);
    
    Value* shift = builder.CreateMul(currentJ, ConstantInt::get(i32, 8));
    Value* shr = builder.CreateLShr(keyStream, shift);
    Value* mask = builder.CreateTrunc(shr, i8);
    Value* out = builder.CreateXor(orig, mask);
    
    Value* outByte = builder.CreateInBoundsGEP(i8, outPtr, offPlusJ);
    builder.CreateStore(out, outByte, true);

    Value* jNext = builder.CreateAdd(currentJ, ConstantInt::get(i32, 1));
    builder.CreateStore(jNext, jVar, true);
    builder.CreateBr(loopJBB);

    builder.SetInsertPoint(afterJBB);
    Value* offNext = builder.CreateAdd(currentOff, chunk);
    builder.CreateStore(offNext, offVar, true);
    builder.CreateStore(newState, stateVar, true);
    builder.CreateBr(loopOffBB);

    builder.SetInsertPoint(afterOffBB);
}

static void obfuscateGlobalStrings(Module& m, const std::string& skipPrefix) {
    LLVMContext& ctx = m.getContext();
    Type* i32 = Type::getInt32Ty(ctx);
    Type* i8Ptr = PointerType::getUnqual(ctx);

    std::vector<GlobalVariable*> gvList;
    std::vector<std::string> rawStrings;
    std::vector<Constant*> ptrList;
    std::vector<Constant*> lenList;

    // Collect string globals
    for (GlobalVariable& gv : m.globals()) {
        if (!gv.hasInitializer()) continue;
        
        Constant* init = gv.getInitializer();
        auto* arr = dyn_cast<ConstantDataArray>(init);
        if (!arr || !arr->isString() || !isa<ArrayType>(arr->getType())) continue;
        
        StringRef name = gv.getName();
        if (name.starts_with("llvm.")) continue;
        if (name.starts_with(".str") == false && name.starts_with("str") == false) {
            // Skip non-string-like names unless they look like string constants
        }
        
        if (gv.hasSection()) {
            StringRef section = gv.getSection();
            if (section.starts_with("debug") || section.starts_with("llvm")) continue;
        }

        std::string raw = arr->getAsString().str();
        
        // Skip if matches prefix
        if (!skipPrefix.empty() && raw.rfind(skipPrefix, 0) == 0) continue;

        uint64_t strLen = arr->getType()->getNumElements();

        gvList.push_back(&gv);
        rawStrings.push_back(raw);

        Constant* ptr = ConstantExpr::getBitCast(&gv, i8Ptr);
        ptrList.push_back(ptr);
        lenList.push_back(ConstantInt::get(i32, strLen));
    }

    if (gvList.empty()) {
        errs() << "No strings found to encrypt\n";
        return;
    }

    errs() << "Encrypting " << gvList.size() << " strings\n";

    // Encrypt strings
    uint32_t masterSeed = RNG->uint32();
    xorEncryptStrings(rawStrings, masterSeed);

    // Update global initializers with encrypted data
    for (size_t i = 0; i < gvList.size(); ++i) {
        GlobalVariable* gv = gvList[i];
        const std::string& encStr = rawStrings[i];
        Constant* encInit = ConstantDataArray::getString(ctx, encStr, false);
        gv->setInitializer(encInit);
        gv->setConstant(false);
    }

    // Create pointer and length tables
    ArrayType* ptrArrTy = ArrayType::get(i8Ptr, ptrList.size());
    Constant* ptrArrInit = ConstantArray::get(ptrArrTy, ptrList);
    auto* ptrTable = new GlobalVariable(m, ptrArrTy, false,
                                        GlobalValue::InternalLinkage,
                                        ptrArrInit, "__enc_ptr_table");

    ArrayType* lenArrTy = ArrayType::get(i32, lenList.size());
    Constant* lenArrInit = ConstantArray::get(lenArrTy, lenList);
    auto* lenTable = new GlobalVariable(m, lenArrTy, false,
                                        GlobalValue::InternalLinkage,
                                        lenArrInit, "__enc_len_table");

    // Create decryption constructor function
    FunctionType* fnTy = FunctionType::get(Type::getVoidTy(ctx), false);
    Function* decryptFn = Function::Create(
        fnTy, GlobalValue::InternalLinkage, "__decrypt_strings_ctor", &m);
    
    BasicBlock* entry = BasicBlock::Create(ctx, "entry", decryptFn);
    IRBuilder<> builder(entry);

    // Allocate loop variables
    AllocaInst* masterVar = builder.CreateAlloca(i32);
    AllocaInst* offVar = builder.CreateAlloca(i32, nullptr, "dec.offset");
    AllocaInst* stateVar = builder.CreateAlloca(i32, nullptr, "dec.state");
    AllocaInst* jVar = builder.CreateAlloca(i32, nullptr, "dec.j");

    Value* numStrings = ConstantInt::get(i32, gvList.size());

    BasicBlock* loopHeader = BasicBlock::Create(ctx, "loop.header", decryptFn);
    BasicBlock* loopBody = BasicBlock::Create(ctx, "loop.body", decryptFn);
    BasicBlock* loopExit = BasicBlock::Create(ctx, "loop.exit", decryptFn);

    builder.CreateStore(builder.getInt32(0), masterVar);
    builder.CreateBr(loopHeader);

    builder.SetInsertPoint(loopHeader);
    Value* masterVal = builder.CreateLoad(i32, masterVar);
    Value* cmp = builder.CreateICmpULT(masterVal, numStrings);
    builder.CreateCondBr(cmp, loopBody, loopExit);

    builder.SetInsertPoint(loopBody);

    Value* strPtr = builder.CreateLoad(
        i8Ptr,
        builder.CreateInBoundsGEP(ptrArrTy, ptrTable,
                                  {builder.getInt64(0), masterVal}));
    Value* strLen = builder.CreateLoad(
        i32,
        builder.CreateInBoundsGEP(lenArrTy, lenTable,
                                  {builder.getInt64(0), masterVal}));

    Value* masterSeedVal = ConstantInt::get(i32, masterSeed);
    Value* stateSeed = builder.CreateXor(masterSeedVal, masterVal);

    emitDecryptBuffer(builder, stateSeed, strPtr, strPtr, strLen,
                      offVar, stateVar, jVar);

    builder.CreateStore(
        builder.CreateAdd(masterVal, builder.getInt32(1)),
        masterVar);
    builder.CreateBr(loopHeader);

    builder.SetInsertPoint(loopExit);
    builder.CreateRetVoid();

    // Register as global constructor
    appendToGlobalCtors(m, decryptFn, 0);
}

static void obfuscateStackStrings(Module& m, const std::string& skipPrefix) {
    LLVMContext& ctx = m.getContext();
    Type* i8 = Type::getInt8Ty(ctx);
    Type* i32 = Type::getInt32Ty(ctx);
    Type* i64 = Type::getInt64Ty(ctx);
    Type* i8Ptr = PointerType::getUnqual(ctx);

    std::vector<std::pair<GlobalVariable*, std::string>> stackList;

    // Collect strings that can be encrypted on stack
    for (GlobalVariable& gv : m.globals()) {
        if (!gv.hasInitializer()) continue;
        
        auto* arr = dyn_cast<ConstantDataArray>(gv.getInitializer());
        if (!arr || !arr->isString()) continue;
        
        StringRef name = gv.getName();
        if (name.starts_with("llvm.")) continue;
        
        std::string raw = arr->getAsString().str();
        if (!skipPrefix.empty() && raw.rfind(skipPrefix, 0) == 0) continue;

        // Check if all uses are in functions
        bool isValid = true;
        for (Use& use : gv.uses()) {
            Value* user = use.getUser();
            
            // Handle constant expressions
            if (auto* ce = dyn_cast<ConstantExpr>(user)) {
                for (User* ceUser : ce->users()) {
                    if (auto* inst = dyn_cast<Instruction>(ceUser)) {
                        if (!inst->getFunction()) {
                            isValid = false;
                            break;
                        }
                    } else {
                        isValid = false;
                        break;
                    }
                }
                if (!isValid) break;
                continue;
            }

            if (auto* inst = dyn_cast<Instruction>(user)) {
                if (!inst->getFunction()) {
                    isValid = false;
                    break;
                }
            } else {
                isValid = false;
                break;
            }
        }

        if (isValid) {
            stackList.push_back({&gv, raw});
        }
    }

    errs() << "Encrypting " << stackList.size() << " strings on stack\n";

    // Process each string for stack encryption
    for (auto& [gv, raw] : stackList) {
        uint32_t seed = RNG->uint32();

        std::vector<std::string> strings = {raw};
        xorEncryptStrings(strings, seed);
        std::string encrypted = strings[0];

        // Create new encrypted global
        Constant* newConst = ConstantDataArray::getString(ctx, encrypted, false);
        GlobalVariable* newGV = new GlobalVariable(
            m, newConst->getType(), false, GlobalValue::PrivateLinkage,
            newConst, "", nullptr, GlobalValue::NotThreadLocal, 0);
        newGV->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
        newGV->setAlignment(Align(1));

        int size = newConst->getType()->getArrayNumElements();

        // Collect uses to replace
        std::vector<Use*> usesToReplace;
        for (Use& use : gv->uses()) {
            usesToReplace.push_back(&use);
        }

        for (Use* usePtr : usesToReplace) {
            Value* user = usePtr->getUser();
            
            // Handle constant expressions
            if (auto* ce = dyn_cast<ConstantExpr>(user)) {
                if (ce->getOpcode() == Instruction::GetElementPtr) {
                    for (User* ceUser : ce->users()) {
                        if (auto* instUser = dyn_cast<Instruction>(ceUser)) {
                            IRBuilder<> b(instUser);
                            auto* gep = cast<GetElementPtrInst>(ce->getAsInstruction());
                            b.Insert(gep);
                            instUser->replaceUsesOfWith(ce, gep);
                        }
                    }
                }
                continue;
            }

            auto* userInst = dyn_cast<Instruction>(user);
            if (!userInst || !userInst->getFunction()) continue;

            Function* f = userInst->getFunction();
            IRBuilder<> builder(&*f->getEntryBlock().getFirstInsertionPt());

            // Allocate stack buffer
            AllocaInst* alloca = builder.CreateAlloca(
                ArrayType::get(i8, size), nullptr, "str_stack");
            alloca->setAlignment(Align(4));

            // Allocate decryption loop variables
            AllocaInst* offVar = builder.CreateAlloca(i32, nullptr, "dec.off");
            AllocaInst* stateVar = builder.CreateAlloca(i32, nullptr, "dec.state");
            AllocaInst* jVar = builder.CreateAlloca(i32, nullptr, "dec.j");

            // Split block before use
            builder.SetInsertPoint(userInst);
            BasicBlock* original = userInst->getParent();
            BasicBlock* split = original->splitBasicBlock(userInst);
            Instruction* term = original->getTerminator();

            builder.SetInsertPoint(term);

            // Copy encrypted data to stack
            Value* allocaCast = builder.CreateBitCast(alloca, i8Ptr);
            Value* srcCast = ConstantExpr::getBitCast(newGV, i8Ptr);
            builder.CreateMemCpy(allocaCast, Align(1), srcCast, Align(1),
                                 ConstantInt::get(i64, size));

            // Get pointer to first element
            Value* firstElem = builder.CreateInBoundsGEP(
                alloca->getAllocatedType(), alloca,
                {builder.getInt32(0), builder.getInt32(0)});

            // Emit decryption
            emitDecryptBuffer(builder, builder.getInt32(seed),
                              firstElem, firstElem, builder.getInt32(size),
                              offVar, stateVar, jVar);

            builder.CreateBr(split);
            term->eraseFromParent();

            usePtr->set(firstElem);
        }

        gv->eraseFromParent();
    }
}

int main(int argc, char** argv) {
    cl::ParseCommandLineOptions(argc, argv,
                                "String Encryption Obfuscator\n\n"
                                "Encrypts string constants with XOR cipher.\n");

    RNG = new Random(Seed);

    LLVMContext context;
    SMDiagnostic err;

    std::unique_ptr<Module> mod = parseIRFile(InputFilename, err, context);
    if (!mod) {
        err.print(argv[0], errs());
        return 1;
    }

    if (Mode == "global") {
        obfuscateGlobalStrings(*mod, SkipPrefix);
    } else if (Mode == "stack") {
        obfuscateStackStrings(*mod, SkipPrefix);
    } else {
        errs() << "Unknown mode: " << Mode << ". Use 'global' or 'stack'.\n";
        return 1;
    }

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
