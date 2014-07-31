#pragma once;
#include <vector>
#include <assert.h>

namespace Interpreter {

enum class Operator : wchar_t {
    Plus = L'+',
};

inline std::wstring ToString(const Operator &op) {
    return{ static_cast<wchar_t>(op) };
}

enum class TokenType {
    Operator,
    Number
};

inline std::wstring ToString(const TokenType &type) {
    switch(type) {
        case TokenType::Operator:
            return L"Operator";
        case TokenType::Number:
            return L"Number";
        default:
            return L"Unknown token type";
    }
}

class Token {
public:
    Token(Operator op) :m_type(TokenType::Operator), m_operator(op) {}

    Token(double num) :m_type(TokenType::Number), m_number(num) {}

    TokenType Type() const {
        return m_type;
    }

    operator Operator() const {
        if(m_type != TokenType::Operator) {
            throw std::logic_error("Should be operator token.");
        }
        return m_operator;
    }

    operator double() const {
        if(m_type != TokenType::Number) {
            throw std::logic_error("Should be number token.");
        }
        return m_number;
    }

private:
    TokenType m_type;
    union {
        Operator m_operator;
        double m_number;
    };
};

inline std::wstring ToString(const Token &token) {
    switch(token.Type()) {
        case TokenType::Number:
            return std::to_wstring(static_cast<double>(token));
        case TokenType::Operator:
            return ToString(static_cast<Operator>(token));
        default:
            "Unknown token.";
    }
}

typedef std::vector<Token> Tokens;

namespace Lexer {

inline Tokens Tokenize(std::wstring expr) {
    if(expr.empty()) {
        return{};
    }
    return{ static_cast<Operator>(expr[0]) };
}

} // namespace Lexer
} // namespace Interpreter