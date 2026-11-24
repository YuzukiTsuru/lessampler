#!/usr/bin/env bash
# Build, test, and package a release. The default preset enables every optional
# backend; ONNX Runtime is downloaded into lib/onnxruntime on first configure.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-max-gcc}"
shift || true

cd "$root"

if (($# > 0)); then
    cmake --preset "$preset" "$@"
else
    cmake --preset "$preset"
fi

jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
cmake --build --preset "$preset" -j "$jobs"
ctest --preset "$preset" --output-on-failure
cmake --build --preset "$preset" --target package

echo
echo "Release artifacts:"
find build -maxdepth 2 -type f \( -name 'lessampler-*.tar.gz' -o -name 'lessampler-*.tar.gz.sha256' \
    -o -name 'lessampler-*.zip' -o -name 'lessampler-*.zip.sha256' \) -print
