/**
 * Test: test_module
 * Tests LLVM Module creation and properties
 *
 * LLVM-C APIs covered:
 * - LLVMModuleCreateWithNameInContext()
 * - LLVMGetModuleIdentifier() / LLVMSetModuleIdentifier()
 * - LLVMGetSourceFileName() / LLVMSetSourceFileName()
 * - LLVMGetDataLayoutStr() / LLVMSetDataLayout()
 * - LLVMGetTarget() / LLVMSetTarget()
 * - LLVMPrintModuleToString()
 * - LLVMCloneModule()
 * - LLVMDisposeModule()
 */

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <cstdio>
#include <cstring>
#include <string>

int main() {
    LLVMContextRef ctx = LLVMContextCreate();

    // Create module
    LLVMModuleRef mod = LLVMModuleCreateWithNameInContext("test_module", ctx);

    // Get initial module identifier (copy to string before modifying)
    size_t id_len;
    const char *id_ptr = LLVMGetModuleIdentifier(mod, &id_len);
    std::string initial_id(id_ptr, id_len);

    // Set new module identifier
    LLVMSetModuleIdentifier(mod, "renamed_module", strlen("renamed_module"));
    id_ptr = LLVMGetModuleIdentifier(mod, &id_len);
    std::string new_id(id_ptr, id_len);

    // Get/set source filename (copy to string before modifying)
    size_t src_len;
    const char *src_ptr = LLVMGetSourceFileName(mod, &src_len);
    std::string initial_src(src_ptr, src_len);
    LLVMSetSourceFileName(mod, "test_source.c", strlen("test_source.c"));
    src_ptr = LLVMGetSourceFileName(mod, &src_len);
    std::string new_src(src_ptr, src_len);

    // Get/set data layout (copy to string before modifying)
    std::string initial_layout = LLVMGetDataLayoutStr(mod);
    LLVMSetDataLayout(mod, "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");
    std::string new_layout = LLVMGetDataLayoutStr(mod);

    // Get/set target triple (copy to string before modifying)
    std::string initial_target = LLVMGetTarget(mod);
    LLVMSetTarget(mod, "x86_64-unknown-linux-gnu");
    std::string new_target = LLVMGetTarget(mod);

    // Clone module
    LLVMModuleRef cloned = LLVMCloneModule(mod);
    id_ptr = LLVMGetModuleIdentifier(cloned, &id_len);
    std::string cloned_id(id_ptr, id_len);

    // Verify module
    char *error = nullptr;
    if (LLVMVerifyModule(mod, LLVMReturnStatusAction, &error)) {
        fprintf(stderr, "; Verification failed: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeModule(cloned);
        LLVMDisposeModule(mod);
        LLVMContextDispose(ctx);
        return 1;
    }
    LLVMDisposeMessage(error);

    // Print diagnostic comments
    printf("; Test: test_module\n");
    printf("; Initial module ID: %s\n", initial_id.c_str());
    printf("; New module ID: %s\n", new_id.c_str());
    printf("; Initial source filename: %s\n", initial_src.c_str());
    printf("; New source filename: %s\n", new_src.c_str());
    printf("; Initial data layout: %s\n", initial_layout.empty() ? "(empty)" : initial_layout.c_str());
    printf("; New data layout: %s\n", new_layout.c_str());
    printf("; Initial target: %s\n", initial_target.empty() ? "(empty)" : initial_target.c_str());
    printf("; New target: %s\n", new_target.c_str());
    printf("; Cloned module ID: %s\n", cloned_id.c_str());
    printf("\n");

    // Print module IR
    char *ir = LLVMPrintModuleToString(mod);
    printf("%s", ir);
    LLVMDisposeMessage(ir);

    // Cleanup
    LLVMDisposeModule(cloned);
    LLVMDisposeModule(mod);
    LLVMContextDispose(ctx);

    return 0;
}
