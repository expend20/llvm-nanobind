# llvm-nanobind

Python bindings for the LLVM-C API using [nanobind](https://github.com/wjakob/nanobind).

This project provides a Pythonic interface to LLVM's compiler infrastructure, enabling you to build compilers, analyzers, and code transformation tools in Python.

**Status**: Under active development. Core APIs are bound and tested against LLVM's `llvm-c-test` suite, but the API is not yet stable. Expect breaking changes.

_Note_: This project is 90%+ vibe coded. It is mostly an experiment to see what LLMs can do when you set things up properly.

## Features

- Comprehensive LLVM-C API coverage (~7300 lines of bindings)
- Memory-safe: validity tokens prevent use-after-free crashes
- Type-safe: auto-generated `.pyi` stubs for IDE support
- Tested: 25+ lit tests, 15 golden master test pairs

## Installation

This package requires LLVM to be installed. The build will automatically find LLVM if it's in your PATH, or you can specify the path:

```bash
export CMAKE_PREFIX_PATH=/path/to/llvm
pip install .
```

## Quick Start

```python
import llvm

# Create a simple function that returns 42
with llvm.create_context() as ctx:
    with ctx.create_module("example") as mod:
        # Create function type: i32 ()
        i32 = ctx.int32_type()
        fn_type = ctx.function_type(i32, [])
        
        # Create function and basic block
        fn = mod.add_function("get_answer", fn_type)
        bb = fn.append_basic_block("entry")
        
        # Build return instruction
        with ctx.create_builder() as builder:
            builder.position_at_end(bb)
            builder.ret(llvm.const_int(i32, 42))
        
        # Print the IR
        print(mod)
```

## Development

### Setup

```bash
# Configure (first time)
cmake -B build -G Ninja

# Build
cmake --build build

# Or use uv (recommended) - auto-rebuilds as needed
uv sync
```

### Testing

```bash
# Golden master tests (C++ and Python must produce identical output)
uv run run_tests.py

# LLVM lit integration tests
uv run run_llvm_c_tests.py        # Run all tests
uv run run_llvm_c_tests.py -v     # Verbose output

# Type checking
uvx ty check
```

### Coverage

```bash
# Run with coverage
uv run coverage run run_llvm_c_tests.py --use-python
uv run coverage combine
uv run coverage report --include="llvm_c_test/*"
```

## Documentation

Type stubs are auto-generated and provide IDE intellisense. After building, find them at:
```
.venv/lib/python3.*/site-packages/llvm/__init__.pyi
```

For development documentation, see `devdocs/README.md`.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

LLVM is licensed under the Apache License v2.0 with LLVM Exceptions.

## Windows

Download LLVM+Clang:

- https://github.com/vovkos/llvm-package-windows/releases/download/llvm-21.1.1/llvm-21.1.1-windows-amd64-msvc17-msvcrt.7z
- https://github.com/vovkos/llvm-package-windows/releases/download/clang-20.1.8/clang-20.1.8-windows-amd64-msvc17-msvcrt.7z

Merge them together in `C:\llvm-21.1.1`.

Create `CMakeUserPresets.json`:

```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "clang-cl",
            "displayName": "Ninja with clang-cl",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": {
                "CMAKE_C_COMPILER": "C:/Program Files/LLVM/bin/clang-cl.exe",
                "CMAKE_CXX_COMPILER": "C:/Program Files/LLVM/bin/clang-cl.exe",
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
                "CMAKE_BUILD_TYPE": "RelWithDebInfo",
                "CMAKE_PREFIX_PATH": "d:/llvm-21.1.1"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "clang-cl",
            "configurePreset": "clang-cl"
        }
    ]
}
```

Create a virtual environment:

```bash
uv venv
```

Activate the virtual environment:

```bash
.venv/Scripts/activate
```

Configure the CMake project (this should find the Python from your venv):

```bash
cmake --preset clang-cl
```

_Note_: This saves the LLVM prefix to a file called `.llvm-prefix`, make sure to delete that if you change the LLVM prefix path.

Build the bindings:

```bash
cmake --build build
```

After that works you can build the Python package with `uv`:

```bash
uv sync --verbose
```
