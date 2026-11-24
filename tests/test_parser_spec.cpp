#include <catch2/catch_test_macros.hpp>

#include <string>
#include <variant>
#include <vector>

#include "egg/ast.h"
#include "egg/parser.h"
#include "helpers.h"


namespace {

const egg::ast::Program& program(const egg::ParseResult& result) {
    return std::get<egg::ast::Program>(result);
}

const egg::ParseDiagnostic& diagnostic(const egg::ParseResult& result) {
    return std::get<egg::ParseDiagnostic>(result);
}

egg::ParseResult parse(const std::string& source) {
    auto lexed = test::lex(source);
    REQUIRE_FALSE(lexed.error.has_value());

    egg::Parser parser(std::move(lexed.tokens), "main.egg");
    return parser.parse();
}

} // namespace


TEST_CASE("parser: empty token stream produces empty program", "[parser][spec]") {
    auto result = parse("");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& ast = program(result);
    CHECK(ast.source_name == "main.egg");
    CHECK(ast.declarations.empty());
    CHECK(ast.range.begin.line == 1);
    CHECK(ast.range.begin.column == 1);
    CHECK(ast.range.end.line == 1);
    CHECK(ast.range.end.column == 1);
}


TEST_CASE("parser: empty main is parsed", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() {}");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& ast = program(result);
    REQUIRE(ast.declarations.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::FunctionDecl>(ast.declarations[0]));

    const auto& function = std::get<egg::ast::FunctionDecl>(ast.declarations[0]);
    CHECK(function.name.name == "main");
    CHECK(function.parameters.empty());
    CHECK_FALSE(function.return_type.has_value());
    CHECK(function.body.statements.empty());
}


TEST_CASE("parser: explicit void return type is preserved", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() -> void {}");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& function = std::get<egg::ast::FunctionDecl>(program(result).declarations[0]);
    REQUIRE(function.return_type.has_value());
    REQUIRE(std::holds_alternative<egg::ast::VoidType>(function.return_type.value()));
}


TEST_CASE("parser: function parameters preserve order", "[parser][spec][!mayfail]") {
    auto result = parse("fn f(int a, bool b) {}");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& function = std::get<egg::ast::FunctionDecl>(program(result).declarations[0]);
    REQUIRE(function.parameters.size() == 2);

    CHECK(function.parameters[0].name.name == "a");
    CHECK(std::holds_alternative<egg::ast::PrimitiveType>(
        function.parameters[0].type.value));
    CHECK(std::get<egg::ast::PrimitiveType>(function.parameters[0].type.value) ==
          egg::ast::PrimitiveType::Int);

    CHECK(function.parameters[1].name.name == "b");
    CHECK(std::get<egg::ast::PrimitiveType>(function.parameters[1].type.value) ==
          egg::ast::PrimitiveType::Bool);
}


