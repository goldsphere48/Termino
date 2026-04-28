#include "isa_helper.h"

#include <unordered_map>
#include <string>

namespace Stringify
{
    std::string_view tokenType(TOKEN_TYPE type)
    {
        switch (type)
        {
        case TOKEN_TYPE::INT:         return "INT";
        case TOKEN_TYPE::FLOAT:       return "FLOAT";
        case TOKEN_TYPE::INSTRUCTION: return "INSTRUCTION";
        case TOKEN_TYPE::DIRECTIVE:   return "DIRECTIVE";
        case TOKEN_TYPE::LABEL_DEF:   return "LABEL_DEF";
        case TOKEN_TYPE::IDENTIFIER:  return "IDENTIFIER";
        case TOKEN_TYPE::OPERATOR:    return "OPERATOR";
        }

        return "?";
    }

    std::string_view opCode(OP_CODE type)
    {
        switch (type)
        {
        case OP_CODE::PUSH8:      return "PUSH8";
        case OP_CODE::PUSH16:     return "PUSH16";
        case OP_CODE::PUSHF:      return "PUSHF";
        case OP_CODE::POP:        return "POP";
        case OP_CODE::DUP:        return "DUP";
        case OP_CODE::SWAP:       return "SWAP";
        case OP_CODE::LOAD:       return "LOAD";
        case OP_CODE::STORE:      return "STORE";
        case OP_CODE::ADD:        return "ADD";
        case OP_CODE::SUB:        return "SUB";
        case OP_CODE::MUL:        return "MUL";
        case OP_CODE::DIV:        return "DIV";
        case OP_CODE::MOD:        return "MOD";
        case OP_CODE::SHL:        return "SHL";
        case OP_CODE::SHR:        return "SHR";
        case OP_CODE::AND:        return "AND";
        case OP_CODE::OR:         return "OR";
        case OP_CODE::EQ:         return "EQ";
        case OP_CODE::NEQ:        return "NEQ";
        case OP_CODE::GT:         return "GT";
        case OP_CODE::LT:         return "LT";
        case OP_CODE::JMP:        return "JMP";
        case OP_CODE::JMP_F:      return "JMP_F";
        case OP_CODE::HALT:       return "HALT";
        case OP_CODE::SYS_CLEAR:  return "SYS_CLEAR";
        case OP_CODE::SYS_PIXEL:  return "SYS_PIXEL";
        case OP_CODE::SYS_LINE:   return "SYS_LINE";
        case OP_CODE::SYS_FRAME:  return "SYS_FRAME";
        case OP_CODE::SYS_BTN:    return "SYS_BTN";
        case OP_CODE::SYS_PRINT:  return "SYS_PRINT";
        case OP_CODE::SYS_SPRITE: return "SYS_SPRITE";
        }

        return "?";
    }

    std::string_view directive(DIRECTIVE type)
    {
        switch (type)
        {
        case DIRECTIVE::DATA:  return "DATA";
        case DIRECTIVE::CODE:  return "CODE";
        case DIRECTIVE::BYTES: return "BYTES";
        }

        return "?";
    }

    std::string_view oper(OPERATOR type)
    {
        switch (type)
        {
        case OPERATOR::MINUS:   return "MINUS";
        case OPERATOR::PLUS:    return "PLUS";
        case OPERATOR::DIV:     return "DIV";
        case OPERATOR::MUL:     return "MUL";
        case OPERATOR::MOD:     return "MOD";
        case OPERATOR::BIT_AND: return "BIT_AND";
        case OPERATOR::BIT_OR:  return "BIT_OR";
        case OPERATOR::BIT_NOT: return "BIT_NOT";
        case OPERATOR::BIT_XOR: return "BIT_XOR";
        }

        return "?";
    }
}

