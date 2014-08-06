#include "stdafx.h"
#include "CppUnitTest.h"
#include "Interpreter.h"

namespace InterpreterTests {

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Interpreter;
using namespace std;

namespace AssertRange {

template<class T, class ActualRange>
static void AreEqual(initializer_list<T> expect, const ActualRange &actual) {
    auto actualIter = begin(actual);
    auto expectIter = begin(expect);

    Assert::AreEqual(distance(expectIter, end(expect)), distance(actualIter, end(actual)), L"Size differs.");

    for(; expectIter != end(expect) && actualIter != end(actual); ++expectIter, ++actualIter) {
        auto message = L"Mismatch at position " + to_wstring(distance(begin(expect), expectIter));
        Assert::AreEqual<T>(*expectIter, *actualIter, message.c_str());
    }
}

} // namespace AssertRange

const Token plus(Operator::Plus), minus(Operator::Minus);
const Token mul(Operator::Mul), div(Operator::Div);
const Token pLeft(Operator::LParen), pRight(Operator::RParen);
const Token uPlus(Operator::UPlus), uMinus(Operator::UMinus);
const Token _1(1), _2(2), _3(3), _4(4), _5(5);

TEST_CLASS(LexerTests) {
public:
    TEST_METHOD(Should_return_empty_token_list_when_put_empty_expression) {
        Tokens tokens = Lexer::Tokenize(L"");
        Assert::IsTrue(tokens.empty());
    }

    TEST_METHOD(Should_tokenize_single_plus_operator) {
        Tokens tokens = Lexer::Tokenize(L"+");
        AssertRange::AreEqual({ plus }, tokens);
    }

    TEST_METHOD(Should_tokenize_single_digit) {
        Tokens tokens = Lexer::Tokenize(L"1");
        AssertRange::AreEqual({ _1 }, tokens);
    }

    TEST_METHOD(Should_tokenize_floating_point_number) {
        Tokens tokens = Lexer::Tokenize(L"12.34");
        AssertRange::AreEqual({ Token(12.34) }, tokens);
    }

    TEST_METHOD(Should_tokenize_plus_and_number) {
        Tokens tokens = Lexer::Tokenize(L"+12.34");
        AssertRange::AreEqual({ plus, Token(12.34) }, tokens);
    }

    TEST_METHOD(Should_skip_spaces) {
        Tokens tokens = Lexer::Tokenize(L" 1 +  12.34  ");
        AssertRange::AreEqual({ _1, plus, Token(12.34) }, tokens);
    }

    TEST_METHOD(Should_tokenize_complex_experssion) {
        Tokens tokens = Lexer::Tokenize(L"1+2*3/(4-5)");
        AssertRange::AreEqual({ _1, plus, _2, mul, _3, div, pLeft, _4, minus, _5, pRight }, tokens);
    }
};

TEST_CLASS(LexerMarkUnaryOperatorsTests) {
public:
    TEST_METHOD(Should_return_same_list_when_it_without_pluses_or_minuses) {
        auto tokens = { _1, mul, _2, div, _3 };
        Tokens result = Lexer::MarkUnaryOperators(tokens);
        AssertRange::AreEqual(tokens, result);
    }

    TEST_METHOD(Should_mark_unary_first_minus_in_sequence) {
        Tokens result = Lexer::MarkUnaryOperators({ minus, _1 });
        AssertRange::AreEqual({ uMinus, _1 }, result);
    }

    TEST_METHOD(Should_return_empty_list_when_get_empty_list) {
        Tokens result = Lexer::MarkUnaryOperators({});
        Assert::IsTrue(result.empty());
    }

    TEST_METHOD(Should_mark_unary_minus_after_another_minus) {
        Tokens result = Lexer::MarkUnaryOperators({ _1, minus, minus, _1 });
        AssertRange::AreEqual({ _1, minus, uMinus, _1 }, result);
    }

    TEST_METHOD(Should_mark_unary_minus_after_plus) {
        Tokens result = Lexer::MarkUnaryOperators({ _1, plus, minus, _1 });
        AssertRange::AreEqual({ _1, plus, uMinus, _1 }, result);
    }

    TEST_METHOD(Should_mark_unary_only_minus_after_left_paren) {
        Tokens result = Lexer::MarkUnaryOperators({ _1, minus, pLeft, minus, _1, pRight, minus, _1 });
        AssertRange::AreEqual({ _1, minus, pLeft, uMinus, _1, pRight, minus, _1 }, result);
    }

    TEST_METHOD(Should_mark_all_pluses_in_following_experssion_as_unary) {
        // +(+1-++1)-1
        Tokens result = Lexer::MarkUnaryOperators({ plus, pLeft, plus, _1, minus, plus, plus, _1, pRight, minus, _1 });
        AssertRange::AreEqual({ uPlus, pLeft, uPlus, _1, minus, uPlus, uPlus, _1, pRight, minus, _1 }, result);
    }
};

TEST_CLASS(TokenTests) {
public:
    TEST_METHOD(Should_check_for_equality_operator_tokens) {
        Assert::AreEqual(minus, minus);
        Assert::AreNotEqual(minus, plus);
        Assert::AreNotEqual(minus, _1);
    }

    TEST_METHOD(Should_check_for_equality_number_tokens) {
        Assert::AreEqual(_1, _1);
        Assert::AreNotEqual(_1, _2);
        Assert::AreNotEqual(_1, minus);
    }
};

TEST_CLASS(ParserTests) {
public:
    TEST_METHOD(Should_return_empty_list_when_put_empty_list) {
        Tokens tokens = Parser::Parse({});
        Assert::IsTrue(tokens.empty());
    }

    TEST_METHOD(Should_parse_single_number) {
        Tokens tokens = Parser::Parse({ _1 });
        AssertRange::AreEqual({ _1 }, tokens);
    }

    TEST_METHOD(Should_parse_num_plus_num) {
        Tokens tokens = Parser::Parse({ _1, plus, _2 });
        AssertRange::AreEqual({ _1, _2, plus }, tokens);
    }

    TEST_METHOD(Should_parse_two_additions) {
        Tokens tokens = Parser::Parse({ _1, plus, _2, plus, _3 });
        AssertRange::AreEqual({ _1, _2, plus, _3, plus }, tokens);
    }

    TEST_METHOD(Should_get_same_precedence_for_operator_pairs) {
        Assert::AreEqual(Parser::PrecedenceOf(plus), Parser::PrecedenceOf(minus));
        Assert::AreEqual(Parser::PrecedenceOf(mul), Parser::PrecedenceOf(div));
    }

    TEST_METHOD(Should_get_greater_precedence_for_multiplicative_operators) {
        Assert::IsTrue(Parser::PrecedenceOf(mul) > Parser::PrecedenceOf(plus));
    }

    TEST_METHOD(Should_parse_add_and_mul) {
        Tokens tokens = Parser::Parse({ _1, plus, _2, mul, _3 });
        AssertRange::AreEqual({ _1, _2, _3, mul, plus }, tokens);
    }

    TEST_METHOD(Should_parse_complex_experssion) {
        Tokens tokens = Parser::Parse({ _1, plus, _2, div, _3, minus, _4, mul, _5 });
        AssertRange::AreEqual({ _1, _2, _3, div, plus, _4, _5, mul, minus }, tokens);
    }

    TEST_METHOD(Should_skip_parens_around_number) {
        Tokens tokens = Parser::Parse({ pLeft, _1, pRight });
        AssertRange::AreEqual({ _1 }, tokens);
    }

    TEST_METHOD(Should_parse_expression_with_parens_in_beginning) {
        Tokens tokens = Parser::Parse({ pLeft, _1, plus, _2, pRight, mul, _3 });
        AssertRange::AreEqual({ _1, _2, plus, _3, mul }, tokens);
    }

    TEST_METHOD(Should_throw_when_opening_paren_not_found) {
        Assert::ExpectException<std::logic_error>([]() {Parser::Parse({ _1, pRight }); });
    }

    TEST_METHOD(Should_throw_when_closing_paren_not_found) {
        Assert::ExpectException<std::logic_error>([]() {Parser::Parse({ pLeft, _1 }); });
    }

    TEST_METHOD(Should_parse_complex_experssion_with_paren) {
        // (1+2)*(3/(4-5)) = 1 2 + 3 4 5 - / *
        Tokens tokens = Parser::Parse({ pLeft, _1, plus, _2, pRight, mul, pLeft, _3, div, pLeft, _4, minus, _5, pRight, pRight });
        AssertRange::AreEqual({ _1, _2, plus, _3, _4, _5, minus, div, mul }, tokens);
    }
};

TEST_CLASS(EvaluatorTests) {
public:
    TEST_METHOD(Should_return_zero_when_evaluete_empty_list) {
        double result = Evaluator::Evaluate({});
        Assert::AreEqual(0.0, result);
    }

    TEST_METHOD(Should_return_number_when_evaluete_list_with_number) {
        double result = Evaluator::Evaluate({ _1 });
        Assert::AreEqual(1.0, result);
    }

    TEST_METHOD(Should_eval_expression_with_one_operator) {
        double result = Evaluator::Evaluate({ _1, _2, plus });
        Assert::AreEqual(3.0, result);
    }

    TEST_METHOD(Should_eval_expression_with_one_multiplication) {
        double result = Evaluator::Evaluate({ _2, _3, mul });
        Assert::AreEqual(6.0, result);
    }

    TEST_METHOD(Should_eval_expression_with_one_subtraction) {
        double result = Evaluator::Evaluate({ _2, _3, minus });
        Assert::AreEqual(-1.0, result);
    }

    TEST_METHOD(Should_eval_expression_with_one_division) {
        double result = Evaluator::Evaluate({ _5, _2, div });
        Assert::AreEqual(2.5, result);
    }

    TEST_METHOD(Should_eval_complex_expression) {
        // (4+1)*2/(4/(3-1)) = 4 1 + 2 * 4 3 1 - / /  = 5
        double result = Evaluator::Evaluate({ _4, _1, plus, _2, mul, _4, _3, _1, minus, div, div });
        Assert::AreEqual(5.0, result);
    }
};

TEST_CLASS(InterpreterIntegrationTests) {
public:
    TEST_METHOD(Should_interprete_empty_experssion) {
        double result = Interpreter::InterpreteExperssion(L"  ");
        Assert::AreEqual(0.0, result);
    }

    TEST_METHOD(Should_interprete_experssion) {
        double result = Interpreter::InterpreteExperssion(L"1+2");
        Assert::AreEqual(3.0, result);
    }
};

}