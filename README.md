# llvm-nanobind

LLVM-C Python bindings with nanobind. 

⚠️ This project is still in a very early design phase and not remotely usable.

Design discussion with Claude: https://claude.ai/share/eb38206e-c546-45a2-971a-9ae4bea00848

## Building

Create a virtual environment (we will switch to `uv` later):

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Get the path to the LLVM prefix:

```bash
export LLVM_PREFIX=$(brew --prefix llvm)
```

Configure the bindings:

```bash
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=$LLVM_PREFIX
```

Build the bindings:

```bash
cmake --build build
```

Run the example:

```bash
python3 test.py
```
