#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "egg/lexer.h"
#include "egg/parser.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace {

namespace ast = egg::ast;
using egg::ParseDiagnostic;
using egg::ParseErrorCode;
using egg::TokenKind;

constexpr std::string_view function_prefix = "fn main() { ";

egg::ParseResult parse_body(const std::string &body)
{
    auto lexed = egg::Lexer(std::string(function_prefix) + body + " }").tokenize();
    REQUIRE_FALSE(lexed.error.has_value());
    return egg::Parser(std::move(lexed.tokens)).parse();
}

ast::StmtPtr parse_statement(const std::string &body)
{
    auto result = parse_body(body);
    REQUIRE(std::holds_alternative<ast::Program>(result));
    auto &declarations = std::get<ast::Program>(result).declarations;
    REQUIRE(declarations.size() == 1);
    REQUIRE(std::holds_alternative<ast::FunctionDecl>(declarations.front()));
    auto &statements = std::get<ast::FunctionDecl>(declarations.front()).body.statements;
    REQUIRE(statements.size() == 1);
    REQUIRE(statements.front());
    return std::move(statements.front());
}

ast::ExprPtr parse_expression(const std::string &source)
{
    auto statement = parse_statement(source + ";");
    auto *expression = std::get_if<ast::ExpressionStmt>(&statement->value);
    REQUIRE(expression);
    return std::move(expression->expression);
}

template <typename Node>
const Node &as(const ast::ExprPtr &expression)
{
    REQUIRE(expression);
    const auto *node = std::get_if<Node>(&expression->value);
    REQUIRE(node);
    return *node;
}

void check_range(egg::SourceRange range, std::size_t begin, std::size_t end)
{
    const auto offset = function_prefix.size();
    CHECK(range.begin.byte_offset == offset + begin);
    CHECK(range.end.byte_offset == offset + end);
    CHECK(range.begin.line == 1);
    CHECK(range.end.line == 1);
    CHECK(range.begin.column == offset + begin + 1);
    CHECK(range.end.column == offset + end + 1);
}

}

TEST_CASE("Arithmetic expressions respect precedence and associate left", "[parser][expressions]")
{
    SECTION("multiplication binds more tightly than addition") {
        const auto expression = parse_expression("1 + 2 * 3");
        const auto &sum = as<ast::BinaryExpr>(expression);
        CHECK(sum.op == ast::BinaryOp::Add);
        CHECK(as<ast::IntegerLiteral>(sum.left).digits == "1");
        const auto &product = as<ast::BinaryExpr>(sum.right);
        CHECK(product.op == ast::BinaryOp::Multiply);
        CHECK(as<ast::IntegerLiteral>(product.left).digits == "2");
        CHECK(as<ast::IntegerLiteral>(product.right).digits == "3");
        check_range(expression->range, 0, 9);
        check_range(sum.operator_range, 2, 3);
        check_range(sum.right->range, 4, 9);
        check_range(product.operator_range, 6, 7);
    }

    SECTION("successive subtractions group left") {
        const auto expression = parse_expression("a - b - c");
        const auto &outer = as<ast::BinaryExpr>(expression);
        const auto &inner = as<ast::BinaryExpr>(outer.left);
        CHECK(outer.op == ast::BinaryOp::Subtract);
        CHECK(inner.op == ast::BinaryOp::Subtract);
        CHECK(as<ast::NameExpr>(inner.left).name.name == "a");
        CHECK(as<ast::NameExpr>(inner.right).name.name == "b");
        CHECK(as<ast::NameExpr>(outer.right).name.name == "c");
    }

    SECTION("division and remainder share a level") {
        const auto expression = parse_expression("a / b % c");
        const auto &remainder = as<ast::BinaryExpr>(expression);
        CHECK(remainder.op == ast::BinaryOp::Remainder);
        CHECK(as<ast::BinaryExpr>(remainder.left).op == ast::BinaryOp::Divide);
    }
}

TEST_CASE("Unary expressions nest right", "[parser][expressions]")
{
    SECTION("logical not applies to another unary expression") {
        const auto expression = parse_expression("!!x");
        const auto &outer = as<ast::UnaryExpr>(expression);
        const auto &inner = as<ast::UnaryExpr>(outer.operand);
        CHECK(outer.op == ast::UnaryOp::LogicalNot);
        CHECK(inner.op == ast::UnaryOp::LogicalNot);
        CHECK(as<ast::NameExpr>(inner.operand).name.name == "x");
        check_range(outer.operator_range, 0, 1);
        check_range(inner.operator_range, 1, 2);
    }

    SECTION("binary minus can be followed by unary minus") {
        const auto expression = parse_expression("a--b");
        const auto &difference = as<ast::BinaryExpr>(expression);
        CHECK(difference.op == ast::BinaryOp::Subtract);
        CHECK(as<ast::NameExpr>(difference.left).name.name == "a");
        const auto &negation = as<ast::UnaryExpr>(difference.right);
        CHECK(negation.op == ast::UnaryOp::Negate);
        CHECK(as<ast::NameExpr>(negation.operand).name.name == "b");
    }
}

