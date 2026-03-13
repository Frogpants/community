#!/bin/bash
set -euo pipefail

rm -rf dist
rm -rf glfw

git clone https://github.com/glfw/glfw.git

cd glfw
rm -rf build

UNAME_S="$(uname -s)"

if [ "$UNAME_S" = "Darwin" ]; then
  cmake -S . -B build \
    -G "Unix Makefiles" \
    -DBUILD_SHARED_LIBS=OFF
else
  if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1 && command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    cmake -S . -B build \
      -G "Unix Makefiles" \
      -DCMAKE_SYSTEM_NAME=Windows \
      -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
      -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
      -DBUILD_SHARED_LIBS=OFF
  else
    cmake -S . -B build \
      -G "Unix Makefiles" \
      -DBUILD_SHARED_LIBS=OFF
  fi
fi

cmake --build build
cd ..