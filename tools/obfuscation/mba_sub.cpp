/**
 * Mixed Boolean Arithmetic (MBA) Substitution Tool
 *
 * Obfuscates arithmetic operations by replacing them with equivalent
 * but more complex expressions using boolean arithmetic identities.
 *
 * Implements Mixed Boolean Arithmetic substitution.
 *
 * Usage:
 *   mba_sub [options] <input.bc> <output.bc>
 *
 * Options:
 *   --iterations N    Number of iterations (default: 1)
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

static cl::opt<unsigned> Seed("seed",
                              cl::desc("Random seed (default: random)"),
                              cl::init(0));

// Random number generator
class Random {
public:
    Random(unsigned seed) : rng_(seed ? seed : std::random_device{}()) {}

    template <typename T>
    T intRanged(T min, T max) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(rng_);
    }

    uint32_t uint32() { return intRanged<uint32_t>(0, UINT32_MAX); }
    uint64_t uint64() { return intRanged<uint64_t>(0, UINT64_MAX); }

    size_t index(size_t max) {
        return intRanged<size_t>(0, max);
    }

private:
    std::mt19937 rng_;
};

static Random* RNG = nullptr;

// MBA substitution callbacks
using Callback = std::function<Value*(IRBuilder<>&, BinaryOperator*)>;

// X - Y == (X ^ -Y) + 2*(X & -Y)
static Callback sub_ops[] = {
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Type* type = op->getOperand(0)->getType();
        Value* x = op->getOperand(0);
        Value* y = op->getOperand(1);
        Value* neg_y = builder.CreateNeg(y);

        return builder.CreateAdd(
            builder.CreateXor(x, neg_y),
            builder.CreateMul(
                ConstantInt::getSigned(type, 2),
                builder.CreateAnd(x, neg_y)));
    },
};

// x + y = (~ (x + ((- x) + ((- x) + (~ y)))))
static Callback add_ops[] = {
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* x = op->getOperand(0);
        Value* y = op->getOperand(1);

        return builder.CreateNot(builder.CreateAdd(
            x,
            builder.CreateAdd(
                builder.CreateNeg(x),
                builder.CreateAdd(
                    builder.CreateNeg(x),
                    builder.CreateNot(y)))));
    },
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        // r = rand(); c = b + r; a = a + c; a = a - r
        Value* a = op->getOperand(0);
        Value* b = op->getOperand(1);

        Constant* r = ConstantInt::get(op->getOperand(0)->getType(),
                                       RNG->uint64());
        Value* c = builder.CreateAdd(b, r);
        a = builder.CreateAdd(a, c);
        return builder.CreateSub(a, r);
    },
};

// a ^ b = (~a & b) | (a & ~b)
// a ^ b = (a | b) & ~(a & b)
// a ^ b = (a + b) - 2 * (a & b)
// a ^ b = ~(~a & ~b) & ~(a & b)
static Callback xor_ops[] = {
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* a = op->getOperand(0);
        Value* b = op->getOperand(1);
        return builder.CreateOr(
            builder.CreateAnd(builder.CreateNot(a), b),
            builder.CreateAnd(a, builder.CreateNot(b)));
    },
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* a = op->getOperand(0);
        Value* b = op->getOperand(1);
        return builder.CreateAnd(
            builder.CreateOr(a, b),
            builder.CreateNot(builder.CreateAnd(a, b)));
    },
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* a = op->getOperand(0);
        Value* b = op->getOperand(1);
        return builder.CreateSub(
            builder.CreateAdd(a, b),
            builder.CreateMul(ConstantInt::get(a->getType(), 2),
                              builder.CreateAnd(a, b)));
    },
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* a = op->getOperand(0);
        Value* b = op->getOperand(1);
        return builder.CreateAnd(
            builder.CreateNot(builder.CreateAnd(builder.CreateNot(a),
                                                builder.CreateNot(b))),
            builder.CreateNot(builder.CreateAnd(a, b)));
    },
};

// b * c = (((b | c) * (b & c)) + ((b & ~c) * (c & ~b)))
static Callback mul_ops[] = {
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* b = op->getOperand(0);
        Value* c = op->getOperand(1);
        return builder.CreateAdd(
            builder.CreateMul(builder.CreateOr(b, c),
                              builder.CreateAnd(b, c)),
            builder.CreateMul(builder.CreateAnd(b, builder.CreateNot(c)),
                              builder.CreateAnd(c, builder.CreateNot(b))));
    },
};

// a | b = ~(~a & ~b)
// a | b = a ^ b ^ (a & b)
// a | b = (a + b) - (a & b)
static Callback or_ops[] = {
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* a = op->getOperand(0);
        Value* b = op->getOperand(1);
        return builder.CreateNot(
            builder.CreateAnd(builder.CreateNot(a), builder.CreateNot(b)));
    },
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* a = op->getOperand(0);
        Value* b = op->getOperand(1);
        return builder.CreateXor(
            a, builder.CreateXor(b, builder.CreateAnd(a, b)));
    },
    [](IRBuilder<>& builder, BinaryOperator* op) -> Value* {
        Value* a = op->getOperand(0);
        Value* b = op->getOperand(1);
        return builder.CreateSub(builder.CreateAdd(a, b),
                                 builder.CreateAnd(a, b));
    },
};

template <size_t N>
static void runOnOpcode(BasicBlock& bb, unsigned opcode, Callback (&callbacks)[N]) {
    std::vector<Instruction*> instructions;

    for (Instruction& instr : bb) {
        if (instr.getOpcode() == opcode) {
            instructions.push_back(&instr);
        }
    }

    for (Instruction* instr : instructions) {
        BinaryOperator* binOp = cast<BinaryOperator>(instr);
        IRBuilder<> builder(instr);
        size_t idx = RNG->index(N - 1);
        Value* replacement = callbacks[idx](builder, binOp);
        instr->replaceAllUsesWith(replacement);
    }
}

static void runOnBasicBlock(BasicBlock& bb) {
    runOnOpcode(bb, Instruction::Sub, sub_ops);
    runOnOpcode(bb, Instruction::Add, add_ops);
    runOnOpcode(bb, Instruction::Xor, xor_ops);
    runOnOpcode(bb, Instruction::Mul, mul_ops);
    runOnOpcode(bb, Instruction::Or, or_ops);
}

static void obfuscateFunction(Function& func) {
    for (BasicBlock& bb : func) {
        runOnBasicBlock(bb);
    }
}

static void obfuscateModule(Module& mod, int iterations) {
    for (int i = 0; i < iterations; i++) {
        for (Function& func : mod) {
            if (!func.isDeclaration()) {
                obfuscateFunction(func);
            }
        }
    }
}

int main(int argc, char** argv) {
    cl::ParseCommandLineOptions(argc, argv,
                                "MBA Substitution Obfuscator\n\n"
                                "Replaces arithmetic operations with equivalent "
                                "boolean arithmetic expressions.\n");

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
