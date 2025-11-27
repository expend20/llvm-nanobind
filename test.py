#!/opt/homebrew/bin/python3.14
try:
  from build.llvm import LLVMContext, LLVMModule, LLVMContextManager, LLVMModuleManager, global_context
except ModuleNotFoundError as e:
  raise ModuleNotFoundError(f"{e}\n  You can compile the native module with the following commands:\n    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON\n    cmake --build build")

global_ctx = global_context()
print("start context")
with LLVMModuleManager("test_module") as module:
  pass
#with LLVMContextManager() as ctx:
#  pass
"""
  print(ctx.discard_value_names)
  print("begin module")
  with LLVMModuleManager("test_module", ctx) as module:
    print(module.data_layout)
  print("end module")
print("end context")
"""