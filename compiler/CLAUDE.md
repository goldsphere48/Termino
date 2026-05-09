# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project context

This directory is the **compiler** for **Termino**, a small stack-based VM with a custom `.tasm` assembly language. The parent repo (`D:\Code\C++\Termino`) declares three subprojects in `CMakeLists.txt` — `compiler`, `vm`, `emulator` — but only `compiler` has source code today; `vm/` and `emulator/` are empty placeholders. The top-level `CMakeLists.txt` will not currently configure successfully.

The compiler currently implements **lexer + parser** (AST printing only). There is no semantic analysis, no codegen, no linker — `main.cpp` just dumps tokens then prints the AST.

## Build & run

The compiler is **not** built via CMake. Use the batch script:

```
build.bat
```

This invokes `clang main.cpp lexer.cpp error.cpp isa_helper.cpp parser.cpp -o build\compiler.exe -O0 -g -std=c++20`. C++20 is required (uses `std::variant`, `std::string_view`, structured bindings on maps, `if constexpr`).

Run the compiler against a `.tasm` source file:

```
build\compiler.exe test.tasm
build\compiler.exe tests\main.tasm
```

There is no test runner — `tests/` contains hand-written `.tasm` files (`valid_identifiers.tasm`, `invalid_identifiers.tasm`, `main.tasm`) used as ad-hoc fixtures. To "test" a change, run the compiler against these files and inspect stdout/stderr.

## Architecture

Pipeline is a classic recursive-descent flow with a shared error sink:

```
source bytes ──► Lexer ──► vector<Token> ──► Parser ──► ProgramNode (AST)
                    │                            │
                    └─────► ErrorCollector ◄─────┘
```

### Single source of truth: `isa.h`

`isa.h` defines the four enums that the rest of the codebase keys off:
- `TOKEN_TYPE` — INT, FLOAT, STRING, INSTRUCTION, DIRECTIVE, LABEL_DEF, IDENTIFIER, OPERATOR, END_OF_FILE
- `OP_CODE` — stack/memory, math, bitwise, flow, and `SYS_*` syscalls
- `DIRECTIVE` — section markers (`.data`, `.code`, `.equ`) and data type directives (`.byte`, `.short`, `.word`, `.dword`, `.string`, `.fill`)
- `OPERATOR` — `+`, `-`

When extending the language, add to these enums first, then update `isa_helper.cpp` (which provides bidirectional `Stringify::*` / `Convert::*` between strings and enum values — both lexer keyword recognition and AST pretty-printing depend on it).

### Token representation

`Token` (in `lexer.h`) carries its value in a `std::variant<monostate, string, int, float, OP_CODE, DIRECTIVE, OPERATOR>`. Typed accessors (`getInt`, `getOpCode`, ...) and predicates (`hasInt`, ...) wrap `std::get` / `std::holds_alternative`. Use those rather than touching the variant directly.

### Error handling pattern

Errors do **not** throw. `ErrorCollector` accumulates `ErrorMessage`s; both lexer and parser take it by reference. After each phase, `main.cpp` checks `errors.hasError()` and exits early. Do not introduce exceptions for parse/lex errors — follow the collect-and-continue pattern. New error kinds go in the `ErrorType` enum in `error.h` and get a format string in `error.cpp`.

### Parser (recursive descent)

Grammar is documented in the comment at the top of `parser.h`:

```
program ::= (data_section | equ_section | code_section)*
instruction ::= COMMAND operand*
operand ::= const_eval_expression | term
const_eval_expression ::= term (binary_operator term)*
```

AST node hierarchy lives in `parser.h`: `ASTNode` → `ExpressionNode` (→ `NumberNode`, `IdentifierNode`, `BinaryOperationNode`) plus `InstructionNode`, `DataNode`, `*SectionNode`, `EquItemNode`, `ProgramNode`. Section nodes own their children via `unique_ptr`; `ProgramNode` keeps `codeNodes` / `dataNodes` as separate vectors plus an `equSection` map keyed by identifier.

`Parser` exposes `peek/advance/check/match/expect` helpers with overloads per enum. `expect()` currently advances even on mismatch (this and other rough edges are catalogued in `PARSER_REVIEW.md` — read it before fixing parser bugs to avoid duplicating work or re-discovering known issues).

## `.tasm` language quick reference

A program is composed of three section kinds, each introduced by a directive and freely interleaveable:

- `.data` — labeled storage: `label:` followed by `.byte <ints>+` or `.string "..."` (parser also accepts `.short`, `.word`, `.dword`)
- `.equ` — compile-time constants: `IDENTIFIER literal`
- `.code` — instructions: `OPCODE operand*` where operands are numbers, identifiers, or `term (+|- term)*` const expressions

Identifiers follow C rules (letter/`_` then alphanumeric/`_`). Numeric literals support decimal, `0x` hex, and `0b` binary. Strings are double-quoted. See `tests/main.tasm` for a representative program exercising most of the ISA.

## Style

- `.clang-format`: LLVM base, Allman braces, 4-space indent, `PointerAlignment: Left` (`int* p`, not `int *p`).
- `.editorconfig` says tabs for `.cpp/.h`, but existing code is space-indented — follow the surrounding file rather than the editorconfig.
- File layout convention: `<feature>.h` declares, `<feature>.cpp` implements; `print()` overrides currently live inline in `parser.h` headers (a known wart per the review).

## Notes on stray files

- `#parser.cpp#` is an Emacs autosave artifact — ignore, do not commit.
- `PARSER_REVIEW.md` is a working review document, not user docs.
- `build/` contains build outputs (`.exe`, `.pdb`, `.ilk`); these are gitignored.
