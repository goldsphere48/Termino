#include "semantic.h"
#include "isa.h"
#include "parser.h"
#include "target.h"

#include <iostream>

std::optional<FoldedValue> FromSymbolValue(const SymbolValue value)
{
    return std::visit([](auto x) -> std::optional<FoldedValue> {
        if constexpr(std::is_same<decltype(x), termino_platform::Address>())
        {
            return std::nullopt;
        } else
        {
            return FoldedValue { x };
        }
    }, value);
}

SymbolValue ToSymbolValue(const FoldedValue value)
{
    return std::visit([](auto v) -> SymbolValue { return v; }, value);
}

FoldedValue ApplyUnariOperator(FoldedValue value, std::optional<OPERATOR> unarOperator)
{
    return std::visit([=](auto x) -> FoldedValue {
        return unarOperator == OPERATOR::MINUS  ? -x : x;
    }, value);
}

FoldedValue ApplyBinariOperator(FoldedValue left, FoldedValue right, OPERATOR oper)
{
    return std::visit([oper](auto l, auto r) {
        using Common = std::common_type_t<decltype(l), decltype(r)>;
        Common a = static_cast<Common>(l);
        Common b = static_cast<Common>(r);
        switch (oper)
        {
            case OPERATOR::MINUS:
                return FoldedValue { static_cast<Common>(a - b) };
            case OPERATOR::PLUS:
                return FoldedValue { static_cast<Common>(a + b) };
            default:
                return FoldedValue { static_cast<Common>(a) };
        }
    }, left, right);
}

SemanticAnalyzer::SemanticAnalyzer(const ProgramNode& root, ErrorCollector& errorCollector)
    : m_root(root), m_errorCollector(errorCollector)
{
    m_instructionSpecs = {
        { OP_CODE::PUSH8,  InstructionSpec { .operands = { OPERAND_KIND::I8         } }},
        { OP_CODE::PUSH16, InstructionSpec { .operands = { OPERAND_KIND::I16        } }},
        { OP_CODE::PUSHF,  InstructionSpec { .operands = { OPERAND_KIND::F16        } }},
        { OP_CODE::JMP,    InstructionSpec { .operands = { OPERAND_KIND::CODE_LABEL } }},
        { OP_CODE::JMP_F,  InstructionSpec { .operands = { OPERAND_KIND::CODE_LABEL } }},
        { OP_CODE::LOAD,   InstructionSpec { .operands = { OPERAND_KIND::U32        } }},
        { OP_CODE::STORE,  InstructionSpec { .operands = { OPERAND_KIND::U32        } }},
    };
}

void SemanticAnalyzer::analyze()
{
    collectSymbols();
    resolveAndValidate();
}

bool SemanticAnalyzer::tryAddSymbol(std::optional<DeclaredSymbol> declaredSymbol, size_t line, size_t column)
{
    if (declaredSymbol == std::nullopt)
    {
        return false;
    }
    
    if (m_symbols.find(declaredSymbol->label) != m_symbols.end())
    {
        m_errorCollector.report(
            ERROR_TYPE::SYMBOL_REDEFINITION,
            line,
            column,
            { declaredSymbol->label }
        );
        
        return false;
    }

    m_symbols.insert({declaredSymbol->label, Symbol {
            .expression = declaredSymbol->pExpression,
            .kind = declaredSymbol->kind,
            .line = line,
            .column = column,
        }}
    );

    return true;
}

void SemanticAnalyzer::collectSymbols()
{
    for (const std::unique_ptr<ASTNode>& node : m_root.nodes)
    {
        if (
            const auto& equSection = node->as<EquSectionNode>();
            equSection != nullptr
        )
        {
            for (const std::unique_ptr<EquItemNode>& equItem : equSection->nodes)
            {
               tryAddSymbol(
                   equItem->declaredSymbol(),
                   equItem->line,
                   equItem->column
               );
            }
        }
         
        if (
            const auto& dataSection = node->as<DataSectionNode>();
            dataSection != nullptr
        )
        {
            for (const std::unique_ptr<DataNode>& dataItem : dataSection->datas)
            {
               tryAddSymbol(
                   dataItem->declaredSymbol(),
                   dataItem->line,
                   dataItem->column
               );
            }
        }

         if (
             const auto& codeSection = node->as<CodeSectionNode>();
             codeSection != nullptr
         )
         {
             for (const std::unique_ptr<CodeStatementNode>& stmt : codeSection->nodes)
             {
                 tryAddSymbol(
                     stmt->declaredSymbol(),
                     stmt->line,
                     stmt->column
                 );
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
    auto folded = foldExpression(symbol.expression);
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
                { identifier->identifier }
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
        auto left = foldExpression(binaryOp->left.get());
        auto right = foldExpression(binaryOp->right.get());
        if (!left || !right)
        {
            return std::nullopt;
        }
        
        return ApplyBinariOperator(*left, *right, binaryOp->oper);
    }
    
    
    return std::nullopt;
}

void SemanticAnalyzer::resolveAndValidate()
{
    for (auto&[key, symbol] : m_symbols)
    {
        if (symbol.kind != SYMBOL_KIND::EQU)
        {
            continue;
        }

        resolveSymbol(key);
    }
}


void SemanticAnalyzer::printDeclaryedSymbols() const
{
    for (const auto& [name, symbol] : m_symbols)
    {
        std::cout << name << " = ";
        std::visit([](auto value) { std::cout << value; }, symbol.value);
        std::cout << std::endl;
    }
}
