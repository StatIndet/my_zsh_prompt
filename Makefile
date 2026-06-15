CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -pedantic
PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin
TARGET ?= prompt

SOURCES := main.cpp modules.cpp
BUILD_TARGET := prompt_dev
TEST_TARGET := prompt_tests

.PHONY: all install test clean

all: $(BUILD_TARGET)

$(BUILD_TARGET): $(SOURCES) utils.h
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@

$(TEST_TARGET): tests.cpp modules.cpp utils.h
	$(CXX) $(CXXFLAGS) tests.cpp modules.cpp -o $@

install: $(BUILD_TARGET)
	install -d $(BINDIR)
	install -m 0755 $(BUILD_TARGET) $(BINDIR)/$(TARGET)

test: $(BUILD_TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)
	./$(BUILD_TARGET) 0 120ms 40 >/tmp/prompt_dev_narrow.out

clean:
	rm -f $(BUILD_TARGET) $(TEST_TARGET)
