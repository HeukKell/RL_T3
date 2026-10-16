import os
import sys
import time

torch_lib = os.path.join(
    sys.prefix,
    "Lib",
    "site-packages",
    "torch",
    "lib"
)

print("DLL directory:", torch_lib, flush=True)

for filename in sorted(os.listdir(torch_lib)):
    if filename.lower().endswith(".dll"):
        print(filename)

print("Starting torch import...", flush=True)

start = time.perf_counter()

import torch

print(
    f"Import completed: {time.perf_counter() - start:.2f}s",
    flush=True
)