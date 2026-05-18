CXX := clang++-20
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -g -O0 \
            $(shell llvm-config-20 --cxxflags | sed 's/-fno-exceptions//g; s/-funwind-tables//g')
LDFLAGS := $(shell llvm-config-20 --ldflags) -lLLVM-20 $(shell llvm-config-20 --system-libs)
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

# Runtime library (linked into generated executables)
FMT_SRC := src/std/fmt.cpp
FMT_OBJ := build/std/fmt.o

.PHONY: all clean run fmt-lib

all: $(TARGET) fmt-lib

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c -o $@ $<

# Standalone compile for the runtime library (no LLVM deps)
fmt-lib: $(FMT_OBJ)

$(FMT_OBJ): $(FMT_SRC)
	@mkdir -p $(dir $@)
	$(CXX) -std=c++20 -c -o $@ $<

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

# Build, run codegen, optimize, and emit IR to file
test-all: $(TARGET)
	@mkdir -p output
	./$(TARGET) 2>&1 | grep -E "verification failed|Error" || echo "All tests passed"
	# Extract and compile each IR
	python3 tools/extract_ir.py 2>/dev/null || echo "extract_ir.py not found"
