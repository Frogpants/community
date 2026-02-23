CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = main
SRC = $(shell find . -name "*.cpp")

LDFLAGS = -lglfw -lGL -lGLU -ldl -lpthread

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run:
	./main

.PHONY: all clean