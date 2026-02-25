CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = dist
SRC = $(shell find . -name "*.cpp")

UNAME_S := $(shell uname -s)

LDFLAGS = -lglfw -lGL -lGLU -ldl -lpthread

all: $(TARGET)



$(TARGET): $(SRC)
ifeq ($(UNAME_S),Darwin)
	bash build_macos.sh
else
	bash build.sh
endif

clean:
	bash init.sh

run:
ifeq ($(UNAME_S),Darwin)
	./dist/game
else
	./dist/game.exe
endif

.PHONY: all clean