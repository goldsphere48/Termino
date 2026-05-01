#include "parser.h"
#include "isa_helper.h"

#include <cassert>

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

std::unique_ptr<ExpressionNode> Parser::parseExpression()
{
    
    return nullptr;
}

std::unique_ptr<DataNode> Parser::parseDataNode()
{
    if (const auto label = expect(TOKEN_TYPE::LABEL_DEF))
    {
        std::unique_ptr<DataNode> node = std::make_unique<DataNode>();
                
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
        else if (match(DIRECTIVE::BYTES))
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
                    ErrorType::EXPECTED_BYTE_LITERAL,
                    token.line,
                    token.column,
                    { std::string(Stringify::tokenValue(token)) }
                );
                return nullptr;
            }
            
            node->dataDirective = DIRECTIVE::BYTES;
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

std::unique_ptr<CodeSectionNode> Parser::parseCodeSection()
{
    return nullptr;
}

std::unique_ptr<EquSectionNode> Parser::parseEquSection()
{
    return nullptr;
}
