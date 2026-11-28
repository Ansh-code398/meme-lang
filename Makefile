CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -I.
LDFLAGS =

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -framework Cocoa -framework CoreGraphics -framework Metal -framework QuartzCore -framework JavaScriptCore
    MM_SRCS = src/ui/NativeDesktopWindow.mm
    MM_OBJS = $(MM_SRCS:.mm=.o)
    QJS_OBJS =
else
    CC ?= cc
    QJS_SRCS = third_party/quickjs/quickjs.c third_party/quickjs/dtoa.c third_party/quickjs/libregexp.c third_party/quickjs/libunicode.c
    QJS_OBJS = $(QJS_SRCS:.c=.o)
    LDFLAGS += -lm -ldl -lpthread
endif

SRCS = src/main.cpp src/interpreter/Interpreter.cpp src/types/TypeChecker.cpp src/compiler/NativeCompiler.cpp src/lsp/LspServer.cpp src/dap/DapServer.cpp
OBJS = $(SRCS:.cpp=.o) $(MM_OBJS) $(QJS_OBJS)
HEADERS = $(wildcard src/lexer/*.h) $(wildcard src/parser/*.h) $(wildcard src/interpreter/*.h) $(wildcard src/types/*.h) $(wildcard src/bridge/*.h) $(wildcard third_party/quickjs/*.h) $(wildcard src/ui/*.h) $(wildcard src/hosts/*/*.h) $(wildcard src/compiler/*.h) $(wildcard src/platform/*.h) $(wildcard src/packager/*.h) $(wildcard src/server/*.h) $(wildcard src/lsp/*.h) $(wildcard src/dap/*.h)
TARGET = meme

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.mm $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

third_party/quickjs/%.o: third_party/quickjs/%.c $(wildcard third_party/quickjs/*.h)
	$(CC) -std=c11 -O2 -D_GNU_SOURCE -Ithird_party/quickjs -c $< -o $@

test: $(QJS_OBJS)
	$(CXX) $(CXXFLAGS) -DMEME_HAS_EVAL src/tests/test_suite.cpp src/interpreter/Interpreter.cpp src/types/TypeChecker.cpp src/compiler/NativeCompiler.cpp $(MM_SRCS) $(QJS_OBJS) -o test_runner $(LDFLAGS)
	./test_runner
	rm -f test_runner

clean:
	rm -f $(OBJS) $(TARGET) test_runner a.out src/ui/*.o

wasm:
	./scripts/build-wasm.sh

docs: $(TARGET)
	./meme package docs/src/App.meme --target=web -o docs/.build
	rm -rf docs/dist
	mv docs/.build/web docs/dist
	rm -rf docs/.build

docs-dev: $(TARGET)
	./meme dev docs/src/App.meme --port 4173

test-lowlevel: $(TARGET)
	python3 tests/lowlevel/test_e2e_runner.py

test-lowlevel-tier1: $(TARGET)
	python3 tests/lowlevel/test_e2e_runner.py --tier 1

test-lowlevel-parity: $(TARGET)
	python3 tests/lowlevel/test_e2e_runner.py --feature primitives --engine all

test-all: test test-lowlevel

benchmark-lowlevel: $(TARGET)
	python3 benchmarks/lowlevel/run_lowlevel_benchmarks.py

.PHONY: all test clean wasm docs docs-dev test-lowlevel test-lowlevel-tier1 test-lowlevel-parity test-all benchmark-lowlevel
