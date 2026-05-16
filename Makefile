CXX := clang++-20
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -g -O0 \
            $(shell llvm-config-20 --cxxflags | sed 's/-fno-exceptions//g; s/-funwind-tables//g')
LDFLAGS := $(shell llvm-config-20 --ldflags --system-libs --libs core irreader bitwriter analysis transformutils passes support)
SRCDIR := src
BUILDDIR := build
TARGET := razenc

SRCS := $(SRCDIR)/main.cpp \
        $(SRCDIR)/lexer/lexer.cpp \
        $(SRCDIR)/ast/expression.cpp \
        $(SRCDIR)/ast/type_parser.cpp \
        $(SRCDIR)/parser/parser.cpp \
        $(SRCDIR)/dbg/debug.cpp \
        $(SRCDIR)/semantic/analyzer.cpp \
        $(SRCDIR)/codegen/ir.cpp \
        $(SRCDIR)/codegen/codegen.cpp

OBJS := $(SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)
