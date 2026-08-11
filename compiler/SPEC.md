# Termino Language & ISA Specification

Status: **living document**. This is the single source of truth for the `.tasm`
language and the Termino VM instruction set. Where the compiler does not yet
implement something described here, it is marked **(planned)**.

The authoritative enums live in `include/isa.h`; the string↔enum mapping in
`src/isa_helper.cpp`; the instruction operand table in
`SemanticAnalyzer`'s `m_instructionSpecs` (`src/semantic.cpp`). Keep this file in
sync with those.

---

## 1. Target platform

From `shared/target.h`:

| Property        | Value                      |
|-----------------|----------------------------|
| Memory size     | 8 MiB (`8 * 1024 * 1024`)  |
| Addressing      | byte-addressable           |
| Address width   | 4 bytes (`uint32_t`)       |
| Stack cell      | **4 bytes (32-bit)**       |
| Endianness      | little-endian (host x86)   |

The runtime stack is **untyped 32-bit cells**. A cell holds either a signed
`int32` or an IEEE-754 `float32`; how the bits are interpreted is decided by the
*operation*, not stored with the value. Integer cells are **signed two's
complement**.

---

## 2. Design goal: compact binary

The VM uses uniform 4-byte cells at runtime, but the *binary* stores immediates
in the narrowest form that round-trips the value. That is the whole reason
`PUSH8`/`PUSH16`/`PUSH32` and `PUSHF16`/`PUSHF32` exist: a small constant costs 1
byte in the encoding, not 4, and is widened to a full cell when executed.

---

## 3. Lexical structure

- **Whitespace / newlines** separate tokens; otherwise insignificant.
- **Comments**: *not supported yet* **(planned)**.
- **Identifiers**: C rules — first char a letter or `_`, then
  letters / digits / `_`. Case-sensitive.
- **Label definition**: an identifier immediately followed by `:` (e.g.
  `counter:`).
- **Integer literals**: decimal, `0x`/`0X` hex, `0b`/`0B` binary. Optional
  leading `-` (unary minus).
- **Float literals**: decimal with a `.` (e.g. `3.14`). Optional leading `-`.
- **String literals**: double-quoted (`"..."`).
- **Operators**: `+`, `-`, `,`.
- **Instructions / directives** are recognized by keyword (see tables below).

---

## 4. Program structure

```
program      ::= (data_section | equ_section | code_section)*

code_section ::= ".code" (label_def | instruction)*
data_section ::= ".data" data_item*
equ_section  ::= ".equ"  equ_item*
```

Sections may appear in any order and may be repeated/interleaved. Symbols are
visible across all sections (forward references to `.code` labels and `.equ`
constants are allowed).

### 4.1 `.equ` — compile-time constants

```
equ_item ::= IDENTIFIER expression
```

The expression must fold to a number at compile time (literals, other `.equ`
constants, and `+`/`-` over them). Cyclic definitions are an error
(`CYCLED_DEPENDECIE`). An `.equ` symbol never has an address — it *is* its value.

### 4.2 `.data` — labeled storage

```
data_item      ::= LABEL_DEF data_directive
data_directive ::= (".byte" | ".short" | ".word" | ".dword") expression+
                 | ".string" STRING_LITERAL
```

Each label becomes a `DATA` symbol whose value is its **address**, assigned during
layout **(planned: Pass 3)**.

| Directive  | Element width | Notes                                   |
|------------|---------------|-----------------------------------------|
| `.byte`    | 1 byte        | each expression must fit in the width   |
| `.short`   | 2 bytes       |                                         |
| `.word`    | 4 bytes       |                                         |
| `.dword`   | 4 bytes       | currently same as `.word`               |
| `.string`  | 1 byte/char   | raw bytes of the string literal         |
| `.fill`    | —             | reserved, **(planned)**                 |

### 4.3 `.code` — instructions

```
label_def   ::= LABEL_DEF
instruction ::= COMMAND operand*
operand     ::= expression
```

Each `label_def` becomes a `CODE` symbol whose value is its address
**(planned: Pass 3)**.

---

## 5. Constant expressions

```
term            ::= number | IDENTIFIER
expression      ::= term (binary_operator term)?   // (planned: allow `*` repetition)
binary_operator ::= "+" | "-"
unary_operator  ::= "-"
```

Folding rules (`SemanticAnalyzer::foldExpression`):

