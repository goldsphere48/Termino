#pragma once

#include "isa.h"

#include <string>

struct Token;

namespace Stringify
{
    std::string_view tokenType(TOKEN_TYPE type);
    std::string_view opCode(OP_CODE type);
    std::string_view directive(DIRECTIVE type);
    std::string_view oper(OPERATOR type);
    std::string tokenValue(const Token& token);
}

namespace Convert
{
    std::optional<TOKEN_TYPE> tokenType(std::string_view str);
    std::optional<OP_CODE> opCode(std::string_view str);
    std::optional<DIRECTIVE> directive(std::string_view str);
    std::optional<OPERATOR> oper(std::string_view str);
}
