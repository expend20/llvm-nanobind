# llvm-nanobind

LLVM-C Python bindings with nanobind. 

⚠️ This project is still in a very early design phase and not remotely usable.

## Local development

Set the CMake prefix path environment variable to point to the LLVM prefix:

```bash
export CMAKE_PREFIX_PATH=$(brew --prefix llvm)
```

Configure the bindings:

```bash
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Build the bindings:

```bash
cmake --build build
```

Run the example:

```bash
uv run playground.py
```
