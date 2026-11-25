---
title: "Build"
description: "Requirements, presets, optional backends, packaging and tests."
---

## Requirements

- CMake 3.30 or newer. CMake 3.31.x is recommended; CMake 4.x works through the compatibility shim in the root `CMakeLists.txt`.
- A C++26 compiler: GCC 15+, Clang 19+, or MSVC driven with `/std:c++latest`. AppleClang and MSVC are not mapped to C++26 by CMake itself, so the Windows preset uses MSVC's latest mode and the macOS preset uses Homebrew GCC 15.
- Git. Every dependency under `lib/` is a submodule pinned to an exact commit.

```bash
git clone --recursive https://github.com/YuzukiTsuru/lessampler
cd lessampler

cmake --preset max-gcc
cmake --build --preset max-gcc -j
ctest --preset max-gcc --output-on-failure
```

Do not clone with `--depth 1`; the pinned submodule commits are not necessarily at the branch tips. The executable lands in `build/<preset>/bin/lessampler`; it contains the resampler, the wavtool and the CLI. The config file is created next to `lessampler`.

## Presets

| Preset | Contents |
| --- | --- |
| `release-gcc` / `debug-gcc` | WORLD only |
| `asan-gcc` | AddressSanitizer + UndefinedBehaviorSanitizer |
| `neural-gcc` | WORLD + NHVSing neural backend |
| `max-gcc` | WORLD + LLSM + neural |
| `ci-gcc`, `ci-clang`, `ci-msvc` | CI-style WORLD builds |
| `ci-max`, `ci-msvc-max` | CI maximal builds used for packaging |

`LESSAMPLER_ENABLE_LLSM=ON` links libllsm2 and ciglet (GPLv3). The resulting binary is GPLv3; the default WORLD-only build remains LGPL-3.0.

`LESSAMPLER_ENABLE_NEURAL=ON` builds the NHVSing-compatible backend. `cmake/OnnxRuntime.cmake` downloads a pinned ONNX Runtime NuGet package into `lib/onnxruntime` and configures headers, the library, and the Windows DLL copy. `ONNXRUNTIME_ROOT` overrides it. No model weights are distributed.

Other options: `LESSAMPLER_BUILD_TESTS`, `LESSAMPLER_BUILD_TOOLS`, `LESSAMPLER_ENABLE_SANITIZERS`, `LESSAMPLER_DUMP_DATA`, `LESSAMPLER_TIME_EVAL`, `LESSAMPLER_WERROR`, `LESSAMPLER_ENABLE_SINGMOS_TESTS`. `LESSAMPLER_SINGMOS_PYTHON` selects the interpreter used by the optional SingMOS test.

## Release packages

CI builds the maximal configuration on Ubuntu, macOS, and Windows. Tagged pushes (`v*`) run `.github/workflows/release.yml`, which uploads `lessampler-<version>-<os>-<arch>` archives, SHA-256 checksums, and GitHub Release assets. The archives contain both executables, ONNX Runtime when enabled, the Linux OpenMP runtime, README/LICENSE, and third-party licenses. Model weights are not bundled.

```bash
scripts/package-release.sh       # GCC preset max-gcc
scripts/package-release.ps1      # MSVC preset ci-msvc-max
scripts/download-release.sh      # latest release for this machine
```

## Tests and formatting

```bash
ctest --preset max-gcc --output-on-failure

cmake --preset asan-gcc
cmake --build --preset asan-gcc -j
ctest --preset asan-gcc --output-on-failure

find src test -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | \
  xargs -0 clang-format --dry-run --Werror
```

The test data path is passed as `LESSAMPLER_TEST_DATA_DIR`; tests read fixtures and write only to temporary directories. `LESSAMPLER_TIME_EVAL=ON` prints stage timings.

`QualityEvaluationTest` (label `evaluation`) runs the standardized objective evaluation when Python 3 is available. The real SingMOS test is opt-in:

```bash
cmake --preset max-gcc \
  -DLESSAMPLER_ENABLE_SINGMOS_TESTS=ON \
  -DLESSAMPLER_SINGMOS_PYTHON="$PWD/build/quality-venv/bin/python"
cmake --build --preset max-gcc -j
ctest --preset max-gcc -L evaluation --output-on-failure
```

`test/quality/setup_singmos.sh` creates that virtual environment. See [Quality evaluation](../guides/quality.md) for the protocol and JSON schema.
