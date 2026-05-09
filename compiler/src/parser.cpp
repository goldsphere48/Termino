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
    ProgramNode program = { };
    
    while(m_pos < m_tokens.size())
    {
        if (auto section = parseCodeSection())
        {
            program.nodes.push_back(std::move(section));
        }
        else if (auto section = parseDataSection())
        {
            program.nodes.push_back(std::move(section));            
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
            m_errorCollector.reportUnrecognizedToken(peek());
            advance();
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

const Token* Parser::expect(TOKEN_TYPE type)
{
    const Token& token = peek();
    if (token.type != type)
    {
        m_errorCollector.reportUnexpectedToken(type, token);
        return nullptr;
    }

    advance();
    return &token;
}

const Token* Parser::expect(DIRECTIVE type)
{
    const Token& token = peek();
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

    advance();
    return &token;
}

const Token* Parser::expect(OPERATOR type)
{
    const Token& token = peek();
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

    advance();
    return &token;    
}

void Parser::recoverLast()
{
    assert(m_pos - 1 >= 0);
    m_pos--;
}

void Parser::recoverTo(size_t pos)
{
    assert(m_pos - pos >= 0);
    m_pos = pos;
}

bool Parser::atSectionBoundary() const
{
    const Token& token = peek();

    if (token.type == TOKEN_TYPE::END_OF_FILE)
    {
        return true;
    }
    
    if (token.type != TOKEN_TYPE::DIRECTIVE)
    {
        return false;
    }

    const DIRECTIVE directive = token.getDirective();
    
    return
        directive == DIRECTIVE::CODE ||
        directive == DIRECTIVE::DATA ||
        directive == DIRECTIVE::EQU;
}
    
void Parser::syncToInstructionStart()
{
    while(peek().type != TOKEN_TYPE::END_OF_FILE)
    {
        TOKEN_TYPE type = peek().type;
        if (type == TOKEN_TYPE::INSTRUCTION) return;
        if (type == TOKEN_TYPE::LABEL_DEF) return;
        if (atSectionBoundary()) return;
        advance();
    }
}

void Parser::syncToDataNodeStart()
{
    while(peek().type != TOKEN_TYPE::END_OF_FILE)
    {
        TOKEN_TYPE type = peek().type;
        if (type == TOKEN_TYPE::LABEL_DEF) return;
        if (atSectionBoundary()) return;
        advance();
    }
}

void Parser::syncToEquItemStart()
{
    while(peek().type != TOKEN_TYPE::END_OF_FILE)
    {
        TOKEN_TYPE type = peek().type;
        if (type == TOKEN_TYPE::LABEL_DEF) return;
        if (atSectionBoundary()) return;
        advance();
    }
}

std::unique_ptr<ExpressionNode> Parser::parseUnmodifiedTerm()
{
    std::unique_ptr<ExpressionNode> node;
    const Token& token = peek();
    if (match(TOKEN_TYPE::INT) || match(TOKEN_TYPE::FLOAT))
    {
        std::unique_ptr<NumberNode> number = std::make_unique<NumberNode>();
        number->value = token.type == TOKEN_TYPE::INT ? token.getInt() : token.getFloat();
        node = std::move(number);
        node->line = token.line;
        node->column = token.column;
        return node;
    }
    else if (match(TOKEN_TYPE::IDENTIFIER))
    {
        std::unique_ptr<IdentifierNode> identifier = std::make_unique<IdentifierNode>();
        identifier->identifier = token.getString();
        node = std::move(identifier);
        node->line = token.line;
        node->column = token.column;
        return node;
    }

    return nullptr;
}

std::unique_ptr<ExpressionNode> Parser::parseTerm()
{
    std::unique_ptr<ExpressionNode> node;
    const Token& token = peek();

    if (auto term = parseUnmodifiedTerm())
    {
        node = std::move(term);
        return node;
    }
    else if (match(OPERATOR::MINUS))
    {
        if (auto term = parseUnmodifiedTerm())
        {
            node = std::move(term);
            node->unarOperator = OPERATOR::MINUS;
            return node;
        }
        else
        {
            m_errorCollector.report(ErrorType::INVALID_EXPRESSION, token.line, token.column, { Stringify::tokenValue(token) });
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
    if (match(OPERATOR::PLUS) || match(OPERATOR::MINUS))
    {
        if (auto right = parseTerm(); right != nullptr)
        {
            std::unique_ptr<BinaryOperationNode> binOp = std::make_unique<BinaryOperationNode>();
            binOp->oper = token.getOperator();
            binOp->left = std::move(left);
            binOp->right = std::move(right);
        
            binOp->line = token.line;
            binOp->column = token.column;
            
            return binOp;
        }
        else
        {
            m_errorCollector.report(ErrorType::INVALID_EXPRESSION, token.line, token.column);
            return left;
        }
    }

    return left;
}

std::vector<std::unique_ptr<ExpressionNode>> Parser::parseExpressionsList()
{
    std::vector<std::unique_ptr<ExpressionNode>> expressions;
    bool waitForExpression = false;
    while (true)
    {
        if (auto expression = parseExpression())
        {
            expressions.push_back(std::move(expression));
            waitForExpression = false;
        }
        else
        {
            const Token& token = peek();
            
            if (waitForExpression)
            {
                m_errorCollector.report(ErrorType::WAIT_EXPRESSION, token.line, token.column, { Stringify::tokenValue(token) });
            }


            if (!match(OPERATOR::COMMA))
            {
                break;
            }
            
            waitForExpression = true;
        }
    }
    return expressions;
}

std::unique_ptr<DataNode> Parser::parseDataNode()
{
    const Token& label = peek();
    if (match(TOKEN_TYPE::LABEL_DEF))
    {
        std::unique_ptr<DataNode> node = std::make_unique<DataNode>();
        const Token& typeToken = peek();
        node->line = typeToken.line;
        node->column = typeToken.column;
        
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
            node->expressions = parseExpressionsList();
            
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

        node->label = label.getString();
        return node;
    }
    else
    {
        return nullptr;
    }
}

std::unique_ptr<DataSectionNode> Parser::parseDataSection()
{
    const Token& token = peek();
    std::unique_ptr<DataSectionNode> section = std::make_unique<DataSectionNode>();
    if (match(DIRECTIVE::DATA))
    {
        section->line = token.line;
        section->column = token.column;
        
        while (!atSectionBoundary())
        {
            size_t before = m_pos;
            if (auto dataNode = parseDataNode())
            {
                section->datas.push_back(std::move(dataNode));       
            }
            else
            {
                if (m_pos == before)
                {
                    m_errorCollector.reportUnrecognizedToken(peek());
                }
                syncToDataNodeStart();
            }
        }
    }
    else
    {
        return nullptr;
    }
    
    return section;    
}

std::unique_ptr<LabelDefNode> Parser::parseLabelDef()
{
    const Token& token = peek();
    if (match(TOKEN_TYPE::LABEL_DEF))
    {
        std::unique_ptr<LabelDefNode> labelNode = std::make_unique<LabelDefNode>();
        labelNode->identifier = token.getString();
        labelNode->line = token.line;
        labelNode->column = token.column;
        return labelNode;
    }

    return nullptr;
}

std::unique_ptr<InstructionNode> Parser::parseInstruction()
{
    const Token& token = peek();
    if (match(TOKEN_TYPE::INSTRUCTION))
    {
        std::unique_ptr<InstructionNode> node = std::make_unique<InstructionNode>();
        node->command = token.getOpCode();
        node->line = token.line;
        node->column = token.column;
        
        node->operands = parseExpressionsList();
        
        return node;
    }

    return nullptr;
}

std::unique_ptr<CodeSectionNode> Parser::parseCodeSection()
{
    const Token& token = peek();
    if (match(DIRECTIVE::CODE))
    {
        std::unique_ptr<CodeSectionNode> codeSection = std::make_unique<CodeSectionNode>();
        codeSection->line = token.line;
        codeSection->column = token.column;
        
        while (!atSectionBoundary())
        {
            size_t before = m_pos;
            if (auto labelDef = parseLabelDef())
            {
                codeSection->nodes.push_back(std::move(labelDef));
            }
            else if (auto instruction = parseInstruction())
            {
                codeSection->nodes.push_back(std::move(instruction));
            }
            else
            {
                if (m_pos == before)
                {
                    m_errorCollector.reportUnrecognizedToken(peek());
                }
                syncToInstructionStart();
            }
        }

        return codeSection;
    }
    
    return nullptr;
}

std::unique_ptr<EquItemNode> Parser::parseEquItem()
{
    const Token& identifier = peek();
    if (match(TOKEN_TYPE::LABEL_DEF))
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
    const Token& token = peek();
    if (match(DIRECTIVE::EQU))
    {
        std::unique_ptr<EquSectionNode> equSection = std::make_unique<EquSectionNode>();
        equSection->line = token.line;
        equSection->column = token.column;
        
        while (!atSectionBoundary())
        {
            size_t before = m_pos;
            if (auto item = parseEquItem())
            {
                equSection->nodes.push_back(std::move(item));
            }
            else
            {
                if (m_pos == before)
                {
                    m_errorCollector.reportUnrecognizedToken(peek());
                }
                syncToEquItemStart();
            }
        }
        
        return equSection;
    }

    return nullptr;
}
