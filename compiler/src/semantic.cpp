#include "semantic.h"

#include "isa.h"
#include "parser.h"
#include "target.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <variant>

static bool IsFloatType(OPERAND_KIND type)
{
    return type == OPERAND_KIND::F16 || type == OPERAND_KIND::F32;
}

static bool IsIntType(OPERAND_KIND type)
{
    return type == OPERAND_KIND::I8 || type == OPERAND_KIND::I16 || type == OPERAND_KIND::I32;
}

static bool IsNumberType(OPERAND_KIND type)
{
    return IsFloatType(type) || IsIntType(type);
}

static bool MatchType(FoldedValue value, OPERAND_KIND type)
{
    return (std::holds_alternative<float>(value) && IsFloatType(type))
           || (std::holds_alternative<int64_t>(value) && IsIntType(type));
}

// Largest finite magnitude representable by an IEEE-754 half (f16).
constexpr float HALF_MAX = 65504.0f;

int GetBitsCount(OPERAND_KIND type)
{
    switch (type)
    {
        case OPERAND_KIND::I8:
            return 8;
        case OPERAND_KIND::I16:
            return 16;
        case OPERAND_KIND::I32:
            return 32;
        case OPERAND_KIND::F16:
            return 16;
        case OPERAND_KIND::F32:
            return 32;
    }

    return 0;
}

// Accepts the union of the signed and unsigned ranges for an N-bit field
// (assembler convention): value is representable in `bits` bits as signed OR
// unsigned. Computed in 64-bit, so `bits` must be in (0, 63].
static bool FitsInBits(int64_t value, int bits)
{
    int64_t signedMin   = -(int64_t{1} << (bits - 1));  // -2^(bits-1)
    int64_t unsignedMax = (int64_t{1} << bits) - 1;     //  2^bits - 1
    return value >= signedMin && value <= unsignedMax;
}

static bool RangeCheck(FoldedValue value, OPERAND_KIND type)
{
    if (!MatchType(value, type))
    {
        return false;
    }

    switch (type)
    {
        case OPERAND_KIND::I8:
        case OPERAND_KIND::I16:
        case OPERAND_KIND::I32:
        {
            int bits = GetBitsCount(type);
            return FitsInBits(std::get<int64_t>(value), bits);
        }
        case OPERAND_KIND::F16:
        {
            float floatValue = std::get<float>(value);
            return std::isfinite(floatValue) && std::fabs(floatValue) <= HALF_MAX;
        }
        case OPERAND_KIND::F32:
        {
            return std::holds_alternative<float>(value);
        }
        default:
            return false;
    }
}

static std::string OperandKindName(OPERAND_KIND kind)
{
    switch (kind)
    {
        case OPERAND_KIND::I8:
            return "i8";
        case OPERAND_KIND::I16:
            return "i16";
        case OPERAND_KIND::I32:
            return "i32";
        case OPERAND_KIND::F16:
            return "f16";
        case OPERAND_KIND::F32:
            return "f32";
        case OPERAND_KIND::DATA_LABEL:
            return "data label";
        case OPERAND_KIND::CODE_LABEL:
            return "code label";
    }

    return "?";
}

static std::string FoldedValueToString(FoldedValue value)
{
    return std::visit(
        [](auto v)
        {
            return std::to_string(v);
        },
        value
    );
}

static std::string FoldedValueTypeName(FoldedValue value)
{
    return std::holds_alternative<float>(value) ? "float" : "integer";
}

static std::string OperandText(const ExpressionNode* operand)
{
    if (const IdentifierNode* identifier = operand->as<IdentifierNode>())
    {
        return identifier->identifier;
    }

    return "expression";
}

std::optional<FoldedValue> FromSymbolValue(const SymbolValue value)
{
    return std::visit(
        [](auto x) -> std::optional<FoldedValue>
        {
            if constexpr (std::is_same<decltype(x), termino_platform::Address>())
            {
                return std::nullopt;
            }
            else
            {
                return FoldedValue{x};
            }
        },
        value
    );
}

SymbolValue ToSymbolValue(const FoldedValue value)
{
    return std::visit(
        [](auto v) -> SymbolValue
        {
            return v;
        },
        value
    );
}

FoldedValue ApplyUnariOperator(FoldedValue value, std::optional<OPERATOR> unarOperator)
{
    return std::visit(
        [=](auto x) -> FoldedValue
        {
            return unarOperator == OPERATOR::MINUS ? -x : x;
        },
        value
    );
}

