#pragma once;
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <memory>

namespace Interpreter {

enum class Operator {
    Plus, Minus, Mul, Div, LParen, RParen, UPlus, UMinus,
};

inline std::wstring ToString(const Operator &op) {
    static const std::unordered_map<Operator, std::wstring> opmap{
            { Operator::Plus, L"+" }, { Operator::Minus, L"-" },
            { Operator::Mul, L"*" }, { Operator::Div, L"/" },
            { Operator::LParen, L"(" }, { Operator::RParen, L")" },
            { Operator::UPlus, L"u+" }, { Operator::UMinus, L"u-" } };
    return opmap.at(op);
}

inline std::wstring ToString(const double &num) {
    return std::to_wstring(num);
}

struct TokenVisitor {
    template<typename Iter> void VisitAll(Iter first, Iter last) {
        std::for_each(first, last, [this](const auto &token) { token->Accept(*this); });
    }

    virtual ~TokenVisitor() {}
    virtual void Visit(double) = 0;
    virtual void Visit(Operator) = 0;
};

namespace Detail {

struct TokenConcept {
    virtual ~TokenConcept() {}
    virtual void Accept(TokenVisitor &) const = 0;
    virtual std::wstring ToString() const = 0;
    virtual bool DispatchEquals(const TokenConcept &) const = 0;

    virtual bool EqualsTo(double) const {
        return false;
    }

    virtual bool EqualsTo(Operator) const {
        return false;
    }
};

typedef std::shared_ptr<const TokenConcept> Token;

inline std::wstring ToString(const Token &token) {
    return token->ToString();
}

inline bool operator==(const Token &left, const Token &right) {
    return left->DispatchEquals(*right);
}

template<typename T> bool operator==(const Token &left, const T &right) {
    return left->EqualsTo(right);
}

template<typename T> struct GenericToken : TokenConcept {
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
} // namespace Detail

using Detail::Token;
typedef std::vector<Token> Tokens;

inline Token MakeToken(Operator value) {
    return std::make_shared<Detail::GenericToken<Operator>>(value);
}

inline Token MakeToken(double value) {
    return std::make_shared<Detail::GenericToken<double>>(value);
}

class WithTokensResult {
public:
    Tokens Result() {
        return std::move(m_result);
    }

protected:
    ~WithTokensResult() {}

    template<typename T> void AddToResult(T value) {
        m_result.push_back(MakeToken(value));
    }

    void AddToResult(const Token &value) {
        m_result.push_back(value);
    }

private:
    Tokens m_result;
};

namespace Lexer {
namespace Detail {

class Tokenizer : public WithTokensResult {
public:
    void Tokenize(const std::wstring &expression) {
        for(m_current = expression.c_str(); *m_current;) {
            if(IsNumber()) {
                ScanNumber();
            }
            else if(IsOperator()) {
                ScanOperator();
            }
            else {
                ++m_current;
            }
        }
    }

private:
    bool IsNumber() const {
        return iswdigit(*m_current) != 0;
    }

    void ScanNumber() {
        AddToResult(wcstod(m_current, const_cast<wchar_t **>(&m_current)));
    }

    const static auto &CharToOperatorMap() {
        static const std::unordered_map<wchar_t, Operator> opmap{
                { L'+', Operator::Plus }, { L'-', Operator::Minus },
                { L'*', Operator::Mul }, { L'/', Operator::Div },
                { L'(', Operator::LParen }, { L')', Operator::RParen },
        };
        return opmap;
    }

    bool IsOperator() const {
        return CharToOperatorMap().find(*m_current) != CharToOperatorMap().end();
    }

    void ScanOperator() {
        AddToResult(CharToOperatorMap().at(*m_current));
        ++m_current;
    }

    const wchar_t *m_current = nullptr;
};

class UnaryOperatorMarker : public TokenVisitor, public WithTokensResult {
private:
    void Visit(double num) override {
        AddToResult(num);
        m_nextCanBeUnary = false;
    }

    void Visit(Operator op) override {
        AddToResult(m_nextCanBeUnary ? TryConvertToUnary(op) : op);
        m_nextCanBeUnary = (op != Operator::RParen);
    }

    static Operator TryConvertToUnary(Operator op) {
        if(op == Operator::Plus) return Operator::UPlus;
        if(op == Operator::Minus) return Operator::UMinus;
        return op;
    }

