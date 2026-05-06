#include "parser.h"
#include "isa_helper.h"

#include <cassert>
#include <unordered_map>

Parser::Parser(const std::vector<Token>& tokens, ErrorCollector& collector) noexcept
    : m_errorCollector(collector), m_tokens(tokens), m_pos(0)
{
}


ProgramNode Parser::parse()
{
    ProgramNode program;
    
    while(m_pos < m_tokens.size())
    {
        if (auto section = parseCodeSection())
        {
            program.codeNodes.push_back(std::move(section));
        }
        else if (auto section = parseDataSection())
        {
            program.dataNodes.push_back(std::move(section));            
        }
        else if (auto section = parseEquSection())
        {
            for (auto& node : section->nodes)
            {
                if (program.equSection.find(node->identifier) == program.equSection.end())
                {
                    program.equSection[node->identifier] = std::move(node);   
                }
                else
                {
                    
                    m_errorCollector.report(ErrorType::EQU_ITEM_DUPLICATION, node->line, node->column, { node->identifier });
                }
            }
        }
        else if (peek().type == TOKEN_TYPE::END_OF_FILE)
        {
            break;
        }
        else
        {
            const Token& token = advance();
            m_errorCollector.reportUnrecognizedToken(token);
            return program;
        }
    }
    
    return program;
}


const Token& Parser::peek() const
{
    assert(m_pos < m_tokens.size());
    return m_tokens[m_pos];
}

const Token& Parser::advance()
{
    const Token& token = peek();
    if (token.type != TOKEN_TYPE::END_OF_FILE) {
        m_pos++;
    }
    return token;    
}

bool Parser::check(TOKEN_TYPE type) const
{
    return peek().type == type;
}

bool Parser::check(DIRECTIVE type) const
{
    return peek().hasDirective() && peek().getDirective() == type;
}

bool Parser::check(OPERATOR type) const
{
    return peek().hasOperator() && peek().getOperator() == type;    
}

