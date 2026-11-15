#include <catch2/catch_test_macros.hpp>
#include "helpers.h"

using egg::TokenKind;

TEST_CASE("trivia placeholder", "[trivia]") {
    auto r = test::lex("");
    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(test::kinds(r) == std::vector<TokenKind>{TokenKind::EndOfFile});
}

TEST_CASE("пустой файл: один EOF в позиции 1:1", "[trivia]") {
    auto r = test::lex("");
    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(r.tokens.size() == 1);
    CHECK(r.tokens[0].kind == TokenKind::EndOfFile);
    CHECK(r.tokens[0].range.begin.byte_offset == 0);
    CHECK(r.tokens[0].range.begin.line == 1);
    CHECK(r.tokens[0].range.begin.column == 1);
}

TEST_CASE("пробелы и табуляция: каждый занимает один столбец", "[trivia]") {
    auto r = test::lex("  \t");
    REQUIRE_FALSE(r.error.has_value());
    auto eof = r.tokens.back().range.begin;
    CHECK(eof.byte_offset == 3);
    CHECK(eof.line == 1);
    CHECK(eof.column == 4);
}

TEST_CASE("LF и CRLF: каждый — один перенос строки", "[trivia]") {
    auto r = test::lex("\n\r\n");
    REQUIRE_FALSE(r.error.has_value());
    auto eof = r.tokens.back().range.begin;
    CHECK(eof.byte_offset == 3);
    CHECK(eof.line == 3);
    CHECK(eof.column == 1);
}

TEST_CASE("CR без LF — ошибка BareCarriageReturn", "[trivia]") {
    auto r = test::lex(" \r ");
    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::BareCarriageReturn);
    CHECK(r.error->range.begin.byte_offset == 1);
    CHECK(r.error->range.begin.column == 2);
    CHECK(r.tokens.empty());  // при ошибке EOF не добавляется
}

TEST_CASE("комментарий с русским текстом, столбцы в символах", "[trivia]") {
    // "// " — 3 байта, "Привет" — 6 букв по 2 байта = 12 байт
    auto r = test::lex("// Привет");
    REQUIRE_FALSE(r.error.has_value());
    auto eof = r.tokens.back().range.begin;
    CHECK(eof.byte_offset == 15);
    CHECK(eof.column == 10);   // 3 + 6 символов + 1
}

TEST_CASE("BOM в начале: входит в byte_offset, но не в столбец", "[trivia]") {
    auto r = test::lex("\xEF\xBB\xBF");
    REQUIRE_FALSE(r.error.has_value());
    auto eof = r.tokens.back().range.begin;
    CHECK(eof.byte_offset == 3);
    CHECK(eof.column == 1);
}

TEST_CASE("BOM не в начале — UnexpectedBom", "[trivia]") {
    auto r = test::lex("  \xEF\xBB\xBF");
    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::UnexpectedBom);
    CHECK(r.error->range.begin.byte_offset == 2);

    auto r2 = test::lex("// \xEF\xBB\xBF");   // и в комментарии тоже
    REQUIRE(r2.error.has_value());
    CHECK(r2.error->code == egg::LexErrorCode::UnexpectedBom);
}

TEST_CASE("битый UTF-8 в комментарии — InvalidUtf8 на первом плохом байте", "[trivia]") {
    auto r = test::lex("// \xFF");
    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::InvalidUtf8);
    CHECK(r.error->range.begin.byte_offset == 3);
    CHECK(r.error->range.begin.column == 4);
    CHECK(r.error->range.end.byte_offset == 4);
    CHECK(r.error->range.end.column == 5);

    CHECK(test::lex("// \xC3").error->code == egg::LexErrorCode::InvalidUtf8);          // оборван в конце
    CHECK(test::lex("// \xC0\xAF").error->code == egg::LexErrorCode::InvalidUtf8);      // overlong
    CHECK(test::lex("// \xED\xA0\x80").error->code == egg::LexErrorCode::InvalidUtf8);  // суррогат
}

TEST_CASE("не-ASCII вне комментария — UnexpectedCharacter", "[trivia]") {
    auto r = test::lex("Я");
    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::UnexpectedCharacter);
    CHECK(r.error->range.end.byte_offset == 2);
    CHECK(r.error->range.end.column == 2);
}