TEST_CASE("parser: class field is parsed", "[parser][spec][!mayfail]") {
    auto result = parse("class C { int x; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& ast = program(result);
    REQUIRE(ast.declarations.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::ClassDecl>(ast.declarations[0]));

    const auto& class_decl = std::get<egg::ast::ClassDecl>(ast.declarations[0]);
    CHECK(class_decl.name.name == "C");
    REQUIRE(class_decl.members.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::FieldDecl>(class_decl.members[0]));

    const auto& field = std::get<egg::ast::FieldDecl>(class_decl.members[0]);
    CHECK(field.name.name == "x");
}


TEST_CASE("parser: constructor is parsed", "[parser][spec][!mayfail]") {
    auto result = parse("class C { constructor() {} }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& class_decl = std::get<egg::ast::ClassDecl>(program(result).declarations[0]);
    REQUIRE(class_decl.members.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::ConstructorDecl>(class_decl.members[0]));
}


TEST_CASE("parser: method is parsed inside class", "[parser][spec][!mayfail]") {
    auto result = parse("class C { fn f() {} }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& class_decl = std::get<egg::ast::ClassDecl>(program(result).declarations[0]);
    REQUIRE(class_decl.members.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::FunctionDecl>(class_decl.members[0]));
}


TEST_CASE("parser: variable declaration without initializer", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { int x; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(body.statements.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::VariableDecl>(body.statements[0]->value));

    const auto& declaration = std::get<egg::ast::VariableDecl>(body.statements[0]->value);
    CHECK(declaration.name.name == "x");
    CHECK(declaration.initializer == nullptr);
}


TEST_CASE("parser: variable declaration with initializer", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { int x = 1; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(body.statements.size() == 1);

    const auto& declaration = std::get<egg::ast::VariableDecl>(body.statements[0]->value);
    REQUIRE(declaration.initializer != nullptr);
    REQUIRE(std::holds_alternative<egg::ast::IntegerLiteral>(declaration.initializer->value));
    CHECK(std::get<egg::ast::IntegerLiteral>(declaration.initializer->value).digits == "1");
}


TEST_CASE("parser: assignment is a statement", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { x = 1; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(body.statements.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::AssignmentStmt>(body.statements[0]->value));
}


TEST_CASE("parser: call expression statement", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { f(); }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(body.statements.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::ExpressionStmt>(body.statements[0]->value));

    const auto& statement = std::get<egg::ast::ExpressionStmt>(body.statements[0]->value);
    REQUIRE(std::holds_alternative<egg::ast::CallExpr>(statement.expression->value));
}


TEST_CASE("parser: empty block statement", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { {} }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(body.statements.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::Block>(body.statements[0]->value));
}


TEST_CASE("parser: if without else", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { if (x) {} }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(body.statements.size() == 1);

    const auto& statement = std::get<egg::ast::IfStmt>(body.statements[0]->value);
    CHECK(std::holds_alternative<std::monostate>(statement.else_branch));
}


TEST_CASE("parser: if with else block", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { if (x) {} else {} }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::IfStmt>(body.statements[0]->value);

    REQUIRE(std::holds_alternative<egg::ast::Block>(statement.else_branch));
}


TEST_CASE("parser: else if is nested if", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { if (x) {} else if (y) {} }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::IfStmt>(body.statements[0]->value);

    REQUIRE(std::holds_alternative<std::unique_ptr<egg::ast::IfStmt>>(statement.else_branch));
}


TEST_CASE("parser: while statement", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { while (x) {} }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(body.statements.size() == 1);
    REQUIRE(std::holds_alternative<egg::ast::WhileStmt>(body.statements[0]->value));
}


TEST_CASE("parser: break and continue statements", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { break; continue; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(body.statements.size() == 2);
    CHECK(std::holds_alternative<egg::ast::BreakStmt>(body.statements[0]->value));
    CHECK(std::holds_alternative<egg::ast::ContinueStmt>(body.statements[1]->value));
}


TEST_CASE("parser: return without value", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { return; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::ReturnStmt>(body.statements[0]->value);
    CHECK(statement.value == nullptr);
}


TEST_CASE("parser: return with value", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { return 1; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::ReturnStmt>(body.statements[0]->value);
    REQUIRE(statement.value != nullptr);
    REQUIRE(std::holds_alternative<egg::ast::IntegerLiteral>(statement.value->value));
}


TEST_CASE("parser: addition groups from left", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { 1 + 2 + 3; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::ExpressionStmt>(body.statements[0]->value);

    const auto& outer = std::get<egg::ast::BinaryExpr>(statement.expression->value);
    CHECK(outer.op == egg::ast::BinaryOp::Add);

    const auto& left = std::get<egg::ast::BinaryExpr>(outer.left->value);
    CHECK(left.op == egg::ast::BinaryOp::Add);
}


TEST_CASE("parser: unary operations group from right", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { !!x; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::ExpressionStmt>(body.statements[0]->value);

    const auto& outer = std::get<egg::ast::UnaryExpr>(statement.expression->value);
    CHECK(outer.op == egg::ast::UnaryOp::LogicalNot);

    const auto& inner = std::get<egg::ast::UnaryExpr>(outer.operand->value);
    CHECK(inner.op == egg::ast::UnaryOp::LogicalNot);
}


TEST_CASE("parser: single comparison is allowed", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { a < b; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::ExpressionStmt>(body.statements[0]->value);

    const auto& expression = std::get<egg::ast::BinaryExpr>(statement.expression->value);
    CHECK(expression.op == egg::ast::BinaryOp::Less);
}


TEST_CASE("parser: chained comparison is rejected", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { a < b < c; }");

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));

    const auto& error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::ChainedComparison);
    CHECK(error.source_name == "main.egg");
}


TEST_CASE("parser: parenthesized comparison is allowed", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { (a < b) < c; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));
}


TEST_CASE("parser: member call is parsed", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { make().run(); }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::ExpressionStmt>(body.statements[0]->value);

    const auto& member = std::get<egg::ast::MemberExpr>(statement.expression->value);
    REQUIRE(std::holds_alternative<egg::ast::CallExpr>(member.receiver->value));
}


TEST_CASE("parser: call of call is syntactically valid", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { make()(); }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::ExpressionStmt>(body.statements[0]->value);

    const auto& call = std::get<egg::ast::CallExpr>(statement.expression->value);
    REQUIRE(std::holds_alternative<egg::ast::CallExpr>(call.callee->value));
}


TEST_CASE("parser: assignment to literal is syntactically valid", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { 1 = 2; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(std::holds_alternative<egg::ast::AssignmentStmt>(body.statements[0]->value));
}


TEST_CASE("parser: break outside loop is syntactically valid", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { break; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    REQUIRE(std::holds_alternative<egg::ast::BreakStmt>(body.statements[0]->value));
}


TEST_CASE("parser: vector literal is parsed", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { vector<int>[1, 2]; }");

    REQUIRE(std::holds_alternative<egg::ast::Program>(result));

    const auto& body = std::get<egg::ast::FunctionDecl>(
        program(result).declarations[0]).body;
    const auto& statement = std::get<egg::ast::ExpressionStmt>(body.statements[0]->value);

    const auto& literal = std::get<egg::ast::VectorLiteral>(statement.expression->value);
    CHECK(std::get<egg::ast::PrimitiveType>(literal.element_type.value) ==
          egg::ast::PrimitiveType::Int);
    CHECK(literal.elements.size() == 2);
}


TEST_CASE("parser: unterminated function reports unexpected end", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() {");

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));

    const auto& error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::UnexpectedEnd);
    CHECK(error.source_name == "main.egg");
    CHECK_FALSE(error.actual.has_value());
}


TEST_CASE("parser: missing closing parenthesis reports unexpected token", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() { if (x {} }");

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));

    const auto& error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::UnexpectedToken);
    CHECK(error.source_name == "main.egg");
    REQUIRE(error.actual.has_value());
    CHECK(error.actual.value() == egg::TokenKind::LeftBrace);
}


TEST_CASE("parser: trailing token reports unexpected token", "[parser][spec][!mayfail]") {
    auto result = parse("fn main() {} fn");

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));

    const auto& error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::UnexpectedToken);
    CHECK(error.actual.has_value());
    CHECK(error.actual.value() == egg::TokenKind::KwFn);
}


TEST_CASE("parser: missing EOF reports invalid token stream", "[parser][spec][!mayfail]") {
    egg::Lexer lexer("fn main() {}");
    auto lexed = lexer.tokenize();
    REQUIRE_FALSE(lexed.error.has_value());

    egg::Parser parser(std::move(lexed.tokens), "main.egg");
    auto result = parser.parse();

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));
    CHECK(std::get<egg::ParseDiagnostic>(result).code ==
          egg::ParseErrorCode::InvalidTokenStream);
}


TEST_CASE("parser: repeated EOF reports invalid token stream", "[parser][spec][!mayfail]") {
    egg::Lexer lexer("fn main() {}");
    auto lexed = lexer.tokenize();
    REQUIRE_FALSE(lexed.error.has_value());

    auto tokens = lexed.tokens;
    tokens.push_back(tokens.back());

    egg::Parser parser(std::move(tokens), "main.egg");
    auto result = parser.parse();

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));
    CHECK(std::get<egg::ParseDiagnostic>(result).code ==
          egg::ParseErrorCode::InvalidTokenStream);
}


TEST_CASE("parser: repeated parse returns structurally equal result", "[parser][spec][!mayfail]") {
    egg::Lexer lexer("fn main() {}");
    auto lexed = lexer.tokenize();
    REQUIRE_FALSE(lexed.error.has_value());

    egg::Parser parser(std::move(lexed.tokens), "main.egg");
    auto first = parser.parse();
    auto second = parser.parse();

    REQUIRE(std::holds_alternative<egg::ast::Program>(first));
    REQUIRE(std::holds_alternative<egg::ast::Program>(second));

    const auto& first_program = std::get<egg::ast::Program>(first);
    const auto& second_program = std::get<egg::ast::Program>(second);

    CHECK(first_program.source_name == second_program.source_name);
    CHECK(first_program.declarations.size() == second_program.declarations.size());
}

