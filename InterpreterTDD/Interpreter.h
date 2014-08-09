#pragma once;
#include <vector>
#include <wchar.h>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <memory>
#include <array>

namespace Interpreter {

enum class Operator {
    Plus, Minus, Mul, Div, LParen, RParen, UPlus, UMinus,
};

typedef double Number;

inline std::wstring ToString(const Operator &op) {
    switch(op) {
        case Operator::Plus: return L"+";
        case Operator::Minus: return L"-";
        case Operator::Mul: return L"*";
        case Operator::Div: return L"/";
        case Operator::LParen: return L"(";
        case Operator::RParen: return L")";
        case Operator::UPlus: return L"Unary +";
        case Operator::UMinus: return L"Unary -";
        default: throw std::out_of_range("Operator.");
    }
}

inline std::wstring ToString(const Number &num) {
    return std::to_wstring(num);
}

struct TokenVisitor {
    virtual ~TokenVisitor() {}
    virtual void Visit(Number) = 0;
    virtual void Visit(Operator) = 0;
};

class Token {
    struct TokenConcept {
        virtual ~TokenConcept() {}

        virtual void Accept(TokenVisitor &) const = 0;

        virtual std::wstring ToString() const = 0;

        virtual bool DispatchEquals(const TokenConcept &) const = 0;

        virtual bool EqualsTo(Number) const {
            return false;
        }

        virtual bool EqualsTo(Operator) const {
            return false;
        }
    };

    template<typename T>
    struct GenericToken : TokenConcept {
        GenericToken(T payload) : m_payload(std::move(payload)) {}

        void Accept(TokenVisitor &visitor) const override {
            visitor.Visit(m_payload);
        }

        std::wstring ToString() const override {
            return Interpreter::ToString(m_payload);
        }

        bool EqualsTo(T value) const override {
            return value == m_payload;
        }

        bool DispatchEquals(const TokenConcept &other) const override {
            return other.EqualsTo(m_payload);
        }

        T m_payload;
    };

public:
    explicit Token(Operator value) : m_concept(std::make_shared<GenericToken<Operator>>(value)) {}

    explicit Token(Number value) : m_concept(std::make_shared<GenericToken<Number>>(value)) {}

    void Accept(TokenVisitor &visitor) const {
        m_concept->Accept(visitor);
    }

    bool EqualsTo(Operator value) const {
        return m_concept->EqualsTo(value);
    }

    bool EqualsTo(Number value) const {
        return m_concept->EqualsTo(value);
    }

    friend inline bool operator==(const Token &left, const Token &right) {
        return left.m_concept->DispatchEquals(*right.m_concept);
    }

    friend inline bool operator!=(const Token &left, const Token &right) {
        return !(left == right);
    }

    friend inline std::wstring ToString(const Token &token) {
        return token.m_concept->ToString();
    }

private:
    std::shared_ptr<const TokenConcept> m_concept;
};

typedef std::vector<Token> Tokens;

namespace Lexer {

namespace Detail {

class Tokenizer {
public:
    Tokenizer(const std::wstring &expression) : m_current(expression.c_str()) {}

    void Tokenize() {
        while(!EndOfExperssion()) {
            if(IsNumber()) {
                ScanNumber();
            }
            else if(IsOperator()) {
                ScanOperator();
            }
            else {
                MoveNext();
            }
        }
    }

    const Tokens &Result() const {
        return m_result;
    }

private:
    bool EndOfExperssion() const {
        return *m_current == L'\0';
    }

    bool IsNumber() const {
        return iswdigit(*m_current) != 0;
    }

    void ScanNumber() {
        wchar_t *end = nullptr;
        m_result.emplace_back(wcstod(m_current, &end));
        m_current = end;
    }

    bool IsOperator() const {
        static const std::wstring all(L"+-*/()");
        return all.find(*m_current) != std::wstring::npos;
    }

    void ScanOperator() {
        switch(*m_current) {
            case L'+': m_result.emplace_back(Operator::Plus); break;
            case L'-': m_result.emplace_back(Operator::Minus); break;
            case L'*': m_result.emplace_back(Operator::Mul); break;
            case L'/': m_result.emplace_back(Operator::Div); break;
            case L'(': m_result.emplace_back(Operator::LParen); break;
            case L')': m_result.emplace_back(Operator::RParen); break;
            default: break;
        }
        MoveNext();
    }

    void MoveNext() {
        ++m_current;
    }

    const wchar_t *m_current;
    Tokens m_result;
};

class UnaryOperatorMarker : TokenVisitor {
public:
    void Mark(const Tokens &tokens) {
        for(const Token &token : tokens) token.Accept(*this);
    }

    const Tokens &Result() const {
        return m_result;
    }

private:
    void Visit(Number num) override {
        m_result.emplace_back(num);
        m_nextCanBeUnary = false;
    }

    void Visit(Operator current) override {
        m_result.emplace_back(m_nextCanBeUnary ? TryConvertToUnary(current) : current);
        m_nextCanBeUnary = (current != Operator::RParen);
    }

    static Operator TryConvertToUnary(Operator op) {
        switch(op) {
            case Operator::Plus: return Operator::UPlus;
            case Operator::Minus: return Operator::UMinus;
            default: return op;
        }
    }

