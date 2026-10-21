# Builds and runs the engine tests with the host compiler (macOS, Linux or
# Haiku). Usage: make -C tests

CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O1 -g
ENGINE := ../src/engine

test: test_canvas
	./test_canvas

test_canvas: test_canvas.cpp $(wildcard $(ENGINE)/*.cpp) $(wildcard $(ENGINE)/*.h)
	$(CXX) $(CXXFLAGS) -I$(ENGINE) test_canvas.cpp $(wildcard $(ENGINE)/*.cpp) -o $@

clean:
	rm -f test_canvas
	rm -rf test_canvas.dSYM

.PHONY: test clean
