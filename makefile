CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = dist
SRC = $(shell find . -name "*.cpp")

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	RUN_TARGET = ./dist/game
else
	RUN_TARGET = ./dist/game.exe
endif

LDFLAGS = -lglfw -lGL -lGLU -ldl -lpthread

all: $(TARGET)

$(TARGET): $(SRC)
ifeq ($(UNAME_S),Darwin)
	bash build_macos.sh
else
	bash build.sh
endif

run:
	$(RUN_TARGET)

.PHONY: all clean