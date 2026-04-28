#include "isa.h"

std::string_view OpCodeToString(OP_CODE opCode)
{
    switch(opCode)
    {
    case OP_CODE::PUSH8:
        return "PUSH8";
    case OP_CODE::PUSH16:
        return "PUSH16";
    case OP_CODE::PUSHF:
        return "PUSHF";
    case OP_CODE::POP:
        return "POP";
    case OP_CODE::ADD:
        return "ADD";
    case OP_CODE::SUB:
        return "SUB";
    case OP_CODE::MUL:
        return "MUL";
    }

    return "UNKNOWN";
}
