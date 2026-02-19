; RUN: set +o pipefail; llvm-as < %s | llvm-c-test --echo 2>&1 | FileCheck %s
;
; Known upstream LLVM-C limitation:
; LLVMGetValueKind reports poison constants as UndefValue, causing echo to fail
; when it validates ValueKind before cloning.

define i32 @test_poison() {
entry:
  ret i32 poison
}

; CHECK: LLVMGetValueKind returned incorrect type