FoldedValue ApplyBinariOperator(FoldedValue left, FoldedValue right, OPERATOR oper)
{
    return std::visit(
        [oper](auto l, auto r)
        {
            using Common = std::common_type_t<decltype(l), decltype(r)>;
            Common a     = static_cast<Common>(l);
            Common b     = static_cast<Common>(r);
            switch (oper)
            {
                case OPERATOR::MINUS:
                    return FoldedValue{static_cast<Common>(a - b)};
                case OPERATOR::PLUS:
                    return FoldedValue{static_cast<Common>(a + b)};
                default:
                    return FoldedValue{static_cast<Common>(a)};
            }
        },
        left,
        right
    );
}

SemanticAnalyzer::SemanticAnalyzer(const ProgramNode& root, ErrorCollector& errorCollector)
    : m_root(root), m_errorCollector(errorCollector)
{
    m_instructionSpecs = {
        {OP_CODE::PUSH8, InstructionSpec{.operands = {OPERAND_KIND::I8}}},
        {OP_CODE::PUSH16, InstructionSpec{.operands = {OPERAND_KIND::I16}}},
        {OP_CODE::PUSH32, InstructionSpec{.operands = {OPERAND_KIND::I32}}},
        {OP_CODE::PUSHF16, InstructionSpec{.operands = {OPERAND_KIND::F16}}},
        {OP_CODE::PUSHF32, InstructionSpec{.operands = {OPERAND_KIND::F32}}},
        {OP_CODE::JMP, InstructionSpec{.operands = {OPERAND_KIND::CODE_LABEL}}},
        {OP_CODE::JMP_F, InstructionSpec{.operands = {OPERAND_KIND::CODE_LABEL}}},
        {OP_CODE::LOAD, InstructionSpec{.operands = {OPERAND_KIND::DATA_LABEL}}},
        {OP_CODE::STORE, InstructionSpec{.operands = {OPERAND_KIND::DATA_LABEL}}},
        {OP_CODE::POP, InstructionSpec{.operands = {}}},
        {OP_CODE::DUP, InstructionSpec{.operands = {}}},
        {OP_CODE::SWAP, InstructionSpec{.operands = {}}},
        {OP_CODE::ADD, InstructionSpec{.operands = {}}},
        {OP_CODE::SUB, InstructionSpec{.operands = {}}},
        {OP_CODE::MUL, InstructionSpec{.operands = {}}},
        {OP_CODE::DIV, InstructionSpec{.operands = {}}},
        {OP_CODE::MOD, InstructionSpec{.operands = {}}},
        {OP_CODE::SHL, InstructionSpec{.operands = {}}},
        {OP_CODE::SHR, InstructionSpec{.operands = {}}},
        {OP_CODE::AND, InstructionSpec{.operands = {}}},
        {OP_CODE::OR, InstructionSpec{.operands = {}}},
        {OP_CODE::EQ, InstructionSpec{.operands = {}}},
        {OP_CODE::NEQ, InstructionSpec{.operands = {}}},
        {OP_CODE::GT, InstructionSpec{.operands = {}}},
        {OP_CODE::LT, InstructionSpec{.operands = {}}},
        {OP_CODE::HALT, InstructionSpec{.operands = {}}},
        {OP_CODE::SYS_CLEAR, InstructionSpec{.operands = {}}},
        {OP_CODE::SYS_PIXEL, InstructionSpec{.operands = {}}},
        {OP_CODE::SYS_LINE, InstructionSpec{.operands = {}}},
        {OP_CODE::SYS_FRAME, InstructionSpec{.operands = {}}},
        {OP_CODE::SYS_BTN, InstructionSpec{.operands = {}}},
        {OP_CODE::SYS_PRINT, InstructionSpec{.operands = {}}},
        {OP_CODE::SYS_SPRITE, InstructionSpec{.operands = {}}},
    };
}

void SemanticAnalyzer::analyze()
{
    collectSymbols();
    resolveAndValidate();
}

bool SemanticAnalyzer::tryAddSymbol(
    std::optional<DeclaredSymbol> declaredSymbol,
    size_t                        line,
    size_t                        column
)
{
    if (declaredSymbol == std::nullopt)
    {
        return false;
    }

    if (m_symbols.find(declaredSymbol->label) != m_symbols.end())
    {
        m_errorCollector
            .report(ERROR_TYPE::SYMBOL_REDEFINITION, line, column, {declaredSymbol->label});

        return false;
    }

    m_symbols.insert(
        {declaredSymbol->label,
         Symbol{
             .expression = declaredSymbol->pExpression,
             .kind       = declaredSymbol->kind,
             .line       = line,
             .column     = column,
         }}
    );

    return true;
}

