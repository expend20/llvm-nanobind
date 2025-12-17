#!/usr/bin/env -S uv run
from llvm import global_context, create_context

global_ctx = global_context()
with global_ctx.create_module("global_module") as module:
    print(f"{module.target_triple=}")
    print(f"{module.name=}")
    print(f"{module.data_layout=}")

with create_context() as context:
    with context.create_module("local_module") as module:
        print(f"{module.target_triple=}")
        print(f"{module.name=}")
        print(f"{module.data_layout=}")
