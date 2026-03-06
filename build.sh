#!/bin/bash

echo "======================================"
echo " Building fungi-demo Windows Executable "
echo "======================================"

DIST_DIR="dist"
EXE_NAME="game.exe"
ASSET_DIR="assets"

# Stop immediately if any command fails
set -e

# Clean old build
echo "Cleaning old build..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

echo "Compiling..."

# Collect all source files
SRC="main.cpp"

for dir in core/*; do
    if [ -d "$dir" ]; then
        for file in "$dir"/*.cpp; do
            [ -f "$file" ] && SRC="$SRC $file"
        done
    fi
done

# Compile with MinGW-w64
x86_64-w64-mingw32-g++ $SRC \
    -Iglfw/include \
    -Lglfw/build/src \
    -lglfw3 \
    -lopengl32 \
    -lgdi32 \
    -luser32 \
    -lkernel32 \
    -std=c++17 \
    -O2 \
    -static \
    -o "$DIST_DIR/$EXE_NAME"

echo "Compilation successful."

# Copy assets safely
if [ -d "$ASSET_DIR" ]; then
    echo "Copying assets..."
    mkdir -p "$DIST_DIR/$ASSET_DIR"
    cp -r "$ASSET_DIR/"* "$DIST_DIR/$ASSET_DIR/"
else
    echo "WARNING: No assets folder found!"
fi

echo "======================================"
echo "Build complete"
echo "Run make run to run executable"
echo "======================================"