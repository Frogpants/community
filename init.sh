rm -rf dist

rm -rf glfw

git clone https://github.com/glfw/glfw.git

cd glfw
rm -rf build
cmake -S . -B build \
  -G "Unix Makefiles" \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DBUILD_SHARED_LIBS=OFF

cmake --build build
cd ..