TEST_CASE("Logical operators respect precedence and associate left", "[parser][expressions]")
{
    const auto expression = parse_expression("a || b && c");
    const auto &disjunction = as<ast::BinaryExpr>(expression);
    CHECK(disjunction.op == ast::BinaryOp::LogicalOr);
    CHECK(as<ast::NameExpr>(disjunction.left).name.name == "a");
    const auto &conjunction = as<ast::BinaryExpr>(disjunction.right);
    CHECK(conjunction.op == ast::BinaryOp::LogicalAnd);
    CHECK(as<ast::NameExpr>(conjunction.left).name.name == "b");
    CHECK(as<ast::NameExpr>(conjunction.right).name.name == "c");

    const auto repeated_or = parse_expression("a || b || c");
    const auto repeated_and = parse_expression("a && b && c");
    const auto &outer_or = as<ast::BinaryExpr>(repeated_or);
    const auto &outer_and = as<ast::BinaryExpr>(repeated_and);
    CHECK(as<ast::BinaryExpr>(outer_or.left).op == ast::BinaryOp::LogicalOr);
    CHECK(as<ast::BinaryExpr>(outer_and.left).op == ast::BinaryOp::LogicalAnd);
}

TEST_CASE("Postfix operators wrap the preceding expression", "[parser][expressions]")
{
    const auto expression = parse_expression("nodes[i].method(x)[j]");
    const auto &outer = as<ast::IndexExpr>(expression);
    CHECK(as<ast::NameExpr>(outer.index).name.name == "j");

    const auto &call = as<ast::CallExpr>(outer.receiver);
    REQUIRE(call.arguments.size() == 1);
    CHECK(as<ast::NameExpr>(call.arguments.front()).name.name == "x");

    const auto &member = as<ast::MemberExpr>(call.callee);
    CHECK(member.member.name == "method");
    const auto &inner = as<ast::IndexExpr>(member.receiver);
    CHECK(as<ast::NameExpr>(inner.receiver).name.name == "nodes");
    CHECK(as<ast::NameExpr>(inner.index).name.name == "i");

    check_range(expression->range, 0, 21);
    check_range(outer.receiver->range, 0, 18);
    check_range(call.callee->range, 0, 15);
    check_range(member.receiver->range, 0, 8);
    check_range(member.member.range, 9, 15);
}

TEST_CASE("Comparisons report the second unparenthesized operator", "[parser][expressions]")
{
    for (const std::string source : {"a < b == c", "a == b == c"}) {
        CAPTURE(source);
        const auto result = parse_body(source + ";");
        const auto *error = std::get_if<ParseDiagnostic>(&result);
        REQUIRE(error);
        CHECK(error->code == ParseErrorCode::ChainedComparison);
        REQUIRE(error->actual);
        CHECK(*error->actual == TokenKind::Equal);
        const auto second_operator = source.rfind("==");
        check_range(error->range, second_operator, second_operator + 2);
    }

    const auto expression = parse_expression("(a < b) == c");
    const auto &equality = as<ast::BinaryExpr>(expression);
    CHECK(equality.op == ast::BinaryOp::Equal);
    const auto &parenthesized = as<ast::ParenthesizedExpr>(equality.left);
    CHECK(as<ast::BinaryExpr>(parenthesized.expression).op == ast::BinaryOp::Less);
    CHECK(as<ast::NameExpr>(equality.right).name.name == "c");
    check_range(equality.left->range, 0, 7);
    check_range(parenthesized.expression->range, 1, 6);
}

TEST_CASE("Literal expressions preserve their values", "[parser][expressions]")
{
    const std::string digits = "000922337203685477580899";
    CHECK(as<ast::IntegerLiteral>(parse_expression(digits)).digits == digits);

    const auto negative = parse_expression("-5");
    const auto &unary = as<ast::UnaryExpr>(negative);
    CHECK(unary.op == ast::UnaryOp::Negate);
    CHECK(as<ast::IntegerLiteral>(unary.operand).digits == "5");
    check_range(negative->range, 0, 2);
    check_range(unary.operand->range, 1, 2);

    CHECK(as<ast::BooleanLiteral>(parse_expression("true")).value);
    CHECK_FALSE(as<ast::BooleanLiteral>(parse_expression("false")).value);
    CHECK(as<ast::CharacterLiteral>(parse_expression(R"('\x41')")).value.ascii == 'A');

    const auto text = parse_expression(R"("a\0b")");
    CHECK(as<ast::TextLiteral>(text).value.ascii == std::string("a\0b", 3));
    check_range(text->range, 0, 6);

    const auto receiver = parse_expression("this");
    REQUIRE(receiver);
    CHECK(std::holds_alternative<ast::ThisExpr>(receiver->value));
}

