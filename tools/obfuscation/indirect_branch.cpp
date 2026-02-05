/**
 * Simple Indirect Branch Tool
 *
 * Replaces direct branches with indirect branches through a block address
 * array, making control flow analysis harder.
 *
 * Implements simple indirect branch obfuscation.
 *
 * Usage:
 *   indirect_branch [options] <input.bc> <output.bc>
 *
 * Options:
 *   --iterations N    Number of iterations (default: 1)
 *   --chance N        Percent chance to replace branches (default: 50)
 *   --seed N          Random seed for reproducibility (default: random)
 *   --help            Show this help
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
#include <functional>
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

    uint64_t uint64() {
        std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
        return dist(rng_);
    }

private:
    std::mt19937 rng_;
};

static Random* RNG = nullptr;

// MBA XOR obfuscation for the index computation
// a ^ b = (~a & b) | (a & ~b)
static Value* obfuscateXor(IRBuilder<>& builder, Value* a, Value* b) {
    return builder.CreateOr(
        builder.CreateAnd(builder.CreateNot(a), b),
        builder.CreateAnd(a, builder.CreateNot(b)));
}

// Compute a fake index that ends up being the same as the input
// index ^ rand ^ rand = index
static Value* computeFakeIndex(IRBuilder<>& builder, Value* index) {
    Type* intTy = index->getType();
    unsigned bitWidth = intTy->getIntegerBitWidth();

    uint64_t randVal = bitWidth <= 32 ? RNG->uint32() : RNG->uint64();
    Constant* rand = ConstantInt::get(intTy, randVal);

    // First XOR with random value
    Value* xor1 = obfuscateXor(builder, index, rand);
    // XOR again with same random value to get back original
    return obfuscateXor(builder, xor1, rand);
}

static void obfuscateFunction(Function& func, int replaceChance) {
    if (func.size() < 2) {
        return;
    }

    int branchCount = std::accumulate(
        func.begin(), func.end(), 0,
        [](int acc, const BasicBlock& bb) {
            return acc + (bb.getTerminator()->getOpcode() == Instruction::Br ? 1 : 0);
        });

    if (branchCount == 0) {
        return;
    }

    BasicBlock* entry = &func.getEntryBlock();
    IRBuilder<> builder(&*entry->getFirstInsertionPt());

    Type* i8PtrTy = PointerType::getUnqual(func.getContext());
    ArrayType* blocksArrayTy = ArrayType::get(i8PtrTy, 2);

    Value* blocksArray = builder.CreateAlloca(blocksArrayTy, nullptr, "ibr.blocks");

    // Collect branches to process
    std::vector<BranchInst*> branches;
    for (BasicBlock& bb : func) {
        if (auto* branch = dyn_cast<BranchInst>(bb.getTerminator())) {
            if (RNG->chance(replaceChance)) {
                branches.push_back(branch);
            }
        }
    }

    for (BranchInst* branch : branches) {
        builder.SetInsertPoint(branch);

        // Store block addresses into the array
        for (unsigned i = 0; i < branch->getNumSuccessors(); ++i) {
            Value* indices[] = {builder.getInt32(0), builder.getInt32(i)};
            Value* slot = builder.CreateGEP(blocksArrayTy, blocksArray, indices);
            builder.CreateStore(
                BlockAddress::get(branch->getFunction(), branch->getSuccessor(i)),
                slot, true);
        }

        Value* index;
        if (branch->isConditional()) {
            // Invert condition: true -> 0, false -> 1 (matching successor order)
            Value* invertedCond = builder.CreateNot(branch->getCondition());
            index = builder.CreateZExt(invertedCond, builder.getInt32Ty());
        } else {
            index = builder.getInt32(0);
        }

        // Obfuscate the index computation
        index = computeFakeIndex(builder, index);

        // Load the target address
        Value* indices[] = {builder.getInt32(0), index};
        Value* gep = builder.CreateGEP(blocksArrayTy, blocksArray, indices);
        Value* targetAddr = builder.CreateLoad(i8PtrTy, gep, true);

        // Create indirect branch
        IndirectBrInst* indirBranch = builder.CreateIndirectBr(
            targetAddr, branch->getNumSuccessors());

        for (BasicBlock* successor : branch->successors()) {
            indirBranch->addDestination(successor);
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
                                "Simple Indirect Branch Obfuscator\n\n"
                                "Replaces direct branches with indirect branches.\n");

    // Initialize RNG
    RNG = new Random(Seed);

    // Load input module
    LLVMContext context;
    SMDiagnostic err;

    std::unique_ptr<Module> mod = parseIRFile(InputFilename, err, context);
    if (!mod) {
        err.print(argv[0], errs());
        return 1;
    }

    // Apply obfuscation
    obfuscateModule(*mod, Iterations);

    // Write output
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
