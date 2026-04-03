#!/bin/bash
set -euo pipefail

echo "======================================"
echo " Building Community WebAssembly Bundle "
echo "======================================"

try_source_emsdk() {
    local emsdk_env
    for emsdk_env in \
        "${EMSDK:-}/emsdk_env.sh" \
        "./.emsdk/emsdk_env.sh" \
        "./emsdk/emsdk_env.sh" \
        "../.emsdk/emsdk_env.sh" \
        "../emsdk/emsdk_env.sh" \
        "$HOME/.emsdk/emsdk_env.sh" \
        "$HOME/emsdk/emsdk_env.sh" \
        "/opt/emsdk/emsdk_env.sh"
    do
        if [ -f "$emsdk_env" ]; then
            # shellcheck disable=SC1090
            source "$emsdk_env" >/dev/null 2>&1 || true
            return 0
        fi
    done
    return 1
}

if ! command -v em++ >/dev/null 2>&1; then
    try_source_emsdk || true
fi

if ! command -v em++ >/dev/null 2>&1; then
    # Try to bootstrap a local emsdk checkout in this repository.
    if [ -x "./.emsdk/emsdk" ] || [ -f "./.emsdk/emsdk.py" ]; then
        echo "em++ missing. Bootstrapping local .emsdk toolchain (one-time setup)..."
        (
            cd ./.emsdk
            ./emsdk install latest
            ./emsdk activate latest
        )
        try_source_emsdk || true
    fi
fi

if ! command -v em++ >/dev/null 2>&1; then
    echo "ERROR: em++ not found. Install and activate Emscripten (emsdk) first."
    echo "       https://emscripten.org/docs/getting_started/downloads.html"
    echo
    echo "Quick start:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    echo
    echo "If this repository already has .emsdk, run:"
    echo "  cd .emsdk && ./emsdk install latest && ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

DIST_DIR="dist_web"
OUT_HTML="$DIST_DIR/index.html"

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

SRC=$(find . -name "*.cpp" \
    -not -path "./.emsdk/*" \
    -not -path "./glfw/*" \
    -not -path "./dist/*" \
    -not -path "./dist_web/*" \
    -not -path "./core/net/http.cpp" | tr '\n' ' ')

em++ $SRC \
    -std=c++17 \
    -O2 \
    -sUSE_GLFW=3 \
    -sLEGACY_GL_EMULATION=1 \
    -sFORCE_FILESYSTEM=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sMIN_WEBGL_VERSION=1 \
    -sMAX_WEBGL_VERSION=2 \
    --preload-file assets@/assets \
    --preload-file game/data@/game/data \
    --shell-file web/shell.html \
    -o "$OUT_HTML"

echo "Build complete: $OUT_HTML"
echo "Serve it locally: python3 -m http.server 8080 --directory $DIST_DIR"
