#pragma once

#include <string_view>
#include <optional>

enum class TOKEN_TYPE
{
    INT = 0x0,
    FLOAT,
    STRING,
    INSTRUCTION,
    DIRECTIVE,
    LABEL_DEF,
    IDENTIFIER,
    OPERATOR,
    END_OF_FILE,
};

enum class OP_CODE
{
    // STACK AND MEMORY
    PUSH8 = 0x0,
    PUSH16,
    PUSH32,
    PUSHF16,
    PUSHF32,
    POP,
    DUP,
    SWAP,
    LOAD,
    STORE,
    // MATH
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    // BIT OPERATIONS
    SHL,
    SHR,
    AND,
    OR,
    EQ,
    NEQ,
    GT,
    LT,
    // FLOW
    JMP,
    JMP_F,
    HALT,
    // SYSTEM CALLS
    SYS_CLEAR,
    SYS_PIXEL,
    SYS_LINE,
    SYS_FRAME,
    SYS_BTN,
    SYS_PRINT,
    SYS_SPRITE,
};

enum class DIRECTIVE
{
    DATA = 0x0,
    CODE,
    BYTE,
    SHORT,
    WORD,
    DWORD,
    STRING,
    EQU,
    FILL,
};
 
enum class OPERATOR
{
    MINUS,
    PLUS,
    COMMA,
};

enum class OPERAND_KIND
{
    I8,
    I16,
    I32,
    F16,
    F32,
    DATA_LABEL,
    CODE_LABEL,
};


enum class SYMBOL_KIND
{
    CODE,
    EQU,
    DATA,
};
