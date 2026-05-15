CXX := clang++-20
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -g -O0
SRCDIR := src
BUILDDIR := build
TARGET := razenc

# Source files
SRCS := $(SRCDIR)/main.cpp \
        $(SRCDIR)/lexer/lexer.cpp \
        $(SRCDIR)/ast/expression.cpp \
        $(SRCDIR)/ast/type_parser.cpp \
        $(SRCDIR)/parser/parser.cpp \
        $(SRCDIR)/dbg/debug.cpp \
        $(SRCDIR)/semantic/analyzer.cpp

OBJS := $(SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)
