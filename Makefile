CXX ?= c++
CXXFLAGS ?= -O2 -Wall -Wextra
STD := -std=c++17
PREFIX ?= /usr/local

SRC := $(wildcard src/*.cpp)
LIB_SRC := $(filter-out src/main.cpp,$(SRC))
OBJ := $(patsubst src/%.cpp,build/%.o,$(SRC))
LIB_OBJ := $(patsubst src/%.cpp,build/%.o,$(LIB_SRC))
TEST_SRC := $(wildcard tests/*.cpp)
TEST_OBJ := $(patsubst tests/%.cpp,build/tests/%.o,$(TEST_SRC))
DEPS := $(OBJ:.o=.d) $(TEST_OBJ:.o=.d)

all: bin/fld

bin/fld: $(OBJ)
	@mkdir -p bin
	$(CXX) $(STD) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(STD) $(CXXFLAGS) -MMD -MP -c -o $@ $<

build/tests/%.o: tests/%.cpp
	@mkdir -p build/tests
	$(CXX) $(STD) $(CXXFLAGS) -MMD -MP -Isrc -c -o $@ $<

build/fld_tests: $(TEST_OBJ) $(LIB_OBJ)
	$(CXX) $(STD) $(CXXFLAGS) -o $@ $^

test: build/fld_tests
	./build/fld_tests

install: bin/fld
	install -d $(PREFIX)/bin
	install bin/fld $(PREFIX)/bin/fld

clean:
	rm -rf build bin

-include $(DEPS)

.PHONY: all test install clean
