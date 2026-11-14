
#include <catch2/catch_test_macros.hpp>
#include "helpers.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

using namespace egg;

TEST_CASE("Символьные литералы", "[literals]") {
    struct Case {
        std::string source;
        std::uint8_t expected;
    };

    const std::vector<Case> cases = {
        {"'a'", 'a'},
        {"' '", ' '},
        {R"('\n')", 10},
        {R"('\r')", 13},
        {R"('\t')", 9},
        {R"('\0')", 0},
        {R"('\\')", '\\'},
        {R"('\'')", '\''},
        {R"('\"')", '"'},
        {R"('\x41')", 65},
        {R"('\x7F')", 127}
    };

    for (const auto& tc : cases) {
        INFO(tc.source);

        auto r = test::lex(tc.source);

        REQUIRE_FALSE(r.error.has_value());
        REQUIRE(r.tokens.size() == 2);

        const auto& token = r.tokens[0];

        CHECK(token.kind == TokenKind::CharLiteral);
        CHECK(token.lexeme == tc.source);
        CHECK(token.range.begin.byte_offset == 0);
        CHECK(token.range.end.byte_offset == tc.source.size());

        const auto* value = std::get_if<CharacterValue>(&token.value);
        REQUIRE(value != nullptr);
        CHECK(value->ascii == tc.expected);

        CHECK(r.tokens[1].kind == TokenKind::EndOfFile);
    }
}

TEST_CASE("Текстовые литералы", "[literals]") {
    struct Case {
        std::string source;
        std::string expected;
    };

    const std::vector<Case> cases = {
        {R"("")", ""},
        {R"("Hello")", "Hello"},
        {R"("Hello\nWorld")", "Hello\nWorld"},
        {R"("\x41F")", "AF"},
        {R"("a\0b")", std::string("a\0b", 3)},
        {R"("it's \"ok\"")", "it's \"ok\""},
        {R"("'")", "'"}
    };

    for (const auto& tc : cases) {
        INFO(tc.source);

        auto r = test::lex(tc.source);

        REQUIRE_FALSE(r.error.has_value());
        REQUIRE(r.tokens.size() == 2);

        const auto& token = r.tokens[0];

        CHECK(token.kind == TokenKind::TextLiteral);
        CHECK(token.lexeme == tc.source);
        CHECK(token.range.end.byte_offset == tc.source.size());

        const auto* value = std::get_if<TextValue>(&token.value);
        REQUIRE(value != nullptr);
        CHECK(value->ascii == tc.expected);

        CHECK(r.tokens[1].kind == TokenKind::EndOfFile);
    }
}

TEST_CASE("Ошибки литералов", "[literals]") {
    struct Case {
        std::string source;
        LexErrorCode expected;
        std::size_t begin;
        std::size_t end;
    };

    const std::vector<Case> cases = {
        {"\"abc", LexErrorCode::UnterminatedLiteral, 0, 4},

        {"\"a\n\"", LexErrorCode::NewlineInLiteral, 2, 3},
        {"\"a\r\"", LexErrorCode::NewlineInLiteral, 2, 3},

        {"\"" + std::string("\xD0\xAF", 2) + "\"",
         LexErrorCode::NonAsciiInLiteral, 1, 3},

        {"\"" + std::string(1, '\x01') + "\"",
         LexErrorCode::NonPrintableInLiteral, 1, 2},

        {R"("\q")", LexErrorCode::InvalidEscape, 1, 3},
        {"\"\\", LexErrorCode::InvalidEscape, 1, 2},

        {R"("\xG1")", LexErrorCode::InvalidHexEscape, 1, 3},

        {R"("\xFF")", LexErrorCode::EscapeOutOfAsciiRange, 1, 5},

        {"''", LexErrorCode::InvalidCharacterLength, 0, 2},
        {"'ab'", LexErrorCode::InvalidCharacterLength, 0, 4}
    };

    for (const auto& tc : cases) {
        INFO(tc.source);

        auto r = test::lex(tc.source);

        REQUIRE(r.error.has_value());
        CHECK(r.error->code == tc.expected);
        CHECK(r.error->range.begin.byte_offset == tc.begin);
        CHECK(r.error->range.end.byte_offset == tc.end);
        CHECK(r.tokens.empty());
    }
}

TEST_CASE("BOM и неверный UTF-8 внутри литерала", "[literals]") {
    auto bom = test::lex(
        "\"" + std::string("\xEF\xBB\xBF", 3) + "\""
    );

    REQUIRE(bom.error.has_value());
    CHECK(bom.error->code == LexErrorCode::UnexpectedBom);

    auto invalid = test::lex(
        "\"" + std::string(1, static_cast<char>(0xFF)) + "\""
    );

    REQUIRE(invalid.error.has_value());
    CHECK(invalid.error->code == LexErrorCode::InvalidUtf8);
}