- A `NumberNode` folds to its value.
- An `IDENTIFIER` that names an `.equ` constant folds to that constant's value.
- An `IDENTIFIER` that names a `.data`/`.code` label does **not** fold to a
  number (its address is unknown until layout) — it stays a label reference.
- `+` / `-` fold when **both** sides fold to numbers. Mixed int/float promotes to
  `float` (`std::common_type_t`).

So a folded operand is either a concrete number **or** an unresolved label
reference; anything else is an error.

---

## 6. Operand kinds

`OPERAND_KIND` (in `isa.h`) describes what each instruction operand must be.
Integer immediates are widened to a 32-bit cell at runtime.

| Kind         | Encoding        | Accepted source value                  | Widening    |
|--------------|-----------------|----------------------------------------|-------------|
| `I8`         | 1 byte          | const expr folding to integer          | sign-extend |
| `I16`        | 2 bytes         | const expr folding to integer          | sign-extend |
| `I32`        | 4 bytes         | const expr folding to integer          | exact       |
| `F16`        | 2 bytes (half)  | const expr folding to number           | f16→f32     |
| `F32`        | 4 bytes         | const expr folding to number           | exact       |
| `DATA_LABEL` | 4 bytes (addr)  | identifier naming a `.data` symbol      | —           |
| `CODE_LABEL` | 4 bytes (addr)  | identifier naming a `.code` symbol      | —           |

### 6.1 Signedness & immediate ranges

Narrow integer immediates **sign-extend** into the 32-bit cell (the cell is a
signed `int32`). For values `0..127` (byte) there is no difference between signed
and unsigned, so the common case "just works".

The assembler accepts the **union** of the signed and unsigned ranges and encodes
by truncating to the low N bits (so `PUSH8 -1` and `PUSH8 255` both emit byte
`0xFF`):

| Kind  | Accepted range          | Encoded as       |
|-------|-------------------------|------------------|
| `I8`  | `-128 .. 255`           | low 8 bits       |
| `I16` | `-32768 .. 65535`       | low 16 bits      |
| `I32` | `-2^31 .. 2^32-1`       | low 32 bits      |

> **Fold domain:** integer constant folding is done in a **64-bit** domain
> (`FoldedValue` holds `int64_t`, fed by `int64_t` number tokens), so the full
> signed *and* unsigned 32-bit ranges fold and range-check correctly. The result
> is truncated to the operand width on encoding; the runtime stack cell stays
> 32-bit.

> **Gotcha:** because widening is sign-extend, `PUSH8 200` puts `-56` on the
> stack, not `200`. To push the unsigned value `200` use a wider form
> (`PUSH16 200`) or mask explicitly. See §9 for the planned `PUSH` pseudo-op that
> hides this.

### 6.2 Float immediates

- `PUSHF16` stores a 2-byte IEEE half; widened to `float32` on load. Compact but
  **lossy** — not every literal is exactly representable.
- `PUSHF32` stores a full 4-byte `float32`; exact.

---

## 7. Instruction set

Stack effect notation: `( before -- after )`, top of stack on the right.
`imm` = immediate operand taken from the instruction stream.

### 7.1 Stack & memory

| Mnemonic  | Operand      | Stack effect          | Description                                   |
|-----------|--------------|-----------------------|-----------------------------------------------|
| `PUSH8`   | `I8`         | `( -- a )`            | push sign-extended 8-bit immediate            |
| `PUSH16`  | `I16`        | `( -- a )`            | push sign-extended 16-bit immediate           |
| `PUSH32`  | `I32`        | `( -- a )`            | push 32-bit immediate                         |
| `PUSHF16` | `F16`        | `( -- a )`            | push half-float immediate, widened to f32     |
| `PUSHF32` | `F32`        | `( -- a )`            | push 32-bit float immediate                   |
| `POP`     | —            | `( a -- )`            | discard top                                   |
| `DUP`     | —            | `( a -- a a )`        | duplicate top                                 |
| `SWAP`    | —            | `( a b -- b a )`      | swap top two                                  |
| `LOAD`    | `DATA_LABEL` | `( -- a )`            | push the cell at the label's address          |
| `STORE`   | `DATA_LABEL` | `( a -- )`            | pop and write to the label's address          |

> `LOAD`/`STORE` take the address as an **immediate operand** (a `.data` label),
> not from the stack.

### 7.2 Arithmetic

