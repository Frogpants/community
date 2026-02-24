#!/bin/bash

echo " Building fungi-demo Windows Executable "

DIST_DIR="dist"
EXE_NAME="game.exe"

# Clean old build
rm -rf $DIST_DIR
mkdir -p $DIST_DIR

echo "Compiling..."

# Collect all source files
SRC="main.cpp"
for dir in core/*; do
    if [ -d "$dir" ]; then
        for file in "$dir"/*.cpp; do
            SRC="$SRC $file"
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
    -o $DIST_DIR/$EXE_NAME

if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

echo "Copying assets..."
cp -r assets $DIST_DIR/

echo "Build complete"
echo "Executable located at: $DIST_DIR/$EXE_NAME"
