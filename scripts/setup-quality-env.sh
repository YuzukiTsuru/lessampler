#!/usr/bin/env bash
# Create the optional SingMOS quality environment.
#
# The objective quality report does not need this. This script installs a
# Python 3.12 venv with CPU PyTorch, torchaudio, s3prl and librosa, then loads
# SingMOS-Pro once to download its weights. Everything lands under build/.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
venv="${QUALITY_VENV:-$root/build/quality-venv}"
python_version="${QUALITY_PYTHON:-3.12}"
torch_index="${TORCH_INDEX_URL:-https://download.pytorch.org/whl/cpu}"
skip_warmup="${SKIP_WARMUP:-0}"

say() {
    printf '\n==> %s\n' "$1"
}

fail() {
    printf 'error: %s\n' "$1" >&2
    exit 1
}

command -v curl >/dev/null 2>&1 || fail "curl is required"
command -v git >/dev/null 2>&1 || fail "git is required for torch.hub"

uv_bin="${UV_BIN:-}"
if [[ -z "$uv_bin" ]]; then
    if command -v uv >/dev/null 2>&1; then
        uv_bin="$(command -v uv)"
    elif [[ -x "$HOME/.local/bin/uv" ]]; then
        uv_bin="$HOME/.local/bin/uv"
    elif [[ -x "$HOME/.cargo/bin/uv" ]]; then
        uv_bin="$HOME/.cargo/bin/uv"
    else
        say "Installing uv from https://astral.sh/uv"
        curl -LsSf https://astral.sh/uv/install.sh | UV_NO_MODIFY_PATH=1 sh
        for candidate in "$HOME/.local/bin/uv" "$HOME/.cargo/bin/uv"; do
            if [[ -x "$candidate" ]]; then
                uv_bin="$candidate"
                break
            fi
        done
    fi
fi
[[ -n "$uv_bin" && -x "$uv_bin" ]] || fail "uv was not found after installation"
"$uv_bin" --version

say "Installing Python $python_version"
"$uv_bin" python install "$python_version"

say "Creating venv: $venv"
"$uv_bin" venv --python "$python_version" --clear "$venv"
venv_python="$venv/bin/python"
[[ -x "$venv_python" ]] || fail "venv python was not created"

say "Installing CPU PyTorch"
if [[ "$(uname -s)" == "Darwin" ]]; then
    "$uv_bin" pip install --python "$venv_python" torch torchaudio
else
    "$uv_bin" pip install --python "$venv_python" --index-url "$torch_index" torch torchaudio
fi

say "Installing s3prl / librosa / soundfile"
"$uv_bin" pip install --python "$venv_python" s3prl librosa soundfile

if [[ "$skip_warmup" != "1" ]]; then
    say "Loading SingMOS-Pro (downloads the wav2vec2 weights on first run)"
    "$venv_python" - <<'PY'
import torch

print("torch", torch.__version__)
predictor = torch.hub.load(
    "South-Twilight/SingMOS:v1.1.2",
    "singmos_pro",
    trust_repo=True,
)
print("SingMOS-Pro loaded")
PY
else
    say "Skipping SingMOS warmup (SKIP_WARMUP=1)"
fi

say "Environment ready: $venv"
printf 'Run the quality report with:\n'
printf '  %s scripts/quality_report.py --ust <song.ust> --singmos <renders.wav...>\n' "$venv_python"