| Mnemonic | Stack effect       | Description          |
|----------|--------------------|----------------------|
| `ADD`    | `( a b -- a+b )`   | add                  |
| `SUB`    | `( a b -- a-b )`   | subtract             |
| `MUL`    | `( a b -- a*b )`   | multiply             |
| `DIV`    | `( a b -- a/b )`   | divide (signed)      |
| `MOD`    | `( a b -- a%b )`   | remainder (signed)   |

### 7.3 Bitwise & comparison

| Mnemonic | Stack effect          | Description                |
|----------|-----------------------|----------------------------|
| `SHL`    | `( a b -- a<<b )`     | shift left                 |
| `SHR`    | `( a b -- a>>b )`     | shift right (arithmetic)   |
| `AND`    | `( a b -- a&b )`      | bitwise and                |
| `OR`     | `( a b -- a\|b )`     | bitwise or                 |
| `EQ`     | `( a b -- a==b )`     | equal → 1/0                |
| `NEQ`    | `( a b -- a!=b )`     | not equal → 1/0            |
| `GT`     | `( a b -- a>b )`      | greater than (signed)      |
| `LT`     | `( a b -- a<b )`      | less than (signed)         |

### 7.4 Control flow

| Mnemonic | Operand      | Stack effect | Description                          |
|----------|--------------|--------------|--------------------------------------|
| `JMP`    | `CODE_LABEL` | `( -- )`     | unconditional jump                   |
| `JMP_F`  | `CODE_LABEL` | `( a -- )`   | pop; jump if zero/false              |
| `HALT`   | —            | `( -- )`     | stop execution                       |

### 7.5 System calls

All `SYS_*` take no immediate operands; arguments (if any) come from the stack.
Exact stack contracts are **(planned)** — to be pinned down with the VM.

| Mnemonic     | Description                |
|--------------|----------------------------|
| `SYS_CLEAR`  | clear the screen           |
| `SYS_PIXEL`  | draw a pixel               |
| `SYS_LINE`   | draw a line                |
| `SYS_FRAME`  | present a frame            |
| `SYS_BTN`    | read button state          |
| `SYS_PRINT`  | print                      |
| `SYS_SPRITE` | draw a sprite              |

---

## 8. Semantic analysis & errors

Pipeline (`SemanticAnalyzer`):

1. **collectSymbols** — gather `.equ` / `.data` / `.code` symbols; report
   cross-section duplicates (`SYMBOL_REDEFINITION`).
2. **resolveAndValidate** — fold `.equ` constants (cycle detection), then
   validate each instruction against its `InstructionSpec`: operand count, fold
   each operand to a number with a range check, or require a label of the right
   kind.
3. **computeLayout** **(planned)** — assign `.data`/`.code` addresses and replace
   remaining label references with their addresses.

Error kinds (`ERROR_TYPE` in `error.h`):

| Error                  | When                                              |
|------------------------|---------------------------------------------------|
| `SYMBOL_REDEFINITION`  | same name declared twice (any sections)           |
| `UNDEFINED_SYMBOL`     | identifier names no known symbol                  |
| `CYCLED_DEPENDECIE`    | `.equ` constants depend on each other cyclically  |
| `WRONG_OPERAND_COUNT`  | operand count ≠ instruction spec                  |
| `WRONG_OPERAND_TYPE`   | label/number mismatch for the expected kind **(planned)** |
| `VALUE_OUT_OF_RANGE`   | immediate doesn't fit the operand width **(planned)** |

---

## 9. Planned / open items

- **`PUSH` / `PUSHF` pseudo-instructions**: a single `PUSH <const>` where the
  assembler picks the narrowest opcode that round-trips the value
  (`PUSH8`→`PUSH16`→`PUSH32`; `PUSHF16` if exactly representable as half, else
  `PUSHF32`). Resolved at codegen; **not** an `OP_CODE`. This is the ergonomic
  front for the compact-encoding scheme.
- **Unsigned operations**: `DIVU`, `SHRU` (logical), `LTU`/`GTU` for unsigned
  compares. Deferred until a concrete need (likely address arithmetic). Keep the
  cell model signed by default.
- **Float arithmetic**: there are float pushes but no dedicated float math
  opcodes; decide whether `ADD`/… are polymorphic over the cell type or whether
  to add `FADD`/… .
- **`.fill` directive** and `.dword` vs `.word` distinction.
- **Comments** in the lexer.
- **`LOAD`/`STORE` access width**: currently one cell (4 bytes). If sub-cell
  access is needed, add width-tagged variants.