    bool m_nextCanBeUnary = true;
};
} // namespace Detail

// Convert the expression string to a sequence of tokens.
inline Tokens Tokenize(const std::wstring &expression) {
    Detail::Tokenizer tokenizer;
    tokenizer.Tokenize(expression);
    return tokenizer.Result();
}

// Change binary pluses and minuses that are unary to unary operator tokens.
inline Tokens MarkUnaryOperators(const Tokens &tokens) {
    Detail::UnaryOperatorMarker marker;
    marker.VisitAll(tokens.cbegin(), tokens.cend());
    return marker.Result();
}
} // namespace Lexer

namespace Parser {

template<typename T> int PrecedenceOf(const T &token) {
    if(token == Operator::UMinus) return 2;
    if(token == Operator::Mul || token == Operator::Div) return 1;
    return 0;
}

namespace Detail {

class ShuntingYardParser : public TokenVisitor, private WithTokensResult {
public:
    Tokens Result() {
        PopToOutputUntil([this]() { return StackHasNoOperators(); });
        return WithTokensResult::Result();
    }

private:
    void Visit(Operator op) override {
        switch(op) {
            case Operator::UPlus:
                break;
            case Operator::UMinus:
            case Operator::LParen:
                PushCurrentToStack(op);
                break;
            case Operator::RParen:
                PopToOutputUntil([this]() { return LeftParenOnTop(); });
                PopLeftParen();
                break;
            default:
                PopToOutputUntil([this, op]() { return LeftParenOnTop() || OperatorWithLessPrecedenceOnTop(op); });
                PushCurrentToStack(op);
                break;
        }
    }

    void Visit(double num) override {
        AddToResult(num);
    }

    bool StackHasNoOperators() const {
        if(m_stack.back() == Operator::LParen) throw std::logic_error("Closing paren not found.");
        return false;
    }

    void PushCurrentToStack(Operator op) {
        m_stack.push_back(MakeToken(op));
    }

    void PopLeftParen() {
        if(m_stack.empty() || !LeftParenOnTop()) throw std::logic_error("Opening paren not found.");
        m_stack.pop_back();
    }

    bool OperatorWithLessPrecedenceOnTop(Operator op) const {
        return PrecedenceOf(m_stack.back()) < PrecedenceOf(op);
    }

    bool LeftParenOnTop() const {
        return m_stack.back() == Operator::LParen;
    }

    template <typename T>
    void PopToOutputUntil(T whenToEnd) {
        while(!m_stack.empty() && !whenToEnd()) {
            AddToResult(m_stack.back());
            m_stack.pop_back();
        }
    }

    Tokens m_stack;
};
} // namespace Detail

// Convert the sequence of tokens in infix notation to a sequence in postfix notation.
inline Tokens Parse(const Tokens &tokens) {
    Detail::ShuntingYardParser parser;
    parser.VisitAll(tokens.cbegin(), tokens.cend());
    return parser.Result();
}
} // namespace Parser

namespace Evaluator {
namespace Detail {

class StackEvaluator : public TokenVisitor {
public:
    double Result() const {
        return m_stack.empty() ? 0.0 : m_stack.back();
    }

private:
    typedef std::vector<double> OpStack;
    typedef OpStack::const_iterator Args;

    template<typename T> static auto MakeEvaluator(const size_t arity, T function) {
        return [=](OpStack &stack) {
            if(stack.size() < arity) throw std::logic_error("Not enough arguments in stack.");
            Args argumentsOnStack = stack.cend() - arity;
            double result = function(argumentsOnStack);
            stack.erase(argumentsOnStack, stack.cend());
            stack.push_back(result);
        };
    }

    void Visit(Operator op) override {
        const static std::unordered_map<Operator, std::function<void(OpStack &)>> evaluators{
                { Operator::Plus, MakeEvaluator(2, [=](Args a) { return a[0] + a[1]; }) },
                { Operator::Minus, MakeEvaluator(2, [=](Args a) { return a[0] - a[1]; }) },
                { Operator::Mul, MakeEvaluator(2, [=](Args a) { return a[0] * a[1]; }) },
                { Operator::Div, MakeEvaluator(2, [=](Args a) { return a[0] / a[1]; }) },
                { Operator::UMinus, MakeEvaluator(1, [=](Args a) { return -a[0]; }) }
        };
        evaluators.at(op)(m_stack);
    }

    void Visit(double num) override {
        m_stack.push_back(num);
    }

    OpStack m_stack;
};
} // namespace Detail

// Evaluate the sequence of tokens in postfix notation and get a numerical result.
inline double Evaluate(const Tokens &tokens) {
    Detail::StackEvaluator evaluator;
    evaluator.VisitAll(tokens.cbegin(), tokens.cend());
    return evaluator.Result();
}
} // namespace Evaluator

// Interpret the mathematical expression in infix notation and return a numerical result.
inline double InterpreteExperssion(const std::wstring &expression) {
    return Evaluator::Evaluate(Parser::Parse(Lexer::MarkUnaryOperators(Lexer::Tokenize(expression))));
}
} // namespace Interpreter