bool Parser::match(TOKEN_TYPE type)
{
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(DIRECTIVE type)
{
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(OPERATOR type)
{
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token* Parser::expect(TOKEN_TYPE type)
{
    const Token& token = advance();
    if (token.type != type)
    {
        m_errorCollector.reportUnexpectedToken(type, token);
        return nullptr;
    }

    return &token;
}

const Token* Parser::expect(DIRECTIVE type)
{
    const Token& token = advance();
    if (token.type != TOKEN_TYPE::DIRECTIVE || !token.hasDirective())
    {
        m_errorCollector.reportUnexpectedToken(TOKEN_TYPE::DIRECTIVE, token);
        return nullptr;
    }
    else if (DIRECTIVE directive = token.getDirective(); directive != type)
    {
        m_errorCollector.reportUnexpectedDirective(type, directive, token);
        return nullptr;        
    }

    return &token;
}

const Token* Parser::expect(OPERATOR type)
{
    const Token& token = advance();
    if (token.type != TOKEN_TYPE::OPERATOR || !token.hasOperator())
    {
        m_errorCollector.reportUnexpectedToken(TOKEN_TYPE::OPERATOR, token);
        return nullptr;
    }
    else if (OPERATOR oper = token.getOperator(); oper != type)
    {
        m_errorCollector.reportUnexpectedOperator(type, oper, token);
        return nullptr;
    }
    
    return &token;    
}

std::unique_ptr<ExpressionNode> Parser::parseTerm()
{
    std::unique_ptr<ExpressionNode> node;

    const Token& token = peek();
    if (match(TOKEN_TYPE::IDENTIFIER))
    {
        std::unique_ptr<IdentifierNode> identifier = std::make_unique<IdentifierNode>();
        identifier->identifier = token.getString();
        node = std::move(identifier);
        return node;
    }
    else if (match(TOKEN_TYPE::INT) || match(TOKEN_TYPE::FLOAT))
    {
        std::unique_ptr<NumberNode> number = std::make_unique<NumberNode>();
        number->value = token.type == TOKEN_TYPE::INT ? token.getInt() : token.getFloat();
        node = std::move(number);
        return node;
    }
    else if (match(OPERATOR::MINUS))
    {
        const Token& numberToken = peek();
        if (match(TOKEN_TYPE::INT) || match(TOKEN_TYPE::FLOAT))
        {
            std::unique_ptr<NumberNode> number = std::make_unique<NumberNode>();
            number->value = numberToken.type == TOKEN_TYPE::INT ? -numberToken.getInt() : -numberToken.getFloat();
            node = std::move(number);

            return node;
        }
    }

    return nullptr;
}

std::unique_ptr<ExpressionNode> Parser::parseExpression()
{
    std::unique_ptr<ExpressionNode> left;
    if (auto node = parseTerm())
    {
        left = std::move(node);
    }
    else
    {
        return nullptr;
    }

    const Token& token = peek();
    std::unique_ptr<BinaryOperationNode> binOp = std::make_unique<BinaryOperationNode>();
    if (match(OPERATOR::PLUS) || match(OPERATOR::MINUS))
    {
        binOp->oper = token.getOperator();
        binOp->left = std::move(left);
    }
    else
    {
        return left;
    }

    if (auto right = parseTerm(); right != nullptr)
    {
        binOp->right = std::move(right);
        return binOp;
    }
    else
    {
        m_errorCollector.report(ErrorType::INVALID_EXPRESSION, token.line, token.column);   
    }
   
    return nullptr;
}

std::unique_ptr<DataNode> Parser::parseDataNode()
{
    if (const auto label = expect(TOKEN_TYPE::LABEL_DEF))
    {
        std::unique_ptr<DataNode> node = std::make_unique<DataNode>();
        const Token& typeToken = peek();
                
        if (match(DIRECTIVE::STRING))
        {
            if (const Token* strToken = expect(TOKEN_TYPE::STRING))
            {
                node->stringValue = strToken->getString();
                node->dataDirective = DIRECTIVE::STRING;
            }
            else
            {
                return nullptr;
            }
        }
        else if (match(DIRECTIVE::BYTE) || match(DIRECTIVE::WORD) || match(DIRECTIVE::DWORD) || match(DIRECTIVE::SHORT))
        {            
            while (true)
            {
                if (auto expression = parseExpression())
                {
                    node->expressions.push_back(std::move(expression));
                }
                else
                {
                    break;
                }
            }

            if (node->expressions.empty())
            {
                const Token& token = peek();
                m_errorCollector.report(
                    ErrorType::INVALID_EXPRESSION,
                    token.line,
                    token.column
                );
                return nullptr;
            }
            
            node->dataDirective = typeToken.getDirective();
        }
        else
        {
            const Token& token = peek();
            const std::string got = std::string(Stringify::tokenValue(token));
            
            m_errorCollector.report(
                ErrorType::EXPECTED_DATA_DIRECTIVE,
                token.line,
                token.column,
                { got }
            );

            return nullptr;
        }

        node->label = label->getString();
        return node;
    }
    else
    {
        return nullptr;
    }
}

std::unique_ptr<DataSectionNode> Parser::parseDataSection()
{
    std::unique_ptr<DataSectionNode> section = std::make_unique<DataSectionNode>();
    if (match(DIRECTIVE::DATA))
    {
        while (true)
        {
            if (check(TOKEN_TYPE::LABEL_DEF))
            {
                if (auto dataNode = parseDataNode())
                {
                    section->datas.push_back(std::move(dataNode));       
                }
                else
                {
                    return nullptr;
                }
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        return nullptr;
    }
    
    return section;    
}

std::unique_ptr<InstructionNode> Parser::parseInstruction()
{
    const Token& token = peek();
    if (match(TOKEN_TYPE::INSTRUCTION))
    {
        std::unique_ptr<InstructionNode> node = std::make_unique<InstructionNode>();
        node->command = token.getOpCode();
        while (true)
        {
            if (auto expression = parseExpression())
            {
                node->operands.push_back(std::move(expression));
            }
            else
            {
                break;
            }
        }

        return node;
    }
    return nullptr;
}

std::unique_ptr<CodeSectionNode> Parser::parseCodeSection()
{
    if (match(DIRECTIVE::CODE))
    {
        std::unique_ptr<CodeSectionNode> codeSection = std::make_unique<CodeSectionNode>();
        while (true)
        {
            if (auto instruction = parseInstruction())
            {
                codeSection->nodes.push_back(std::move(instruction));
            }
            else
            {
                break;
            }
        }

        return codeSection;
    }
    
    return nullptr;
}

std::unique_ptr<EquItemNode> Parser::parseEquItem()
{
    const Token& identifier = peek();
    if (match(TOKEN_TYPE::IDENTIFIER))
    {
        const Token& token = peek();
        if (auto expression = parseExpression())
        {
            auto node = std::make_unique<EquItemNode>();
            node->identifier = identifier.getString();
            node->expression = std::move(expression);
            node->line = token.line;
            node->column = token.column;
            return node;
        }
        else
        {
            m_errorCollector.report(ErrorType::INVALID_EXPRESSION, token.line, token.column);
        }
    }

    return nullptr;
}

std::unique_ptr<EquSectionNode> Parser::parseEquSection()
{
    if (match(DIRECTIVE::EQU))
    {
        std::unique_ptr<EquSectionNode> equSection = std::make_unique<EquSectionNode>();
        while (true)
        {
            if (auto item = parseEquItem())
            {
                equSection->nodes.push_back(std::move(item));
            }
            else
            {
                break;
            }
        }
        
        return equSection;
    }

    return nullptr;
}
