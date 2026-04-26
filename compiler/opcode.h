#pragma once

#include <string_view>

enum class OP_CODE
{
    PUSH8 = 0x0,
    PUSH16,
    PUSHF,
    POP,
    ADD,
    SUB,
    MUL,
};

std::string_view OpCodeToString(OP_CODE opCode);
 
