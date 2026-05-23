#include "semantic.h"

#include <iostream>

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
            ErrorType::SYMBOL_REDEFINITION,
            line,
            column,
            { declaredSymbol->label }
        );
        
        return false;
    }

    m_symbols.insert({declaredSymbol->label, Symbol {
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


std::optional<std::variant<int, float>> SemanticAnalyzer::foldExpression()
{
    
}

void SemanticAnalyzer::resolveAndValidate()
{
    
}


void SemanticAnalyzer::printDeclaryedSymbols() const
{
    for (const auto& pair : m_symbols)
    {
        std::cout << pair.first << " = 0" << std::endl;
    }
}
