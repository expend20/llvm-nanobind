/**
 * Basic Block Splitter Tool
 *
 * Splits large basic blocks into smaller ones to increase control flow
 * complexity and make analysis harder.
 *
 * Implements basic block splitting for control flow obfuscation.
 *
 * Usage:
 *   bb_split [options] <input.bc> <output.bc>
 *
 * Options:
 *   --iterations N      Number of iterations (default: 1)
 *   --min-size N        Minimum block size to consider for splitting (default: 10)
 *   --max-size N        Maximum block size after splitting (default: 20)
 *   --chance N          Percent chance to split eligible blocks (default: 40)
 *   --seed N            Random seed for reproducibility (default: random)
 *   --help              Show this help
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
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <random>
#include <vector>
#include <algorithm>

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

static cl::opt<int> MinBlockSize("min-size",
                                 cl::desc("Minimum block size to split (default: 10)"),
                                 cl::init(10));

static cl::opt<int> MaxBlockSize("max-size",
                                 cl::desc("Maximum block size after split (default: 20)"),
                                 cl::init(20));

static cl::opt<int> SplitChance("chance",
                                cl::desc("Percent chance to split (default: 40)"),
                                cl::init(40));

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

private:
    std::mt19937 rng_;
};

static Random* RNG = nullptr;

static void splitBlock(BasicBlock* bb, int maxBlockSize) {
    if (!bb || bb->size() <= static_cast<size_t>(maxBlockSize)) {
        return;
    }

    // Use worklist to handle nested splits
    std::vector<BasicBlock*> workList;
    workList.push_back(bb);

    while (!workList.empty()) {
        BasicBlock* current = workList.back();
        workList.pop_back();

        // Skip blocks that are already small enough
        if (current->size() <= static_cast<size_t>(maxBlockSize)) {
            continue;
        }

        // Get iterators for non-PHI instructions
        BasicBlock::iterator startIt = current->getFirstNonPHIIt();
        BasicBlock::iterator endIt = current->end();
        size_t insCount = std::distance(startIt, endIt);

        // Cannot split if no valid split points
        if (insCount < 2) {
            continue;
        }

        // Calculate target split size
        size_t targetSize = std::min(static_cast<size_t>(maxBlockSize) - 1, insCount - 1);
        if (targetSize == 0) {
            continue;
        }

        // Find valid split point
        BasicBlock::iterator splitIt = startIt;
        std::advance(splitIt, targetSize);

        // Adjust backward if at terminator
        if (splitIt->isTerminator()) {
            while (splitIt != startIt && splitIt->isTerminator()) {
                --splitIt;
            }
            // Skip if no valid split found
            if (splitIt->isTerminator()) {
                continue;
            }
        }

        // Perform the split
        BasicBlock* newBlock = current->splitBasicBlock(&*splitIt);

        // Add both blocks back to worklist for further splitting
        workList.push_back(current);
        workList.push_back(newBlock);
    }
}

static void obfuscateFunction(Function& func, int minBlockSize, int maxBlockSize, int splitChance) {
    std::vector<BasicBlock*> blocks;
    BasicBlock* largestBlock = nullptr;

    for (BasicBlock& bb : func) {
        size_t blockSize = bb.size();
        if (blockSize >= static_cast<size_t>(minBlockSize)) {
            largestBlock = &bb;
            if (RNG->chance(splitChance)) {
                blocks.push_back(&bb);
            }
        }
    }

    // Use the largest block if no candidates found
    if (blocks.empty() && largestBlock) {
        blocks.push_back(largestBlock);
    }

    // Split all candidate blocks to enforce MaxBlockSize
    for (BasicBlock* bb : blocks) {
        splitBlock(bb, maxBlockSize);
    }
}

static void obfuscateModule(Module& mod, int iterations) {
    for (int i = 0; i < iterations; i++) {
        for (Function& func : mod) {
            if (!func.isDeclaration()) {
                obfuscateFunction(func, MinBlockSize, MaxBlockSize, SplitChance);
            }
        }
    }
}

int main(int argc, char** argv) {
    cl::ParseCommandLineOptions(argc, argv,
                                "Basic Block Splitter\n\n"
                                "Splits large basic blocks into smaller ones.\n");

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