TEST_CASE("Vector literals keep their element type and elements", "[parser][expressions]")
{
    const auto empty = parse_expression("vector<int>[]");
    const auto &empty_vector = as<ast::VectorLiteral>(empty);
    CHECK(empty_vector.elements.empty());
    CHECK(std::get<ast::PrimitiveType>(empty_vector.element_type.value) == ast::PrimitiveType::Int);
    check_range(empty->range, 0, 13);
    check_range(empty_vector.element_type.range, 7, 10);

    const std::string source = "vector<vector<int>>[vector<int>[], vector<int>[1, 2]]";
    const auto nested = parse_expression(source);
    const auto &outer = as<ast::VectorLiteral>(nested);
    const auto *element_type = std::get_if<ast::VectorType>(&outer.element_type.value);
    REQUIRE(element_type);
    REQUIRE(element_type->element);
    CHECK(std::get<ast::PrimitiveType>(element_type->element->value) == ast::PrimitiveType::Int);
    REQUIRE(outer.elements.size() == 2);
    CHECK(as<ast::VectorLiteral>(outer.elements[0]).elements.empty());
    const auto &inner = as<ast::VectorLiteral>(outer.elements[1]);
    REQUIRE(inner.elements.size() == 2);
    CHECK(as<ast::IntegerLiteral>(inner.elements[0]).digits == "1");
    CHECK(as<ast::IntegerLiteral>(inner.elements[1]).digits == "2");
    check_range(nested->range, 0, source.size());
    check_range(outer.element_type.range, 7, 18);
}

TEST_CASE("Call expressions accept empty arguments and nested calls", "[parser][expressions]")
{
    const auto empty = parse_expression("f()");
    const auto &call = as<ast::CallExpr>(empty);
    CHECK(call.arguments.empty());
    CHECK(as<ast::NameExpr>(call.callee).name.name == "f");

    const auto repeated = parse_expression("make()()");
    const auto &outer = as<ast::CallExpr>(repeated);
    CHECK(outer.arguments.empty());
    const auto &inner = as<ast::CallExpr>(outer.callee);
    CHECK(inner.arguments.empty());
    CHECK(as<ast::NameExpr>(inner.callee).name.name == "make");

    const auto nested = parse_expression("exit(read_line())");
    const auto &exit = as<ast::CallExpr>(nested);
    REQUIRE(exit.arguments.size() == 1);
    CHECK(as<ast::NameExpr>(exit.callee).name.name == "exit");
    const auto &read_line = as<ast::CallExpr>(exit.arguments.front());
    CHECK(as<ast::NameExpr>(read_line.callee).name.name == "read_line");
}

TEST_CASE("Argument lists reject trailing commas", "[parser][expressions]")
{
    const std::pair<std::string, TokenKind> cases[] = {
        {"f(1,);", TokenKind::RightParen},
        {"vector<int>[1,];", TokenKind::RightBracket}
    };

    for (const auto &[source, closing] : cases) {
        CAPTURE(source);
        const auto result = parse_body(source);
        const auto *error = std::get_if<ParseDiagnostic>(&result);
        REQUIRE(error);
        CHECK(error->code == ParseErrorCode::UnexpectedToken);
        REQUIRE(error->actual);
        CHECK(*error->actual == closing);
        check_range(error->range, source.size() - 2, source.size() - 1);

        const TokenKind starts[] = {
            TokenKind::IntegerLiteral, TokenKind::CharLiteral, TokenKind::TextLiteral,
            TokenKind::KwTrue, TokenKind::KwFalse, TokenKind::Identifier,
            TokenKind::KwThis, TokenKind::LeftParen, TokenKind::KwVector,
            TokenKind::Bang, TokenKind::Minus
        };
        for (const auto start : starts) {
            CHECK(std::find(error->expected.begin(), error->expected.end(), start)
                  != error->expected.end());
        }
    }
}

TEST_CASE("An expression leaves assignment and statement delimiters to the caller",
          "[parser][expressions]")
{
    const auto statement = parse_statement("nodes[i] = f(1, 2);");
    const auto *assignment = std::get_if<ast::AssignmentStmt>(&statement->value);
    REQUIRE(assignment);
    const auto &index = as<ast::IndexExpr>(assignment->target);
    CHECK(as<ast::NameExpr>(index.index).name.name == "i");
    const auto &call = as<ast::CallExpr>(assignment->value);
    REQUIRE(call.arguments.size() == 2);
    CHECK(as<ast::IntegerLiteral>(call.arguments[0]).digits == "1");
    CHECK(as<ast::IntegerLiteral>(call.arguments[1]).digits == "2");
    check_range(assignment->target->range, 0, 8);
    check_range(assignment->operator_range, 9, 10);
    check_range(assignment->value->range, 11, 18);
    check_range(statement->range, 0, 19);
}
