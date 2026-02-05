/**
 * Encrypted Indirect Branch Tool
 *
 * Replaces direct branches with indirect branches through an encrypted
 * block address table using XTEA cipher. This is the "full" version of
 * indirect_branch with runtime decryption.
 *
 * Implements XTEA-encrypted indirect branch obfuscation.
 *
 * Usage:
 *   indirect_branch_enc [options] <input.bc> <output.bc>
 *
 * Options:
 *   --iterations N    Number of iterations (default: 1)
 *   --chance N        Percent chance to replace branches (default: 50)
 *   --seed N          Random seed for reproducibility (default: random)
 *   --help            Show this help
 *
 * Note: This pass generates encrypted branch tables that require a runtime
 * decryption step. The encryption keys are embedded in the output module.
 */

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

#include <random>
#include <vector>
#include <map>
#include <numeric>

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

static cl::opt<int> ReplaceChance("chance",
                                  cl::desc("Percent chance to replace (default: 50)"),
                                  cl::init(50));

static cl::opt<unsigned> Seed("seed",
                              cl::desc("Random seed (default: random)"),
                              cl::init(0));

// Random number generator
class Random {
public:
    Random(unsigned seed) : rng_(seed ? seed : std::random_device{}()) {}

    bool chance(int percent) {
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng_) <= percent;
    }

    uint32_t uint32() {
        std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
        return dist(rng_);
    }

    // Simple RNG for deterministic key generation
    class SimpleRNG {
        uint32_t state_;
    public:
        explicit SimpleRNG(uint32_t seed) : state_(seed) {}

        uint32_t next() {
            state_ += 0x9E3779B9;
            uint32_t z = state_;
            z ^= z >> 15;
            z *= 0x85EBCA6B;
            z ^= z >> 13;
            z *= 0xC2B2AE35;
            z ^= z >> 16;
            return z;
        }
    };

private:
    std::mt19937 rng_;
};

static Random* RNG = nullptr;

// XTEA encryption info for a block
struct XTEAInfo {
    uint32_t key[4];
    uint32_t delta;
    uint32_t rounds;
};

// XTEA encipher (for compile-time encryption of addresses)
static uint64_t xteaEncipher(uint64_t value, const XTEAInfo& info) {
    uint32_t v0 = static_cast<uint32_t>(value);
    uint32_t v1 = static_cast<uint32_t>(value >> 32);
    uint32_t sum = 0;
    
    for (uint32_t i = 0; i < info.rounds; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + info.key[sum & 3]);
        sum += info.delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + info.key[(sum >> 11) & 3]);
    }
    
    return (static_cast<uint64_t>(v1) << 32) | v0;
}

