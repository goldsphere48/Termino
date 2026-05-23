#include "parser.h"

#include <iostream>
#include <string>

namespace
{
    std::string pad(int indent)
    {
        return std::string(indent * 2, ' ');
    }
}

void NumberNode::print(int indent) const
{
    std::visit([this](auto&& v) {
        const auto value = unarOperator == OPERATOR::MINUS ? -v : v;
        std::cout << value;
    }, value);
}

void IdentifierNode::print(int indent) const
{
    std::cout << identifier;
}

void BinaryOperationNode::print(int indent) const
{
    std::cout << '(';
    left->print(indent);
    std::cout << ' ' << Stringify::oper(oper) << ' ';
    right->print(indent);
    std::cout << ')';
}

void InstructionNode::print(int indent) const
{
    std::cout << pad(indent) << Stringify::opCode(command);
    for (const auto& operand : operands)
    {
        std::cout << ' ';
        operand->print(indent);
    }
    std::cout << '\n';
}

void LabelDefNode::print(int indent) const
{
    std::cout << pad(indent) << identifier << ":\n";
}

void DataNode::print(int indent) const
{
    std::cout << pad(indent) << label << " ("
              << Stringify::directive(dataDirective) << "): ";

    if (dataDirective == DIRECTIVE::STRING)
    {
        std::cout << '"' << stringValue << '"';
    }
    else
    {
        for (size_t i = 0; i < expressions.size(); ++i)
        {
            if (i > 0) std::cout << ' ';
            expressions[i]->print(indent);
        }
    }
    std::cout << '\n';
}

void DataSectionNode::print(int indent) const
{
    std::cout << pad(indent) << "DATA SECTION\n";
    for (const auto& node : datas)
    {
        node->print(indent + 1);
    }
}

void CodeSectionNode::print(int indent) const
{
    std::cout << pad(indent) << "CODE SECTION\n";
    for (const auto& stmt : nodes)
    {
        stmt->print(indent + 1);
    }
}

void EquItemNode::print(int indent) const
{
    std::cout << pad(indent) << identifier << " = ";
    expression->print(indent);
    std::cout << '\n';
}

void EquSectionNode::print(int indent) const
{
    std::cout << pad(indent) << "EQU SECTION\n";
    for (const auto& node : nodes)
    {
        node->print(indent + 1);
    }
}

void ProgramNode::print(int indent) const
{
    std::cout << pad(indent) << "PROGRAM\n";

    for (const auto& node : nodes)
    {
        node->print(indent + 1);
    }
}
