CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRC = $(shell find . -name "*.cpp" \
	-not -path "./.emsdk/*" \
	-not -path "./glfw/*" \
	-not -path "./dist/*" \
	-not -path "./dist_web/*")

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	RUN_TARGET = ./dist/game
else
	RUN_TARGET = ./dist/game.exe
endif

BUILD_TARGET = $(RUN_TARGET)

LDFLAGS = -lglfw -lGL -lGLU -ldl -lpthread

all: $(BUILD_TARGET)

clean:
	bash init.sh

$(BUILD_TARGET): $(SRC)
ifeq ($(UNAME_S),Darwin)
	bash build_macos.sh
else
	bash build.sh
endif

run: $(BUILD_TARGET)
	$(RUN_TARGET)

web:
	bash build_web.sh

web-run:
	python3 -m http.server 8080 --directory dist_web

.PHONY: all clean run web web-run