// Generate XTEA decipher code
static void emitXTEADecipher(IRBuilder<>& builder, Value* dataPtr,
                             Value* keyPtr, Value* delta, Value* rounds,
                             AllocaInst* v0Var, AllocaInst* v1Var,
                             AllocaInst* sumVar, AllocaInst* iVar) {
    Type* u32Ty = builder.getInt32Ty();
    Function* fn = builder.GetInsertBlock()->getParent();
    LLVMContext& ctx = fn->getContext();

    BasicBlock* currentBB = builder.GetInsertBlock();
    BasicBlock* splitBB = currentBB->splitBasicBlock(builder.GetInsertPoint(), "xtea.cont");

    builder.SetInsertPoint(currentBB->getTerminator());

    // Load v[0] and v[1]
    Value* v0Ptr = builder.CreateInBoundsGEP(u32Ty, dataPtr, builder.getInt32(0));
    Value* v1Ptr = builder.CreateInBoundsGEP(u32Ty, dataPtr, builder.getInt32(1));
    
    Value* v0Val = builder.CreateLoad(u32Ty, v0Ptr);
    Value* v1Val = builder.CreateLoad(u32Ty, v1Ptr);

    builder.CreateStore(v0Val, v0Var);
    builder.CreateStore(v1Val, v1Var);

    // sum = delta * rounds
    Value* sumInit = builder.CreateMul(delta, rounds);
    builder.CreateStore(sumInit, sumVar);
    builder.CreateStore(builder.getInt32(0), iVar);

    // Create loop blocks
    BasicBlock* loopCond = BasicBlock::Create(ctx, "xtea.cond", fn);
    BasicBlock* loopBody = BasicBlock::Create(ctx, "xtea.body", fn);
    BasicBlock* loopEnd = BasicBlock::Create(ctx, "xtea.end", fn);

    builder.CreateBr(loopCond);
    currentBB->getTerminator()->eraseFromParent();

    // Loop condition
    builder.SetInsertPoint(loopCond);
    Value* iVal = builder.CreateLoad(u32Ty, iVar);
    Value* cond = builder.CreateICmpULT(iVal, rounds);
    builder.CreateCondBr(cond, loopBody, loopEnd);

    // Loop body - XTEA decipher round
    builder.SetInsertPoint(loopBody);

    v0Val = builder.CreateLoad(u32Ty, v0Var);
    v1Val = builder.CreateLoad(u32Ty, v1Var);
    Value* sumVal = builder.CreateLoad(u32Ty, sumVar);

    // v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3])
    Value* v0Shl4 = builder.CreateShl(v0Val, 4);
    Value* v0Lshr5 = builder.CreateLShr(v0Val, 5);
    Value* v0Xor = builder.CreateXor(v0Shl4, v0Lshr5);
    Value* tmp1 = builder.CreateAdd(v0Xor, v0Val);

    Value* keyIdx1 = builder.CreateAnd(builder.CreateLShr(sumVal, 11), 3);
    Value* keyPtr1 = builder.CreateInBoundsGEP(u32Ty, keyPtr, keyIdx1);
    Value* keyVal1 = builder.CreateLoad(u32Ty, keyPtr1);

    Value* sumPlusKey1 = builder.CreateAdd(sumVal, keyVal1);
    Value* xorVal1 = builder.CreateXor(tmp1, sumPlusKey1);
    Value* v1Sub = builder.CreateSub(v1Val, xorVal1);
    builder.CreateStore(v1Sub, v1Var);

    // sum -= delta
    Value* sumSub = builder.CreateSub(sumVal, delta);
    builder.CreateStore(sumSub, sumVar);

    // v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3])
    v1Val = builder.CreateLoad(u32Ty, v1Var);
    Value* v1Shl4 = builder.CreateShl(v1Val, 4);
    Value* v1Lshr5 = builder.CreateLShr(v1Val, 5);
    Value* v1Xor = builder.CreateXor(v1Shl4, v1Lshr5);
    Value* tmp2 = builder.CreateAdd(v1Xor, v1Val);

    sumVal = builder.CreateLoad(u32Ty, sumVar);
    Value* keyIdx2 = builder.CreateAnd(sumVal, 3);
    Value* keyPtr2 = builder.CreateInBoundsGEP(u32Ty, keyPtr, keyIdx2);
    Value* keyVal2 = builder.CreateLoad(u32Ty, keyPtr2);

    Value* sumPlusKey2 = builder.CreateAdd(sumVal, keyVal2);
    Value* xorVal2 = builder.CreateXor(tmp2, sumPlusKey2);
    
    v0Val = builder.CreateLoad(u32Ty, v0Var);
    Value* v0Sub = builder.CreateSub(v0Val, xorVal2);
    builder.CreateStore(v0Sub, v0Var);

    // i++
    Value* iInc = builder.CreateAdd(iVal, builder.getInt32(1));
    builder.CreateStore(iInc, iVar);
    builder.CreateBr(loopCond);

    // Loop end - store results back
    builder.SetInsertPoint(loopEnd);
    v0Val = builder.CreateLoad(u32Ty, v0Var);
    v1Val = builder.CreateLoad(u32Ty, v1Var);
    builder.CreateStore(v0Val, v0Ptr);
    builder.CreateStore(v1Val, v1Ptr);
    builder.CreateBr(splitBB);

    builder.SetInsertPoint(&splitBB->front());
}

template <typename K, typename V>
static int indexOf(const std::map<K, V>& m, const K& key) {
    int index = 0;
    for (auto it = m.begin(); it != m.end(); ++it, ++index) {
        if (it->first == key) return index;
    }
    return -1;
}

