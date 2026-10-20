CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2

SRCS = src/main.cpp src/interpreter/Interpreter.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = meme

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test:
	$(CXX) $(CXXFLAGS) src/tests/test_suite.cpp src/interpreter/Interpreter.cpp -o test_runner
	./test_runner
	rm -f test_runner

clean:
	rm -f $(OBJS) $(TARGET) test_runner a.out

.PHONY: all test clean
