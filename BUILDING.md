# Building the C++ detector

The pipelines run the **Python** detector by default and it needs no build. This
is only for the **C++** option in the chooser's Python / C++ toggle, which stays
unavailable until a binary exists.

Run this once, in the SDK container, from the repository root:

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-modalix.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

It cross-compiles for the board's aarch64 target and produces
`build/adaptive-resolution-object-detector`, which the pipelines pick up
automatically — no configuration, and no restart beyond flipping the toggle.

It requires the Neat SDK's Modalix sysroot, which the SDK container provides;
the toolchain file reads `SYSROOT`, `CC` and `CXX` from the environment.

Both implementations expose identical behaviour and take the same options, so
the toggle changes only which one runs. Python is the better default; C++ is
worth it when you are pushing stream counts.