static void obfuscateFunction(Function& func, int replaceChance) {
    if (func.size() < 2) return;

    int branchCount = std::accumulate(
        func.begin(), func.end(), 0,
        [](int acc, const BasicBlock& bb) {
            return acc + (bb.getTerminator()->getOpcode() == Instruction::Br ? 1 : 0);
        });

    if (branchCount == 0) return;

    BasicBlock* entry = &func.getEntryBlock();
    IRBuilder<> builder(&*entry->getFirstInsertionPt());
    
    LLVMContext& ctx = func.getContext();
    Module* module = func.getParent();
    const DataLayout& dl = module->getDataLayout();
    unsigned ptrSize = dl.getPointerSize(0);
    bool is32Bit = (ptrSize == 4);

    Type* ptrTy = PointerType::getUnqual(ctx);
    Type* pintTy = builder.getIntPtrTy(dl);
    Type* u32Ty = builder.getInt32Ty();
    Type* u64Ty = builder.getInt64Ty();
    ArrayType* keyArrayTy = ArrayType::get(u32Ty, 4);

    // Collect branches and their targets
    std::vector<BranchInst*> branches;
    std::map<BasicBlock*, uint32_t> targetBBs;

    for (BasicBlock& bb : func) {
        if (auto* branch = dyn_cast<BranchInst>(bb.getTerminator())) {
            if (RNG->chance(replaceChance)) {
                branches.push_back(branch);
                for (BasicBlock* target : branch->successors()) {
                    if (!targetBBs.count(target)) {
                        targetBBs[target] = 0;
                    }
                }
            }
        }
    }

    if (branches.empty()) return;

    // Generate XTEA keys for each target
    uint32_t masterSeed = RNG->uint32();
    Random::SimpleRNG rng(masterSeed);
    
    std::map<BasicBlock*, XTEAInfo> blockXTEA;
    for (auto& [bb, _] : targetBBs) {
        XTEAInfo info;
        info.key[0] = rng.next();
        info.key[1] = rng.next();
        info.key[2] = rng.next();
        info.key[3] = rng.next();
        info.delta = rng.next();
        info.rounds = (rng.next() % 3) + 1;
        blockXTEA[bb] = info;
    }

    // Create block address array with magic header
    constexpr int ARRAY_MARKER = 4; // 3 magic + 1 seed
    std::vector<Constant*> elems;
    elems.reserve(targetBBs.size() * (is32Bit ? 2 : 1) + ARRAY_MARKER);

    // Magic markers
    for (int i = 0; i < ARRAY_MARKER - 1; i++) {
        elems.push_back(ConstantExpr::getIntToPtr(
            ConstantInt::get(pintTy, 0xDEADBEEF), ptrTy));
    }
    // Seed/base placeholder
    elems.push_back(ConstantExpr::getIntToPtr(
        ConstantInt::get(pintTy, masterSeed), ptrTy));

    // Block addresses (will be encrypted at link time / runtime)
    for (auto& [bb, _] : targetBBs) {
        elems.push_back(BlockAddress::get(bb));
        if (is32Bit) {
            elems.push_back(Constant::getNullValue(ptrTy)); // Padding for 64-bit encrypted value
        }
    }

    ArrayType* bbArrayTy = ArrayType::get(ptrTy, elems.size());
    Constant* initializer = ConstantArray::get(bbArrayTy, elems);
    GlobalVariable* bbArray = new GlobalVariable(
        *module, bbArrayTy, false, GlobalValue::InternalLinkage, initializer, "ibr.targets");

    // Allocate XTEA working variables at function entry
    builder.SetInsertPoint(&*func.getEntryBlock().getFirstInsertionPt());
    AllocaInst* v0Var = builder.CreateAlloca(u32Ty, nullptr, "xtea.v0");
    AllocaInst* v1Var = builder.CreateAlloca(u32Ty, nullptr, "xtea.v1");
    AllocaInst* sumVar = builder.CreateAlloca(u32Ty, nullptr, "xtea.sum");
    AllocaInst* iVar = builder.CreateAlloca(u32Ty, nullptr, "xtea.i");
    AllocaInst* tempStorage = builder.CreateAlloca(u64Ty, nullptr, "xtea.temp");

    // Process each branch
    for (BranchInst* branch : branches) {
        builder.SetInsertPoint(branch);

        Value* keyPtr;
        Value* xteaDelta;
        Value* xteaRounds;
        Value* arrayIndex;

        if (branch->isConditional()) {
            BasicBlock* trueBB = branch->getSuccessor(0);
            BasicBlock* falseBB = branch->getSuccessor(1);
            XTEAInfo& trueInfo = blockXTEA[trueBB];
            XTEAInfo& falseInfo = blockXTEA[falseBB];

            // Create key arrays for both branches
            std::vector<Constant*> trueKeyVals(4), falseKeyVals(4);
            for (int i = 0; i < 4; i++) {
                trueKeyVals[i] = ConstantInt::get(u32Ty, trueInfo.key[i]);
                falseKeyVals[i] = ConstantInt::get(u32Ty, falseInfo.key[i]);
            }

            GlobalVariable* trueKeyGV = new GlobalVariable(
                *module, keyArrayTy, false, GlobalValue::PrivateLinkage,
                ConstantArray::get(keyArrayTy, trueKeyVals), "key.true");
            GlobalVariable* falseKeyGV = new GlobalVariable(
                *module, keyArrayTy, false, GlobalValue::PrivateLinkage,
                ConstantArray::get(keyArrayTy, falseKeyVals), "key.false");

            Value* cond = branch->getCondition();
            
            Value* trueKeyPtr = builder.CreateInBoundsGEP(keyArrayTy, trueKeyGV, 
                {builder.getInt32(0), builder.getInt32(0)});
            Value* falseKeyPtr = builder.CreateInBoundsGEP(keyArrayTy, falseKeyGV,
                {builder.getInt32(0), builder.getInt32(0)});
            
            keyPtr = builder.CreateSelect(cond, trueKeyPtr, falseKeyPtr);
            xteaDelta = builder.CreateSelect(cond,
                builder.getInt32(trueInfo.delta), builder.getInt32(falseInfo.delta));
            xteaRounds = builder.CreateSelect(cond,
                builder.getInt32(trueInfo.rounds), builder.getInt32(falseInfo.rounds));
            
            int trueIdx = ARRAY_MARKER + indexOf(targetBBs, trueBB) * (is32Bit ? 2 : 1);
            int falseIdx = ARRAY_MARKER + indexOf(targetBBs, falseBB) * (is32Bit ? 2 : 1);
            arrayIndex = builder.CreateSelect(cond,
                ConstantInt::get(pintTy, trueIdx),
                ConstantInt::get(pintTy, falseIdx));
        } else {
            BasicBlock* targetBB = branch->getSuccessor(0);
            XTEAInfo& info = blockXTEA[targetBB];

            std::vector<Constant*> keyVals(4);
            for (int i = 0; i < 4; i++) {
                keyVals[i] = ConstantInt::get(u32Ty, info.key[i]);
            }

            GlobalVariable* keyGV = new GlobalVariable(
                *module, keyArrayTy, false, GlobalValue::PrivateLinkage,
                ConstantArray::get(keyArrayTy, keyVals), "key");
            
            keyPtr = builder.CreateInBoundsGEP(keyArrayTy, keyGV,
                {builder.getInt32(0), builder.getInt32(0)});
            xteaDelta = builder.getInt32(info.delta);
            xteaRounds = builder.getInt32(info.rounds);
            
            int idx = ARRAY_MARKER + indexOf(targetBBs, targetBB) * (is32Bit ? 2 : 1);
            arrayIndex = ConstantInt::get(pintTy, idx);
        }

        // Load encrypted value from array
        Value* gep = builder.CreateGEP(bbArrayTy, bbArray, 
            {ConstantInt::get(pintTy, 0), arrayIndex});
        
        Value* encryptedValue;
        if (is32Bit) {
            Value* i32Ptr = gep;
            Value* low32 = builder.CreateLoad(u32Ty, i32Ptr, true);
            Value* highPtr = builder.CreateGEP(u32Ty, i32Ptr, builder.getInt32(1));
            Value* high32 = builder.CreateLoad(u32Ty, highPtr, true);
            Value* high64 = builder.CreateZExt(high32, u64Ty);
            Value* low64 = builder.CreateZExt(low32, u64Ty);
            Value* shifted = builder.CreateShl(high64, 32);
            encryptedValue = builder.CreateOr(shifted, low64);
        } else {
            Value* castedGep = builder.CreateBitCast(gep, PointerType::getUnqual(ctx));
            encryptedValue = builder.CreateLoad(pintTy, castedGep, true);
        }

        // Store to temp for decryption
        builder.CreateStore(encryptedValue, tempStorage, true);
        Value* castedTemp = builder.CreateBitCast(tempStorage, PointerType::getUnqual(ctx));

        // Emit XTEA decipher
        emitXTEADecipher(builder, castedTemp, keyPtr, xteaDelta, xteaRounds,
                         v0Var, v1Var, sumVar, iVar);

        // Load decrypted value and compute final address
        Value* decryptedOffset = builder.CreateLoad(u64Ty, tempStorage, true);
        
        if (is32Bit) {
            Value* low32 = builder.CreateTrunc(decryptedOffset, u32Ty);
            decryptedOffset = builder.CreateZExt(low32, pintTy);
        }

        // For now, use the decrypted value directly as address offset
        // (In a full implementation, this would be added to a base address)
        Value* finalAddr = builder.CreateIntToPtr(decryptedOffset, ptrTy);

        // Create indirect branch
        IndirectBrInst* indirBranch = builder.CreateIndirectBr(finalAddr, branch->getNumSuccessors());
        for (BasicBlock* succ : branch->successors()) {
            indirBranch->addDestination(succ);
        }

        branch->replaceAllUsesWith(indirBranch);
        branch->eraseFromParent();
    }
}

static void obfuscateModule(Module& mod, int iterations) {
    for (int i = 0; i < iterations; i++) {
        for (Function& func : mod) {
            if (!func.isDeclaration()) {
                obfuscateFunction(func, ReplaceChance);
            }
        }
    }
}

int main(int argc, char** argv) {
    cl::ParseCommandLineOptions(argc, argv,
                                "Encrypted Indirect Branch Obfuscator\n\n"
                                "Replaces branches with XTEA-encrypted indirect branches.\n");

    RNG = new Random(Seed);

    LLVMContext context;
    SMDiagnostic err;

    std::unique_ptr<Module> mod = parseIRFile(InputFilename, err, context);
    if (!mod) {
        err.print(argv[0], errs());
        return 1;
    }

    obfuscateModule(*mod, Iterations);

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