    bool m_nextCanBeUnary = true;
    Tokens m_result;
};

} // namespace Detail

/// <summary>
/// Convert the expression string to a sequence of tokens.
/// </summary>
inline Tokens Tokenize(const std::wstring &expression) {
    Detail::Tokenizer tokenizer(expression);
    tokenizer.Tokenize();
    return tokenizer.Result();
}

/// <summary>
/// Change binary pluses and minuses that are unary to unary operator tokens.
/// </summary>
inline Tokens MarkUnaryOperators(const Tokens &tokens) {
    Detail::UnaryOperatorMarker marker;
    marker.Mark(tokens);
    return marker.Result();
}

} // namespace Lexer

namespace Parser {

inline int PrecedenceOf(const Token &token) {
    if(token.EqualsTo(Operator::UMinus)) return 2;
    if(token.EqualsTo(Operator::Mul) || token.EqualsTo(Operator::Div)) return 1;
    return 0;
}

namespace Detail {

class ShuntingYardParser : TokenVisitor {
public:
    void Parse(const Tokens &tokens) {
        for(const Token &token : tokens) token.Accept(*this);
        PopToOutputUntil([this]() { return StackHasNoOperators(); });
    }

    const Tokens &Result() const {
        return m_output;
    }

private:
    void Visit(Operator op) override {
        switch(op) {
            case Operator::UPlus:
                break;
            case Operator::UMinus:
                PushCurrentToStack(op);
                break;
            case Operator::LParen:
                PushCurrentToStack(op);
                break;
            case Operator::RParen:
                PopToOutputUntil([this]() { return LeftParenOnTop(); });
                PopLeftParen();
                break;
            default:
                PopToOutputUntil([&]() { return LeftParenOnTop() || OperatorWithLessPrecedenceOnTop(op); });
                PushCurrentToStack(op);
                break;
        }
    }

    void Visit(Number num) override {
        m_output.emplace_back(num);
    }

    bool StackHasNoOperators() const {
        if(m_stack.back().EqualsTo(Operator::LParen)) throw std::logic_error("Closing paren not found.");
        return false;
    }

    void PushCurrentToStack(Operator op) {
        m_stack.emplace_back(op);
    }

    void PopLeftParen() {
        if(m_stack.empty() || !m_stack.back().EqualsTo(Operator::LParen)) {
            throw std::logic_error("Opening paren not found.");
        }
        m_stack.pop_back();
    }

    bool OperatorWithLessPrecedenceOnTop(Operator op) const {
        return PrecedenceOf(m_stack.back()) < PrecedenceOf(Token(op));
    }

    bool LeftParenOnTop() const {
        return m_stack.back().EqualsTo(Operator::LParen);
    }

    template <typename T>
    void PopToOutputUntil(T whenToEnd) {
        while(!m_stack.empty() && !whenToEnd()) {
            m_output.push_back(m_stack.back());
            m_stack.pop_back();
        }
    }

    Tokens m_output, m_stack;
};

} // namespace Detail

/// <summary>
/// Convert the sequence of tokens in infix notation to a sequence in postfix notation.
/// </summary>
inline Tokens Parse(const Tokens &tokens) {
    Detail::ShuntingYardParser parser;
    parser.Parse(tokens);
    return parser.Result();
}

} // namespace Parser

namespace Evaluator {

namespace Detail {

class StackEvaluator : TokenVisitor {
    typedef std::vector<Number> Stack;
    typedef Stack::const_iterator Args;

public:
    void Evaluate(const Tokens &tokens) {
        for(const Token &token : tokens) token.Accept(*this);
    }

    Number Result() const {
        return m_stack.empty() ? 0 : m_stack.back();
    }

private:
    template<typename T>
    static auto MakeEvaluator(const size_t arity, T function) {
        return [=](Stack &stack) {
            if(stack.size() < arity) throw std::logic_error("Not enough operands in stack.");
            Args arguments = stack.cend() - arity;
            Number result = function(arguments);
            stack.erase(arguments, stack.cend());
            stack.push_back(result);
        };
    }

    void Visit(Operator op) override {
        const static std::unordered_map<Operator, std::function<void(Stack &)>> evaluators{
                { Operator::Plus, MakeEvaluator(2, [=](Args a) { return a[0] + a[1]; }) },
                { Operator::Minus, MakeEvaluator(2, [=](Args a) { return a[0] - a[1]; }) },
                { Operator::Mul, MakeEvaluator(2, [=](Args a) { return a[0] * a[1]; }) },
                { Operator::Div, MakeEvaluator(2, [=](Args a) { return a[0] / a[1]; }) },
                { Operator::UMinus, MakeEvaluator(1, [=](Args a) { return -a[0]; }) }
        };
        evaluators.at(op)(m_stack);
    }

    void Visit(Number num) override {
        m_stack.push_back(num);
    }

    Stack m_stack;
};

} // namespace Detail

/// <summary>
/// Evaluate the sequence of tokens in postfix notation and get a numerical result.
/// </summary>
inline double Evaluate(const Tokens &tokens) {
    Detail::StackEvaluator evaluator;
    evaluator.Evaluate(tokens);
    return evaluator.Result();
}

} // namespace Evaluator

/// <summary>
/// Interpret the mathematical expression in infix notation and return a numerical result.
/// </summary>
inline double InterpreteExperssion(const std::wstring &expression) {
    return Evaluator::Evaluate(Parser::Parse(Lexer::Tokenize(expression)));
}

} // namespace Interpreter