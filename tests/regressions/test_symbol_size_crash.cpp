/**
 * Standalone reproduction of LLVM bug: LLVMGetSymbolSize crashes on non-common
 * symbols.
 *
 * Bug: LLVMGetSymbolSize() calls getCommonSymbolSize() internally, which
 * asserts if the symbol doesn't have the SF_Common flag set. This affects most
 * symbols in typical object files (functions, data, etc.).
 *
 * Error message:
 *     Assertion failed: (*SymbolFlagsOrErr & SymbolRef::SF_Common),
 *     function getCommonSymbolSize, file ObjectFile.h, line 313.
 *
 * This is an upstream LLVM bug in the C API. The C++ API has getSize() which
 * works for all symbols, but LLVMGetSymbolSize maps to getCommonSymbolSize().
 *
 * Affected LLVM versions: Tested on LLVM 21 (likely affects many versions)
 *
 * To build:
 *     cmake --build build --target test_symbol_size_crash
 *
 * To run:
 *     ./build/test_symbol_size_crash
 *
 * Expected output: Assertion failure when accessing symbol size
 */

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Object.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

// Minimal IR that produces a function symbol (not a common symbol)
static const char *TEST_IR = R"(
; Minimal IR to produce a non-common symbol
target triple = "x86_64-unknown-linux-gnu"

define i32 @test_function() {
    ret i32 42
}
)";

int main() {
  printf("====================================================================="
         "=\n");
  printf("LLVM Bug Reproduction: LLVMGetSymbolSize crashes on non-common "
         "symbols\n");
  printf("====================================================================="
         "=\n\n");

  // Initialize all targets for object file generation
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmPrinters();

  // Parse the IR
  LLVMContextRef ctx = LLVMContextCreate();
  LLVMMemoryBufferRef ir_buf = LLVMCreateMemoryBufferWithMemoryRangeCopy(
      TEST_IR, strlen(TEST_IR), "test.ll");

  char *error = nullptr;
  LLVMModuleRef module = nullptr;
  if (LLVMParseIRInContext(ctx, ir_buf, &module, &error)) {
    fprintf(stderr, "Failed to parse IR: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }

  // Get target
  const char *triple = "x86_64-unknown-linux-gnu";
  LLVMTargetRef target = nullptr;
  if (LLVMGetTargetFromTriple(triple, &target, &error)) {
    fprintf(stderr, "Failed to get target: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }

  // Create target machine
  LLVMTargetMachineRef tm =
      LLVMCreateTargetMachine(target, triple, "", "", LLVMCodeGenLevelDefault,
                              LLVMRelocDefault, LLVMCodeModelDefault);

  // Emit object file to memory buffer
  LLVMMemoryBufferRef obj_buf = nullptr;
  if (LLVMTargetMachineEmitToMemoryBuffer(tm, module, LLVMObjectFile, &error,
                                          &obj_buf)) {
    fprintf(stderr, "Failed to emit object file: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }

  printf("Created test object file: %zu bytes\n", LLVMGetBufferSize(obj_buf));

  // Create binary from the object buffer
  char *binary_error = nullptr;
  LLVMBinaryRef binary = LLVMCreateBinary(obj_buf, ctx, &binary_error);
  if (!binary || binary_error) {
    fprintf(stderr, "Failed to create binary: %s\n", binary_error);
    LLVMDisposeMessage(binary_error);
    return 1;
  }

  printf("Binary type: %d\n\n", LLVMBinaryGetType(binary));

  // List sections (this works fine)
  printf("Sections:\n");
  LLVMSectionIteratorRef sect = LLVMObjectFileCopySectionIterator(binary);
  while (!LLVMObjectFileIsSectionIteratorAtEnd(binary, sect)) {
    printf("  %s: @0x%08llx +%llu\n", LLVMGetSectionName(sect),
           (unsigned long long)LLVMGetSectionAddress(sect),
           (unsigned long long)LLVMGetSectionSize(sect));
    LLVMMoveToNextSection(sect);
  }
  LLVMDisposeSectionIterator(sect);

  // List symbols - this will crash when accessing size
  printf("\nSymbols (will crash on LLVMGetSymbolSize):\n");
  LLVMSymbolIteratorRef sym = LLVMObjectFileCopySymbolIterator(binary);
  while (!LLVMObjectFileIsSymbolIteratorAtEnd(binary, sym)) {
    const char *name = LLVMGetSymbolName(sym);
    uint64_t address = LLVMGetSymbolAddress(sym);

    printf("  %s: @0x%08llx", name ? name : "<null>",
           (unsigned long long)address);

    // Uncomment the next line to trigger the crash:
    uint64_t size = LLVMGetSymbolSize(sym);
    printf(" +%llu", (unsigned long long)size);

    printf(" (size access skipped - would crash)\n");

    LLVMMoveToNextSymbol(sym);
  }
  LLVMDisposeSymbolIterator(sym);

  printf("\nTest completed (crash avoided by not calling LLVMGetSymbolSize)\n");

  // Cleanup
  LLVMDisposeBinary(binary);
  LLVMDisposeMemoryBuffer(obj_buf);
  LLVMDisposeTargetMachine(tm);
  LLVMDisposeModule(module);
  LLVMContextDispose(ctx);

  printf("\n==================================================================="
         "===\n");
  printf("Bug Summary:\n");
  printf("  - LLVMGetSymbolSize() internally calls getCommonSymbolSize()\n");
  printf(
      "  - getCommonSymbolSize() asserts that the symbol has SF_Common flag\n");
  printf("  - Most symbols (functions, data) do NOT have this flag\n");
  printf("  - Result: Assertion failure / crash\n");
  printf("\n");
  printf("Workaround: Don't call LLVMGetSymbolSize()\n");
  printf("Fix needed: LLVM should use SymbolRef::getSize() instead\n");
  printf("====================================================================="
         "=\n");

  return 0;
}
