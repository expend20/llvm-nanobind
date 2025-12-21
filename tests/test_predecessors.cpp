/**
 * Test: test_predecessors
 * Tests LLVM BasicBlock predecessors and successors
 *
 * LLVM-C APIs covered:
 * - LLVMGetBasicBlockTerminator()
 * - LLVMGetNumSuccessors(), LLVMGetSuccessor()
 * - LLVMBasicBlockAsValue()
 * - LLVMGetFirstUse(), LLVMGetNextUse()
 * - LLVMGetUser()
 * - LLVMIsATerminatorInst()
 * - LLVMGetInstructionParent()
 */

#include <cstdio>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <vector>

// Helper function to get successors of a basic block
std::vector<LLVMBasicBlockRef> get_successors(LLVMBasicBlockRef bb) {
  std::vector<LLVMBasicBlockRef> result;
  LLVMValueRef term = LLVMGetBasicBlockTerminator(bb);
  if (!term) {
    printf("Warning: Basic block has no terminator instruction.\n");
    return result;
  }
  unsigned num = LLVMGetNumSuccessors(term);
  for (unsigned i = 0; i < num; ++i) {
    result.push_back(LLVMGetSuccessor(term, i));
  }
  return result;
}

// Helper function to get predecessors of a basic block
// Uses use-def chain iteration as described in devdocs/predecessors.md
std::vector<LLVMBasicBlockRef> get_predecessors(LLVMBasicBlockRef bb) {
  std::vector<LLVMBasicBlockRef> result;
  LLVMValueRef block_value = LLVMBasicBlockAsValue(bb);

  for (LLVMUseRef use = LLVMGetFirstUse(block_value); use != nullptr;
       use = LLVMGetNextUse(use)) {
    LLVMValueRef user = LLVMGetUser(use);
    if (LLVMIsATerminatorInst(user)) {
      LLVMBasicBlockRef pred = LLVMGetInstructionParent(user);
      result.push_back(pred);
    }
  }
  return result;
}

