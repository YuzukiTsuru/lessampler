#!/usr/bin/env bash
# Extract a voicebank/UST pair from data/ and render the UST with lessampler.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="${1:-$root/build/data-render}"
output="${2:-$root/build/data-render/main.wav}"
lessampler="${LESSAMPLER:-$root/build/max-gcc/bin/lessampler}"
wavtool="${LESSWAVTOOL:-$(dirname "$lessampler")/lesswavtool}"

if [[ ! -x "$lessampler" ]]; then
    echo "lessampler not found: $lessampler" >&2
    echo "Build it first: cmake --preset max-gcc && cmake --build --preset max-gcc -j" >&2
    exit 1
fi
if [[ ! -x "$wavtool" ]]; then
    echo "lesswavtool not found: $wavtool" >&2
    echo "Build it first: cmake --preset max-gcc && cmake --build --preset max-gcc -j" >&2
    exit 1
fi

ust_archive="$(find "$root/data" -maxdepth 1 -iname '*.zip' | head -1 || true)"
voice_archive="$(find "$root/data" -maxdepth 1 -iname '*.tar.zst' | head -1 || true)"
if [[ -z "$ust_archive" || -z "$voice_archive" ]]; then
    echo "data/ must contain a UST .zip and a voicebank .tar.zst" >&2
    exit 1
fi

rm -rf "$work"
mkdir -p "$work/ust" "$work/voicebank"
unzip -q "$ust_archive" -d "$work/ust"
tar --zstd -xf "$voice_archive" -C "$work/voicebank"

ust="$(find "$work/ust" -iname '*.ust' | sort | tail -1)"
oto="$(find "$work/voicebank" -maxdepth 2 -name oto.ini | head -1 || true)"
if [[ -z "$oto" ]]; then
    oto="$(find "$work/voicebank" -name oto.ini | head -1)"
fi
voicebank="$(dirname "$oto")"

extra=()
if [[ "${LESSAMPLER_FEATURE:-0}" == "1" ]]; then
    extra+=(--feature)
fi
if [[ "${LESSAMPLER_POLISH:-0}" == "1" ]]; then
    extra+=(--polish)
fi

python3 "$root/scripts/render_ust.py" \
    --ust "$ust" \
    --voicebank "$voicebank" \
    --output "$output" \
    --lessampler "$lessampler" \
    --wavtool "$wavtool" \
    "${extra[@]}"
