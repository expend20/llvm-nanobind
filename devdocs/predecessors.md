In the LLVM-C API, getting the predecessors of a basic block requires iterating through the block's **uses** and finding the parent blocks of terminator instructions that reference it. Here's how:

```c
#include <llvm-c/Core.h>

void print_predecessors(LLVMBasicBlockRef block) {
    // Convert the basic block to a value to access its uses
    LLVMValueRef block_value = LLVMBasicBlockAsValue(block);
    
    // Iterate through all uses of this block
    for (LLVMUseRef use = LLVMGetFirstUse(block_value); 
         use != NULL; 
         use = LLVMGetNextUse(use)) {
        
        // Get the instruction that uses this block
        LLVMValueRef user = LLVMGetUser(use);
        
        // Check if it's a terminator instruction (branch, switch, etc.)
        if (LLVMIsATerminatorInst(user)) {
            // Get the parent block of the terminator instruction
            LLVMBasicBlockRef predecessor = LLVMGetInstructionParent(user);
            
            // Now you have a predecessor block
            printf("Predecessor: %s\n", LLVMGetBasicBlockName(predecessor));
        }
    }
}
```

**Key functions:**
- `LLVMBasicBlockAsValue()` - converts a basic block to a value so you can access its uses
- `LLVMGetFirstUse()` - gets the first use of a value
- `LLVMGetNextUse()` - iterates to the next use
- `LLVMGetUser()` - gets the instruction that contains the use
- `LLVMIsATerminatorInst()` - checks if an instruction is a terminator
- `LLVMGetInstructionParent()` - gets the parent block of an instruction

This works because when a block branches to another block, that creates a "use" relationship - the terminator instruction uses the target block as an operand.