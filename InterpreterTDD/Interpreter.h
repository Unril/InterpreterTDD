#pragma once;
#include <vector>
#include <assert.h>
#include <wchar.h>

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

    friend inline bool operator==(const Token &left, const Token &right) {
        if(left.m_type == right.m_type) {
            switch(left.m_type) {
                case Interpreter::TokenType::Operator:
                    return left.m_operator == right.m_operator;
                case Interpreter::TokenType::Number:
                    return left.m_number == right.m_number;
                default:
                    throw std::out_of_range("TokenType");
            }
        }
        return false;
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
            throw std::out_of_range("TokenType");
    }
}

typedef std::vector<Token> Tokens;

namespace Lexer {

inline Tokens Tokenize(std::wstring expr) {
    Tokens result;
    const wchar_t *current = expr.c_str();
    while(*current) {
        if(iswdigit(*current)) {
            wchar_t *end = nullptr;
            result.push_back(wcstod(current, &end));
            current = end;
        }
        else if(*current == static_cast<wchar_t>(Operator::Plus)) {
            result.push_back(static_cast<Operator>(*current));
            ++current;
        }
        else {
            ++current;
        }
    }
    return result;
}

} // namespace Lexer
} // namespace Interpreter