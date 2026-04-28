#pragma once

#include <string_view>
#include <optional>

enum class TOKEN_TYPE
{
    INT,
    FLOAT,
    INSTRUCTION,
    DIRECTIVE,
    LABEL_DEF,
    IDENTIFIER,
    OPERATOR,
};

enum class OP_CODE
{
    // STACK AND MEMORY
    PUSH8 = 0x0,
    PUSH16,
    PUSHF,
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
    DATA,
    CODE,
    BYTES,
};
 
enum class OPERATOR
{
    MINUS,
    PLUS,
    DIV,
    MUL,
    MOD,
    BIT_AND,
    BIT_OR,
    BIT_NOT,
    BIT_XOR,
};