namespace Convert
{
    std::optional<TOKEN_TYPE> tokenType(std::string_view str)
    {
        static const std::unordered_map<std::string_view, TOKEN_TYPE> table = {
            { "INT",         TOKEN_TYPE::INT         },
            { "FLOAT",       TOKEN_TYPE::FLOAT       },
            { "INSTRUCTION", TOKEN_TYPE::INSTRUCTION },
            { "DIRECTIVE",   TOKEN_TYPE::DIRECTIVE   },
            { "LABEL_DEF",   TOKEN_TYPE::LABEL_DEF   },
            { "IDENTIFIER",  TOKEN_TYPE::IDENTIFIER  },
            { "OPERATOR",    TOKEN_TYPE::OPERATOR    },
        };

        if (auto it = table.find(str); it != table.end())
        {
            return it->second;
        }

        return std::nullopt;
    }

    std::optional<OP_CODE> opCode(std::string_view str)
    {
        static const std::unordered_map<std::string_view, OP_CODE> table = {
            { "PUSH8",      OP_CODE::PUSH8      },
            { "PUSH16",     OP_CODE::PUSH16     },
            { "PUSHF",      OP_CODE::PUSHF      },
            { "POP",        OP_CODE::POP        },
            { "DUP",        OP_CODE::DUP        },
            { "SWAP",       OP_CODE::SWAP       },
            { "LOAD",       OP_CODE::LOAD       },
            { "STORE",      OP_CODE::STORE      },
            { "ADD",        OP_CODE::ADD        },
            { "SUB",        OP_CODE::SUB        },
            { "MUL",        OP_CODE::MUL        },
            { "DIV",        OP_CODE::DIV        },
            { "MOD",        OP_CODE::MOD        },
            { "SHL",        OP_CODE::SHL        },
            { "SHR",        OP_CODE::SHR        },
            { "AND",        OP_CODE::AND        },
            { "OR",         OP_CODE::OR         },
            { "EQ",         OP_CODE::EQ         },
            { "NEQ",        OP_CODE::NEQ        },
            { "GT",         OP_CODE::GT         },
            { "LT",         OP_CODE::LT         },
            { "JMP",        OP_CODE::JMP        },
            { "JMP_F",      OP_CODE::JMP_F      },
            { "HALT",       OP_CODE::HALT       },
            { "SYS_CLEAR",  OP_CODE::SYS_CLEAR  },
            { "SYS_PIXEL",  OP_CODE::SYS_PIXEL  },
            { "SYS_LINE",   OP_CODE::SYS_LINE   },
            { "SYS_FRAME",  OP_CODE::SYS_FRAME  },
            { "SYS_BTN",    OP_CODE::SYS_BTN    },
            { "SYS_PRINT",  OP_CODE::SYS_PRINT  },
            { "SYS_SPRITE", OP_CODE::SYS_SPRITE },
        };

        if (auto it = table.find(str); it != table.end())
        {
            return it->second;
        }

        return std::nullopt;
    }

    std::optional<DIRECTIVE> directive(std::string_view str)
    {
        static const std::unordered_map<std::string_view, DIRECTIVE> table = {
            { "DATA",  DIRECTIVE::DATA  },
            { "CODE",  DIRECTIVE::CODE  },
            { "BYTES", DIRECTIVE::BYTES },
        };

        if (auto it = table.find(str); it != table.end())
        {
            return it->second;
        }

        return std::nullopt;
    }

    std::optional<OPERATOR> oper(std::string_view str)
    {
        static const std::unordered_map<std::string_view, OPERATOR> table = {
            { "-", OPERATOR::MINUS   },
            { "+", OPERATOR::PLUS    },
            { "/", OPERATOR::DIV     },
            { "*", OPERATOR::MUL     },
            { "%", OPERATOR::MOD     },
            { "&", OPERATOR::BIT_AND },
            { "|", OPERATOR::BIT_OR  },
            { "~", OPERATOR::BIT_NOT },
            { "^", OPERATOR::BIT_XOR },
        };

        if (auto it = table.find(str); it != table.end())
        {
            return it->second;
        }

        return std::nullopt;
    }
}
