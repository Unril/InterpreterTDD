#pragma once;
#include <vector>

namespace Interpreter {

enum class Operator : wchar_t {
    Plus = L'+',
};

typedef Operator Token;

inline std::wstring ToString(const Token &token) {
    return{ static_cast<wchar_t>(token) };
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