void SemanticAnalyzer::collectSymbols()
{
    for (const std::unique_ptr<ASTNode>& node : m_root.nodes)
    {
        if (const auto& equSection = node->as<EquSectionNode>(); equSection != nullptr)
        {
            for (const std::unique_ptr<EquItemNode>& equItem : equSection->nodes)
            {
                tryAddSymbol(equItem->declaredSymbol(), equItem->line, equItem->column);
            }
        }

        if (const auto& dataSection = node->as<DataSectionNode>(); dataSection != nullptr)
        {
            for (const std::unique_ptr<DataNode>& dataItem : dataSection->datas)
            {
                tryAddSymbol(dataItem->declaredSymbol(), dataItem->line, dataItem->column);
            }
        }

        if (const auto& codeSection = node->as<CodeSectionNode>(); codeSection != nullptr)
        {
            for (const std::unique_ptr<CodeStatementNode>& stmt : codeSection->nodes)
            {
                tryAddSymbol(stmt->declaredSymbol(), stmt->line, stmt->column);
            }
        }
    }
}

std::optional<FoldedValue> SemanticAnalyzer::resolveSymbol(const std::string& name)
{
    Symbol& symbol = m_symbols[name];
    if (symbol.state == RESOLVE_STATE::DONE)
    {
        return FromSymbolValue(symbol.value);
    }

    if (symbol.state == RESOLVE_STATE::IN_PROGRESS)
    {
        m_errorCollector.report(ERROR_TYPE::CYCLED_DEPENDECIE, symbol.line, symbol.column);
        symbol.state = RESOLVE_STATE::DONE;
        return std::nullopt;
    }

    symbol.state = RESOLVE_STATE::IN_PROGRESS;
    auto folded  = foldExpression(symbol.expression);
    symbol.state = RESOLVE_STATE::DONE;

    if (folded)
    {
        symbol.value = ToSymbolValue(*folded);
    }

    return folded;
}

std::optional<FoldedValue> SemanticAnalyzer::foldExpression(const ExpressionNode* expression)
{
    if (auto* n = expression->as<NumberNode>())
    {
        return ApplyUnariOperator(n->value, expression->unarOperator);
    }
    else if (auto* identifier = expression->as<IdentifierNode>())
    {
        auto it = m_symbols.find(identifier->identifier);
        if (it == m_symbols.end())
        {
            m_errorCollector.report(
                ERROR_TYPE::UNDEFINED_SYMBOL,
                expression->line,
                expression->column,
                {identifier->identifier}
            );

            return std::nullopt;
        }

        if (it->second.kind != SYMBOL_KIND::EQU)
        {
            return std::nullopt;
        }

        auto resolved = resolveSymbol(identifier->identifier);
        if (!resolved)
        {
            return std::nullopt;
        }

        return ApplyUnariOperator(*resolved, identifier->unarOperator);
    }
    else if (const auto* binaryOp = expression->as<BinaryOperationNode>())
    {
        auto left  = foldExpression(binaryOp->left.get());
        auto right = foldExpression(binaryOp->right.get());
        if (!left || !right)
        {
            return std::nullopt;
        }

        return ApplyBinariOperator(*left, *right, binaryOp->oper);
    }

    return std::nullopt;
}

