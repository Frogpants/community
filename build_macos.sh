#!/bin/bash
set -euo pipefail

echo " Building fungi-demo macOS Executable "

DIST_DIR="dist"
EXE_NAME="game"

# Clean old build
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

echo "Collecting source files..."
SRC=$(find . -name "*.cpp" -not -path "./glfw/*" -not -path "./dist/*" | tr '\n' ' ')

echo "Detecting local GLFW build..."
if [ -d "glfw" ] && [ -f "glfw/build/src/libglfw.dylib" ]; then
    echo "Using local GLFW build"
    FLAGS="-Iglfw/include -Lglfw/build/src -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo"
elif command -v pkg-config >/dev/null 2>&1 && pkg-config --exists glfw3; then
    PKG_CFLAGS=$(pkg-config --cflags glfw3)
    PKG_LIBS=$(pkg-config --libs glfw3)
    FLAGS="$PKG_CFLAGS $PKG_LIBS"
else
    FLAGS="-lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo"
fi

echo "Compiling..."
# Ensure runtime linker can find local dylibs next to the executable
RPATH_FLAG="-Wl,-rpath,@executable_path"
clang++ -std=c++17 -O2 $SRC $FLAGS $RPATH_FLAG -o "$DIST_DIR/$EXE_NAME"

echo "Copying assets..."
cp -r assets "$DIST_DIR/" || true

# If we built a local GLFW dynamic library, copy it next to the executable
if [ -d "glfw/build/src" ]; then
    echo "Copying local GLFW dylibs to $DIST_DIR"
    cp -v glfw/build/src/libglfw*.dylib "$DIST_DIR/" || true
fi

echo "Build complete"
echo "Executable located at: $DIST_DIR/$EXE_NAME"
