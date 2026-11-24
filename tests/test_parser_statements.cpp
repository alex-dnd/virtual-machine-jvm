
#include <catch2/catch_test_macros.hpp>
#include "egg/lexer.h"
#include "egg/parser.h"

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {

egg::ParseResult parse_source(const std::string& source) {
    auto lexed = egg::Lexer(source).tokenize();
    REQUIRE_FALSE(lexed.error.has_value());

    return egg::Parser(std::move(lexed.tokens)).parse();
}

const egg::ast::Block& function_body(const egg::ParseResult& result) {
    const auto* program = std::get_if<egg::ast::Program>(&result);
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);

    const auto* fn =
        std::get_if<egg::ast::FunctionDecl>(&program->declarations[0]);
    REQUIRE(fn != nullptr);

    return fn->body;
}

} // namespace

TEST_CASE("Пустые и вложенные блоки", "[parser][statements][.]") {
    auto result = parse_source("fn main() { {} { {} } }");
    const auto& body = function_body(result);

    REQUIRE(body.statements.size() == 2);

    const auto* empty =
        std::get_if<egg::ast::Block>(&body.statements[0]->value);
    REQUIRE(empty != nullptr);
    CHECK(empty->statements.empty());

    const auto* nested =
        std::get_if<egg::ast::Block>(&body.statements[1]->value);
    REQUIRE(nested != nullptr);
    REQUIRE(nested->statements.size() == 1);

    CHECK(std::holds_alternative<egg::ast::Block>(
        nested->statements[0]->value));
}

TEST_CASE("Объявление переменных", "[parser][statements][.]") {
    auto result = parse_source(
        "fn main() { int x; int y = 42; }");
    const auto& body = function_body(result);

    REQUIRE(body.statements.size() == 2);

    const auto* x =
        std::get_if<egg::ast::VariableDecl>(&body.statements[0]->value);
    const auto* y =
        std::get_if<egg::ast::VariableDecl>(&body.statements[1]->value);

    REQUIRE(x != nullptr);
    REQUIRE(y != nullptr);

    CHECK(x->name.name == "x");
    CHECK(x->initializer == nullptr);
    CHECK(y->name.name == "y");
    CHECK(y->initializer != nullptr);
}

TEST_CASE("Цепочка if else if else", "[parser][statements][.]") {
    auto result = parse_source(
        "fn main() { if (x) {} else if (y) {} else {} }");
    const auto& body = function_body(result);

    REQUIRE(body.statements.size() == 1);

    const auto* first =
        std::get_if<egg::ast::IfStmt>(&body.statements[0]->value);
    REQUIRE(first != nullptr);

    const auto* second =
        std::get_if<std::unique_ptr<egg::ast::IfStmt>>(
            &first->else_branch);
    REQUIRE(second != nullptr);
    REQUIRE(*second != nullptr);

    CHECK(std::holds_alternative<egg::ast::Block>(
        (*second)->else_branch));
}

TEST_CASE("If без else", "[parser][statements][.]") {
    auto result = parse_source("fn main() { if (x) {} }");
    const auto& body = function_body(result);

    const auto* branch =
        std::get_if<egg::ast::IfStmt>(&body.statements[0]->value);
    REQUIRE(branch != nullptr);

    CHECK(std::holds_alternative<std::monostate>(
        branch->else_branch));
}

TEST_CASE("Циклы и return", "[parser][statements][.]") {
    auto result = parse_source(
        "fn main() { while (x) { break; continue; } return; return 1; }");
    const auto& body = function_body(result);

    REQUIRE(body.statements.size() == 3);

    const auto* loop =
        std::get_if<egg::ast::WhileStmt>(&body.statements[0]->value);
    REQUIRE(loop != nullptr);
    REQUIRE(loop->body.statements.size() == 2);

    CHECK(std::holds_alternative<egg::ast::BreakStmt>(
        loop->body.statements[0]->value));
    CHECK(std::holds_alternative<egg::ast::ContinueStmt>(
        loop->body.statements[1]->value));

    const auto* r1 =
        std::get_if<egg::ast::ReturnStmt>(&body.statements[1]->value);
    const auto* r2 =
        std::get_if<egg::ast::ReturnStmt>(&body.statements[2]->value);

    REQUIRE(r1 != nullptr);
    REQUIRE(r2 != nullptr);

    CHECK(r1->value == nullptr);
    CHECK(r2->value != nullptr);
}

TEST_CASE("Присваивания и выражения", "[parser][statements][.]") {
    const std::string source =
        "fn main() { a.b = 1; xs[0] = 1; 1 = 2; break; foo(); }";

    auto result = parse_source(source);
    const auto& body = function_body(result);

    REQUIRE(body.statements.size() == 5);

    const auto* field =
        std::get_if<egg::ast::AssignmentStmt>(
            &body.statements[0]->value);
    const auto* index =
        std::get_if<egg::ast::AssignmentStmt>(
            &body.statements[1]->value);
    const auto* number =
        std::get_if<egg::ast::AssignmentStmt>(
            &body.statements[2]->value);

    REQUIRE(field != nullptr);
    REQUIRE(index != nullptr);
    REQUIRE(number != nullptr);

    CHECK(std::holds_alternative<egg::ast::MemberExpr>(
        field->target->value));
    CHECK(std::holds_alternative<egg::ast::IndexExpr>(
        index->target->value));
    CHECK(std::holds_alternative<egg::ast::IntegerLiteral>(
        number->target->value));

    CHECK(field->operator_range.begin.byte_offset == source.find('='));
    CHECK(field->operator_range.end.byte_offset == source.find('=') + 1);

    CHECK(std::holds_alternative<egg::ast::BreakStmt>(
        body.statements[3]->value));
    CHECK(std::holds_alternative<egg::ast::ExpressionStmt>(
        body.statements[4]->value));
}

TEST_CASE("Ошибки операторов", "[parser][statements][.]") {
    const std::vector<std::string> invalid = {
        "fn main() { if (x) y = 1; }",
        "fn main() { while (x) break; }",
        "fn main() { if (x) {} else return; }",
        "fn main() { ; }",
        "fn main() { int x }",
        "fn main() { return 1 }",
        "fn main() { x = 1 }",
        "fn main() { a = b = c; }"
    };

    for (const auto& source : invalid) {
        INFO(source);

        auto result = parse_source(source);
        CHECK(std::holds_alternative<egg::ParseDiagnostic>(result));
    }
}

TEST_CASE("Точные диапазоны ошибок", "[parser][statements][.]") {
    auto eof = parse_source("fn main() {");
    const auto* eof_error =
        std::get_if<egg::ParseDiagnostic>(&eof);

    REQUIRE(eof_error != nullptr);
    CHECK(eof_error->code == egg::ParseErrorCode::UnexpectedEnd);

    const std::string source = "fn main() { a = b = c; }";
    auto chained = parse_source(source);
    const auto* error =
        std::get_if<egg::ParseDiagnostic>(&chained);

    REQUIRE(error != nullptr);
    CHECK(error->code == egg::ParseErrorCode::UnexpectedToken);
    CHECK(error->range.begin.byte_offset == source.rfind('='));
}