void SemanticAnalyzer::validateInstruction(CodeStatementNode& stmt)
{
    InstructionNode* instruction = stmt.as<InstructionNode>();
    if (!instruction)
    {
        return;
    }

    if (m_instructionSpecs.find(instruction->command) == m_instructionSpecs.end())
    {
        m_errorCollector
            .report(ERROR_TYPE::COMPILER_INTERNAL_ERROR, instruction->line, instruction->column);
        return;
    }

    InstructionSpec& spec = m_instructionSpecs[instruction->command];
    if (instruction->operands.size() != spec.operands.size())
    {
        m_errorCollector.report(
            ERROR_TYPE::WRONG_OPERAND_COUNT,
            instruction->line,
            instruction->column,
            {Stringify::opCode(instruction->command),
             std::to_string(spec.operands.size()),
             std::to_string(instruction->operands.size())}
        );

        return;
    }

    for (size_t i = 0; i < instruction->operands.size(); ++i)
    {
        auto&        operand           = instruction->operands[i];
        OPERAND_KIND expectOperandKind = spec.operands[i];

        // Label operands stay as identifiers; their address is resolved in
        // Pass 3.
        if (expectOperandKind == OPERAND_KIND::CODE_LABEL
            || expectOperandKind == OPERAND_KIND::DATA_LABEL)
        {
            const IdentifierNode* identifier = operand->as<IdentifierNode>();
            if (!identifier)
            {
                m_errorCollector.report(
                    ERROR_TYPE::WRONG_OPERAND_TYPE,
                    operand->line,
                    operand->column,
                    {Stringify::opCode(instruction->command),
                     OperandKindName(expectOperandKind),
                     "expression"}
                );
                continue;
            }

            auto it = m_symbols.find(identifier->identifier);
            if (it == m_symbols.end())
            {
                m_errorCollector.report(
                    ERROR_TYPE::UNDEFINED_SYMBOL,
                    operand->line,
                    operand->column,
                    {identifier->identifier}
                );
                continue;
            }

            SYMBOL_KIND wantSymbol = expectOperandKind == OPERAND_KIND::CODE_LABEL
                                         ? SYMBOL_KIND::CODE
                                         : SYMBOL_KIND::DATA;
            if (it->second.kind != wantSymbol)
            {
                m_errorCollector.report(
                    ERROR_TYPE::WRONG_OPERAND_TYPE,
                    operand->line,
                    operand->column,
                    {Stringify::opCode(instruction->command),
                     OperandKindName(expectOperandKind),
                     identifier->identifier}
                );
            }

            continue;
        }

        // Numeric operands must fold to a constant of the right type and width.
        if (IsNumberType(expectOperandKind))
        {
            std::optional<FoldedValue> folded = foldExpression(operand.get());
            if (!folded)
            {
                // foldExpression already reports UNDEFINED_SYMBOL for unknown
                // names; anything still unfolded means the operand isn't a
                // compile-time constant (e.g. it references a label) where a
                // number is required.
                m_errorCollector.report(
                    ERROR_TYPE::NON_CONSTANT_EXPRESSION,
                    operand->line,
                    operand->column,
                    {OperandText(operand.get())}
                );
                continue;
            }

            if (!MatchType(*folded, expectOperandKind))
            {
                m_errorCollector.report(
                    ERROR_TYPE::WRONG_OPERAND_TYPE,
                    operand->line,
                    operand->column,
                    {Stringify::opCode(instruction->command),
                     OperandKindName(expectOperandKind),
                     FoldedValueTypeName(*folded)}
                );
                continue;
            }

            if (!RangeCheck(*folded, expectOperandKind))
            {
                m_errorCollector.report(
                    ERROR_TYPE::VALUE_OUT_OF_RANGE,
                    operand->line,
                    operand->column,
                    {FoldedValueToString(*folded), OperandKindName(expectOperandKind)}
                );
                continue;
            }

            std::unique_ptr<NumberNode> number = std::make_unique<NumberNode>();
            number->value                      = *folded;
            instruction->operands[i]           = std::move(number);
        }
    }
}

void SemanticAnalyzer::validateDataNode(DataNode& data)
{
    DIRECTIVE directive = data.dataDirective;
    for (const auto& e : data.expressions)
    {
        if (const IdentifierNode* identifier = e->as<IdentifierNode>())
        {
            auto it = m_symbols.find(identifier->identifier);
            if (it == m_symbols.end())
            {
            }
        }
    }
}

void SemanticAnalyzer::resolveAndValidate()
{
    for (auto& [key, symbol] : m_symbols)
    {
        if (symbol.kind != SYMBOL_KIND::EQU)
        {
            continue;
        }

        resolveSymbol(key);
    }

    for (auto& n : m_root.nodes)
    {
        if (auto* codeSection = n->as<CodeSectionNode>())
        {
            for (auto& stmt : codeSection->nodes)
            {
                validateInstruction(*stmt);
            }
        }

        if (auto* dataSection = n->as<DataSectionNode>())
        {
            for (auto& data : dataSection->datas)
            {
                validateDataNode(*data);
            }
        }
    }
}

void SemanticAnalyzer::printDeclaryedSymbols() const
{
    for (const auto& [name, symbol] : m_symbols)
    {
        std::cout << name << " = ";
        std::visit(
            [](auto value)
            {
                std::cout << value;
            },
            symbol.value
        );
        std::cout << std::endl;
    }
}