int main() {
  LLVMContextRef ctx = LLVMContextCreate();
  LLVMModuleRef mod =
      LLVMModuleCreateWithNameInContext("test_predecessors", ctx);

  LLVMTypeRef i1 = LLVMInt1TypeInContext(ctx);
  LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx);
  LLVMTypeRef void_ty = LLVMVoidTypeInContext(ctx);

  LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

  // ==========================================
  // Function: diamond pattern (tests multiple predecessors)
  // void diamond(i1 cond)
  // entry -> (if_true | if_false) -> merge
  // ==========================================
  LLVMTypeRef diamond_params[] = {i1};
  LLVMTypeRef diamond_ty = LLVMFunctionType(void_ty, diamond_params, 1, 0);
  LLVMValueRef diamond_func = LLVMAddFunction(mod, "diamond", diamond_ty);

  LLVMValueRef cond = LLVMGetParam(diamond_func, 0);
  LLVMSetValueName2(cond, "cond", 4);

  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(ctx, diamond_func, "entry");
  LLVMBasicBlockRef if_true =
      LLVMAppendBasicBlockInContext(ctx, diamond_func, "if_true");
  LLVMBasicBlockRef if_false =
      LLVMAppendBasicBlockInContext(ctx, diamond_func, "if_false");
  LLVMBasicBlockRef merge =
      LLVMAppendBasicBlockInContext(ctx, diamond_func, "merge");

  // Entry: conditional branch to if_true or if_false
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMBuildCondBr(builder, cond, if_true, if_false);

  // True branch: branch to merge
  LLVMPositionBuilderAtEnd(builder, if_true);
  LLVMBuildBr(builder, merge);

  // False branch: branch to merge
  LLVMPositionBuilderAtEnd(builder, if_false);
  LLVMBuildBr(builder, merge);

  // Merge: return
  LLVMPositionBuilderAtEnd(builder, merge);
  LLVMBuildRetVoid(builder);

  // ==========================================
  // Function: loop pattern (tests self-referential predecessor)
  // void loop(i32 n)
  // entry -> loop -> (loop | exit)
  // ==========================================
  LLVMTypeRef loop_params[] = {i32};
  LLVMTypeRef loop_ty = LLVMFunctionType(void_ty, loop_params, 1, 0);
  LLVMValueRef loop_func = LLVMAddFunction(mod, "loop", loop_ty);

  LLVMValueRef n = LLVMGetParam(loop_func, 0);
  LLVMSetValueName2(n, "n", 1);

  LLVMBasicBlockRef loop_entry =
      LLVMAppendBasicBlockInContext(ctx, loop_func, "entry");
  LLVMBasicBlockRef loop_body =
      LLVMAppendBasicBlockInContext(ctx, loop_func, "loop");
  LLVMBasicBlockRef loop_exit =
      LLVMAppendBasicBlockInContext(ctx, loop_func, "exit");

  // Entry: branch to loop
  LLVMPositionBuilderAtEnd(builder, loop_entry);
  LLVMBuildBr(builder, loop_body);

  // Loop: PHI, compare, conditional branch back to loop or to exit
  LLVMPositionBuilderAtEnd(builder, loop_body);
  LLVMValueRef i_phi = LLVMBuildPhi(builder, i32, "i");
  LLVMValueRef new_i =
      LLVMBuildAdd(builder, i_phi, LLVMConstInt(i32, 1, 0), "new_i");
  LLVMValueRef loop_cond =
      LLVMBuildICmp(builder, LLVMIntSLT, new_i, n, "loop_cond");
  LLVMBuildCondBr(builder, loop_cond, loop_body, loop_exit);

  // Add incoming values to PHI
  LLVMValueRef i_incoming_vals[] = {LLVMConstInt(i32, 0, 0), new_i};
  LLVMBasicBlockRef i_incoming_blocks[] = {loop_entry, loop_body};
  LLVMAddIncoming(i_phi, i_incoming_vals, i_incoming_blocks, 2);

  // Exit: return
  LLVMPositionBuilderAtEnd(builder, loop_exit);
  LLVMBuildRetVoid(builder);

  LLVMDisposeBuilder(builder);

  // Verify module
  char *error = nullptr;
  if (LLVMVerifyModule(mod, LLVMReturnStatusAction, &error)) {
    fprintf(stderr, "; Verification failed: %s\n", error);
    LLVMDisposeMessage(error);
    LLVMDisposeModule(mod);
    LLVMContextDispose(ctx);
    return 1;
  }
  LLVMDisposeMessage(error);

  // Print diagnostic comments
  printf("; Test: test_predecessors\n");
  printf(";\n");
  printf("; Diamond pattern:\n");

  // Entry block
  auto entry_succs = get_successors(entry);
  auto entry_preds = get_predecessors(entry);
  printf(";   entry:\n");
  printf(";     successors: [");
  for (size_t i = 0; i < entry_succs.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(entry_succs[i]));
  }
  printf("]\n");
  printf(";     predecessors: [");
  for (size_t i = 0; i < entry_preds.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(entry_preds[i]));
  }
  printf("]\n");

  // if_true block
  auto if_true_succs = get_successors(if_true);
  auto if_true_preds = get_predecessors(if_true);
  printf(";   if_true:\n");
  printf(";     successors: [");
  for (size_t i = 0; i < if_true_succs.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(if_true_succs[i]));
  }
  printf("]\n");
  printf(";     predecessors: [");
  for (size_t i = 0; i < if_true_preds.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(if_true_preds[i]));
  }
  printf("]\n");

  // if_false block
  auto if_false_succs = get_successors(if_false);
  auto if_false_preds = get_predecessors(if_false);
  printf(";   if_false:\n");
  printf(";     successors: [");
  for (size_t i = 0; i < if_false_succs.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(if_false_succs[i]));
  }
  printf("]\n");
  printf(";     predecessors: [");
  for (size_t i = 0; i < if_false_preds.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(if_false_preds[i]));
  }
  printf("]\n");

  // merge block
  auto merge_succs = get_successors(merge);
  auto merge_preds = get_predecessors(merge);
  printf(";   merge:\n");
  printf(";     successors: [");
  for (size_t i = 0; i < merge_succs.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(merge_succs[i]));
  }
  printf("]\n");
  printf(";     predecessors: [");
  for (size_t i = 0; i < merge_preds.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(merge_preds[i]));
  }
  printf("]\n");

  printf(";\n");
  printf("; Loop pattern:\n");

  // loop_entry block
  auto loop_entry_succs = get_successors(loop_entry);
  auto loop_entry_preds = get_predecessors(loop_entry);
  printf(";   entry:\n");
  printf(";     successors: [");
  for (size_t i = 0; i < loop_entry_succs.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(loop_entry_succs[i]));
  }
  printf("]\n");
  printf(";     predecessors: [");
  for (size_t i = 0; i < loop_entry_preds.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(loop_entry_preds[i]));
  }
  printf("]\n");

  // loop_body block (has self-reference)
  auto loop_body_succs = get_successors(loop_body);
  auto loop_body_preds = get_predecessors(loop_body);
  printf(";   loop:\n");
  printf(";     successors: [");
  for (size_t i = 0; i < loop_body_succs.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(loop_body_succs[i]));
  }
  printf("]\n");
  printf(";     predecessors: [");
  for (size_t i = 0; i < loop_body_preds.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(loop_body_preds[i]));
  }
  printf("]\n");

  // loop_exit block
  auto loop_exit_succs = get_successors(loop_exit);
  auto loop_exit_preds = get_predecessors(loop_exit);
  printf(";   exit:\n");
  printf(";     successors: [");
  for (size_t i = 0; i < loop_exit_succs.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(loop_exit_succs[i]));
  }
  printf("]\n");
  printf(";     predecessors: [");
  for (size_t i = 0; i < loop_exit_preds.size(); i++) {
    if (i > 0)
      printf(", ");
    printf("%s", LLVMGetBasicBlockName(loop_exit_preds[i]));
  }
  printf("]\n");

  printf("\n");

  // Print module IR
  char *ir = LLVMPrintModuleToString(mod);
  printf("%s", ir);
  LLVMDisposeMessage(ir);

  // Cleanup
  LLVMDisposeModule(mod);
  LLVMContextDispose(ctx);

  return 0;
}
