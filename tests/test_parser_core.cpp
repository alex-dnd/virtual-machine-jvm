#include <catch2/catch_test_macros.hpp>

#include "egg/lexer.h"
#include "egg/parser.h"

#include <utility>
#include <variant>

namespace {
egg::ParseResult parse(const std::string &source,
                        const std::string &source_name = "<test>") {
    auto lexed = egg::Lexer(source, source_name).tokenize();
    return egg::Parser(std::move(lexed.tokens), source_name).parse();
}

egg::Token make_token(egg::TokenKind kind, std::size_t begin,
                      std::size_t end, std::string lexeme = {}) {
    const egg::SourcePosition range_begin{begin, 1, begin + 1};
    const egg::SourcePosition range_end{end, 1, end + 1};
    return egg::Token{kind, {range_begin, range_end}, std::move(lexeme), {}};
}

const egg::ParseDiagnostic &diagnostic(const egg::ParseResult &result) {
    return std::get<egg::ParseDiagnostic>(result);
}
} // namespace

TEST_CASE("Пустая программа разбирается без объявлений", "[parser][core]") {
    const auto result = parse("");

    const auto *program = std::get_if<egg::ast::Program>(&result);
    REQUIRE(program);
    CHECK(program->declarations.empty());
    CHECK(program->source_name == "<test>");
}

TEST_CASE("Повторный parse начинает разбор с начала потока", "[parser][core]") {
    auto lexed = egg::Lexer("").tokenize();
    REQUIRE_FALSE(lexed.error);
    const egg::Parser parser(std::move(lexed.tokens));

    const auto first = parser.parse();
    const auto second = parser.parse();

    REQUIRE(std::holds_alternative<egg::ast::Program>(first));
    REQUIRE(std::holds_alternative<egg::ast::Program>(second));
    CHECK(std::get<egg::ast::Program>(first).range.begin.byte_offset ==
          std::get<egg::ast::Program>(second).range.begin.byte_offset);
    CHECK(std::get<egg::ast::Program>(first).range.end.byte_offset ==
          std::get<egg::ast::Program>(second).range.end.byte_offset);
}

TEST_CASE("Пустой поток токенов отклоняется", "[parser][core]") {
    const auto result = egg::Parser({}).parse();

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));
    const auto &error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::InvalidTokenStream);
    CHECK_FALSE(error.actual);
    CHECK(error.range.begin.byte_offset == 0);
    CHECK(error.range.begin.line == 1);
    CHECK(error.range.begin.column == 1);
    CHECK(error.range.end.byte_offset == 0);
}

TEST_CASE("Поток без EOF отклоняется в позиции после последнего токена",
          "[parser][core]") {
    const auto id = make_token(egg::TokenKind::Identifier, 0, 1, "x");
    const auto result = egg::Parser({id}).parse();

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));
    const auto &error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::InvalidTokenStream);
    CHECK_FALSE(error.actual);
    CHECK(error.range.begin.byte_offset == 1);
    CHECK(error.range.end.byte_offset == 1);
}

TEST_CASE("Повторный EOF указывает на первый EOF до конца потока",
          "[parser][core]") {
    const auto eof = make_token(egg::TokenKind::EndOfFile, 0, 0);
    const auto result = egg::Parser({eof, eof}).parse();

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));
    const auto &error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::InvalidTokenStream);
    REQUIRE(error.actual);
    CHECK(*error.actual == egg::TokenKind::EndOfFile);
    CHECK(error.range.begin.byte_offset == 0);
    CHECK(error.range.end.byte_offset == 0);
}

TEST_CASE("EOF в середине потока указывает на преждевременный EOF",
          "[parser][core]") {
    const auto eof = make_token(egg::TokenKind::EndOfFile, 0, 0);
    const auto id = make_token(egg::TokenKind::Identifier, 0, 1, "x");
    const auto result = egg::Parser({eof, id, eof}).parse();

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));
    const auto &error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::InvalidTokenStream);
    REQUIRE(error.actual);
    CHECK(*error.actual == egg::TokenKind::EndOfFile);
    CHECK(error.range.begin.byte_offset == 0);
    CHECK(error.range.end.byte_offset == 0);
}

TEST_CASE("Синтаксическая диагностика содержит имя источника и позицию",
          "[parser][core]") {
    const auto result = parse("int", "sample.egg");

    REQUIRE(std::holds_alternative<egg::ParseDiagnostic>(result));
    const auto &error = diagnostic(result);
    CHECK(error.code == egg::ParseErrorCode::UnexpectedToken);
    CHECK(error.source_name == "sample.egg");
    REQUIRE(error.actual);
    CHECK(*error.actual == egg::TokenKind::KwInt);
    CHECK(error.range.begin.byte_offset == 0);
    CHECK(error.range.begin.line == 1);
    CHECK(error.range.begin.column == 1);
}

TEST_CASE("Результат разбора живёт после уничтожения Parser",
          "[parser][core]") {
    egg::ParseResult result = [&] {
        auto lexed = egg::Lexer("").tokenize();
        egg::Parser parser(std::move(lexed.tokens), "owned.egg");
        return parser.parse();
    }();

    const auto *program = std::get_if<egg::ast::Program>(&result);
    REQUIRE(program);
    CHECK(program->source_name == "owned.egg");
    CHECK(program->declarations.empty());
}
