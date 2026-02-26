#!/bin/bash
set -euo pipefail

echo " Building fungi-demo macOS Executable "

DIST_DIR="dist"
EXE_NAME="game"

# Clean old build
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

echo "Collecting source files..."
SRC=$(find . -name "*.cpp" \
    -not -path "./glfw/*" \
    -not -path "./dist/*" | tr '\n' ' ')

echo "Detecting local GLFW build..."

FLAGS=""
USING_LOCAL_GLFW=false

# Check for locally built GLFW dylib (any version)
if [ -d "glfw/build/src" ] && ls glfw/build/src/libglfw*.dylib >/dev/null 2>&1; then
    echo "Using local GLFW build"
    FLAGS="-Iglfw/include -Lglfw/build/src -lglfw \
           -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo"
    USING_LOCAL_GLFW=true

elif command -v pkg-config >/dev/null 2>&1 && pkg-config --exists glfw3; then
    echo "Using pkg-config GLFW"
    PKG_CFLAGS=$(pkg-config --cflags glfw3)
    PKG_LIBS=$(pkg-config --libs glfw3)
    FLAGS="$PKG_CFLAGS $PKG_LIBS"

else
    echo "Using system GLFW fallback"
    FLAGS="-lglfw \
           -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo"
fi

echo "Compiling..."

# Ensure runtime linker searches next to executable
RPATH_FLAG="-Wl,-rpath,@executable_path"

clang++ -std=c++17 -O2 $SRC $FLAGS $RPATH_FLAG \
    -o "$DIST_DIR/$EXE_NAME"

echo "Copying assets..."
cp -r assets "$DIST_DIR/" 2>/dev/null || true

# Copy local GLFW dylibs if using local build
if [ "$USING_LOCAL_GLFW" = true ]; then
    echo "Copying local GLFW dylibs to $DIST_DIR"
    cp -v glfw/build/src/libglfw*.dylib "$DIST_DIR/" || true

    cd "$DIST_DIR"

    # Automatically create expected compatibility symlink
    REAL_DYLIB=$(ls libglfw.*.dylib | head -n 1 || true)

    if [ -n "$REAL_DYLIB" ]; then
        echo "Creating compatibility symlink: libglfw.3.dylib -> $REAL_DYLIB"
        ln -sf "$REAL_DYLIB" libglfw.3.dylib
    fi

    cd ..
fi

echo "Build complete"
echo "Executable located at: $DIST_DIR/$EXE_NAME"
echo "Run with: ./$DIST_DIR/$EXE_NAME"