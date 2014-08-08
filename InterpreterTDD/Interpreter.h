#pragma once;
#include <vector>
#include <wchar.h>
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <array>

namespace Interpreter {

enum class Operator {
    Plus, Minus, Mul, Div, LParen, RParen, UPlus, UMinus,
};

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

struct TokenVisitor {
    virtual void VisitNumber(double) = 0;
    virtual void VisitOperator(Operator) = 0;

protected:
    ~TokenVisitor() {}
};

class Token {
    struct TokenConcept {
        virtual ~TokenConcept() {}

        virtual void Accept(TokenVisitor &) const = 0;

        virtual std::wstring ToString() const = 0;

        virtual bool Equals(const TokenConcept &other) const = 0;

        virtual bool EqualsToNumber(double) const {
            return false;
        }

        virtual bool EqualsToOperator(Operator) const {
            return false;
        }
    };

    struct NumberToken : TokenConcept {
        NumberToken(double val) : m_number(val) {}

        void Accept(TokenVisitor &visitor) const override {
            visitor.VisitNumber(m_number);
        }

        std::wstring ToString() const override {
            return std::to_wstring(m_number);
        }

        bool EqualsToNumber(double value) const override {
            return value == m_number;
        }

        bool Equals(const TokenConcept &other) const override {
            return other.EqualsToNumber(m_number);
        }

    private:
        double m_number;
    };

    struct OperatorToken : TokenConcept {
        OperatorToken(Operator val) : m_operator(val) {}

        void Accept(TokenVisitor &visitor) const override {
            visitor.VisitOperator(m_operator);
        }

        std::wstring ToString() const override {
            return Interpreter::ToString(m_operator);
        }

        bool EqualsToOperator(Operator value) const override {
            return value == m_operator;
        }

        bool Equals(const TokenConcept &other) const override {
            return other.EqualsToOperator(m_operator);
        }

    private:
        Operator m_operator;
    };

public:
    explicit Token(Operator val) : m_concept(std::make_shared<OperatorToken>(val)) {}

    explicit Token(double val) : m_concept(std::make_shared<NumberToken>(val)) {}

    void Accept(TokenVisitor &visitor) const {
        m_concept->Accept(visitor);
    }

    friend inline bool operator==(const Token &left, const Token &right) {
        return left.m_concept->Equals(*right.m_concept);
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
    Tokenizer(const std::wstring &expr) : m_current(expr.c_str()) {}

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
    void VisitNumber(double number) override {
        m_result.emplace_back(number);
        m_nextCanBeUnary = false;
    }

    void VisitOperator(Operator current) override {
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
inline Tokens Tokenize(const std::wstring &expr) {
    Detail::Tokenizer tokenizer(expr);
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
    if(token == Token(Operator::UMinus)) return 2;
    if(token == Token(Operator::Mul) || token == Token(Operator::Div)) return 1;
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
    void VisitOperator(Operator op) override {
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

    void VisitNumber(double number) override {
        m_output.emplace_back(number);
    }

    bool StackHasNoOperators() const {
        if(m_stack.back() == Token(Operator::LParen)) throw std::logic_error("Closing paren not found.");
        return false;
    }

    void PushCurrentToStack(Operator op) {
        m_stack.emplace_back(op);
    }

    void PopLeftParen() {
        if(m_stack.empty() || m_stack.back() != Token(Operator::LParen)) throw std::logic_error("Opening paren not found.");
        m_stack.pop_back();
    }

    bool OperatorWithLessPrecedenceOnTop(Operator op) const {
        return PrecedenceOf(m_stack.back()) < PrecedenceOf(Token(op));
    }

    bool LeftParenOnTop() const {
        return m_stack.back() == Token(Operator::LParen);
    }

    template <class T> void PopToOutputUntil(T whenToEnd) {
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

struct EvalFunction {
    virtual ~EvalFunction() {};
    virtual void Eval(std::vector<double> &) const = 0;
};

typedef std::unique_ptr<EvalFunction> FunctionPtr;

template<class T, int Size>
struct EvalFunctionImpl : public EvalFunction {
    EvalFunctionImpl(T function) : m_function(function) {}

    void Eval(std::vector<double> &stack) const override {
        if(stack.size() < Size) throw std::logic_error("Not enough operands in stack.");
        std::array<double, Size> operands;
        std::copy(stack.cend() - Size, stack.cend(), operands.begin());
        stack.erase(stack.cend() - Size, stack.cend());
        stack.push_back(m_function(operands));
    }

private:
    T m_function;
};

template<int Size, class T> FunctionPtr MakeFunction(T function) {
    return FunctionPtr(new EvalFunctionImpl<T, Size>(function));
}

class StackEvaluator : TokenVisitor {
public:
    void Evaluate(const Tokens &tokens) {
        for(const Token &token : tokens) token.Accept(*this);
    }

    double Result() const {
        return m_stack.empty() ? 0 : m_stack.back();
    }

private:
    void VisitOperator(Operator op) override {
        const static auto fmap = FunctionsMap();
        fmap.at(op)->Eval(m_stack);
    }

    void VisitNumber(double number) override {
        m_stack.push_back(number);
    }

    static std::map<Operator, FunctionPtr> FunctionsMap() {
        std::map<Operator, FunctionPtr> fmap;
        fmap.emplace(Operator::Plus, MakeFunction<2>([](auto args) {return args[0] + args[1]; }));
        fmap.emplace(Operator::Minus, MakeFunction<2>([](auto args) {return args[0] - args[1]; }));
        fmap.emplace(Operator::Mul, MakeFunction<2>([](auto args) {return args[0] * args[1]; }));
        fmap.emplace(Operator::Div, MakeFunction<2>([](auto args) {return args[0] / args[1]; }));
        fmap.emplace(Operator::UMinus, MakeFunction<1>([](auto args) {return -args[0]; }));
        return fmap;
    }

    std::vector<double> m_stack;
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