CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -I.
LDFLAGS =

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -framework Cocoa -framework CoreGraphics
    MM_SRCS = src/ui/NativeDesktopWindow.mm
    MM_OBJS = $(MM_SRCS:.mm=.o)
endif

SRCS = src/main.cpp src/interpreter/Interpreter.cpp src/types/TypeChecker.cpp
OBJS = $(SRCS:.cpp=.o) $(MM_OBJS)
HEADERS = $(wildcard src/lexer/*.h) $(wildcard src/parser/*.h) $(wildcard src/interpreter/*.h) $(wildcard src/types/*.h) $(wildcard src/bridge/*.h) $(wildcard src/ui/*.h)
TARGET = meme

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.mm $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test:
	$(CXX) $(CXXFLAGS) src/tests/test_suite.cpp src/interpreter/Interpreter.cpp src/types/TypeChecker.cpp $(MM_SRCS) -o test_runner $(LDFLAGS)
	./test_runner
	rm -f test_runner

clean:
	rm -f $(OBJS) $(TARGET) test_runner a.out src/ui/*.o

wasm:
	./scripts/build-wasm.sh

docs: wasm
	cd docs && npm run build

docs-dev: wasm
	cd docs && npm run dev

.PHONY: all test clean wasm docs docs-dev
