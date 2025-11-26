#!/opt/homebrew/bin/python3.14
try:
  from build.llvm import LLVMContext
except ModuleNotFoundError as e:
  raise ModuleNotFoundError(f"{e}\n  You can compile the native module with the following commands:\n    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON\n    cmake --build build")


global_ctx = LLVMContext(global_context=True)

ctx = LLVMContext()
print(ctx.discard_value_names)
