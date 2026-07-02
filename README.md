# TinyC - A Simple Compiler Built with Flex, Bison & LLVM

A minimal C-like compiler that demonstrates the classic compiler pipeline:

```
source.c → Flex (lexer) → Bison (parser) → AST → CodeGen → LLVM IR → Machine Code
```

## Features

- `int` variable declarations and assignments
- Arithmetic: `+`, `-`, `*`, `/`
- Comparisons: `<`, `>`, `<=`, `>=`, `==`, `!=`
- Control flow: `if`/`else`, `while`
- `print(expr)` built-in (maps to `printf`)
- `return` statement

## Quick Start

```bash
# Generate parser & scanner
bison -d parser.y -o parser.tab.cpp --defines=parser.tab.hpp
flex -o scanner.yy.cpp scanner.l

# Compile
c++ -std=c++17 $(llvm-config --cxxflags) -I. -c *.cpp parser.tab.cpp scanner.yy.cpp
c++ *.o $(llvm-config --ldflags --system-libs --libs core) -o tinyc

# Run on a test file
./tinyc test.c

# Compile LLVM IR to native executable
./tinyc test.c | llc -relocation-model=pic -o test.s
c++ test.s -o prog && ./prog
```

## Project Structure

| File | Purpose |
|------|---------|
| `scanner.l` | Flex lexer: tokenizes source into tokens |
| `parser.y` | Bison parser: grammar rules + AST construction |
| `ast.h` / `ast.cpp` | AST node definitions and globals |
| `codegen.h` / `codegen.cpp` | LLVM IR generation via IRBuilder |
| `main.cpp` | Driver: read file → parse → codegen → output IR |

## Example

```c
int main() {
    int a;
    int b;
    a = 10;
    b = 20;
    print(a + b * 2);
    return 0;
}
```

Produces LLVM IR with correct arithmetic, then compiles to runnable machine code.
