#!/usr/bin/env bash
# Download the latest lessampler release archive for this machine.
# Requires the GitHub CLI (gh) and an authenticated or anonymous session.
set -euo pipefail

repo="YuzukiTsuru/lessampler"
destination="${1:-release}"

if ! command -v gh >/dev/null 2>&1; then
    echo "gh (GitHub CLI) is required: https://cli.github.com/" >&2
    exit 1
fi

case "$(uname -s)" in
    Linux*) os_name="Linux" ;;
    Darwin*) os_name="Darwin" ;;
    MINGW* | MSYS* | CYGWIN*) os_name="Windows" ;;
    *) os_name="" ;;
esac

case "$(uname -m)" in
    x86_64 | amd64) arch="x86_64" ;;
    arm64 | aarch64) arch="arm64" ;;
    *) arch="" ;;
esac

if [[ "$os_name" == "Windows" ]]; then
    case "$arch" in
        x86_64) arch="AMD64" ;;
        arm64) arch="ARM64" ;;
    esac
fi

mkdir -p "$destination"

if [[ -n "$os_name" && -n "$arch" ]]; then
    if gh release download --repo "$repo" --dir "$destination" --clobber \
        --pattern "lessampler-*-${os_name}-${arch}.*"; then
        echo "Downloaded to $destination"
        exit 0
    fi
    echo "No ${os_name}-${arch} asset found; downloading every release asset." >&2
fi

gh release download --repo "$repo" --dir "$destination" --clobber
echo "Downloaded to $destination"
