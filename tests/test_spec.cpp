#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "helpers.h"


TEST_CASE("empty source produces one EOF", "[spec]"){
    auto r = test::lex("");

    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(r.tokens.size() == 1);

    const auto& eof = r.tokens[0];

    CHECK(eof.kind == egg::TokenKind::EndOfFile);
    CHECK(eof.lexeme.empty());
    CHECK(eof.range.begin.line == 1);
    CHECK(eof.range.begin.column == 1);
    CHECK(eof.range.begin.byte_offset == 0);
    CHECK(eof.range.end.line == 1);
    CHECK(eof.range.end.column == 1);
    CHECK(eof.range.end.byte_offset == 0);
}


TEST_CASE("whitespace is skipped and does not create tokens", "[spec]"){
    auto r = test::lex("   \t\n\r\n  int");

    REQUIRE_FALSE(r.error.has_value());
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{egg::TokenKind::KwInt, egg::TokenKind::EndOfFile});
    CHECK(test::lexemes(r) == std::vector<std::string>{"int", ""});
}


TEST_CASE("line comment is skipped", "[spec]"){
    auto r = test::lex("// comment\nint");

    REQUIRE_FALSE(r.error.has_value());
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{egg::TokenKind::KwInt, egg::TokenKind::EndOfFile});
}


TEST_CASE("leading BOM is skipped", "[spec]"){
    const std::string source = "\xEF\xBB\xBFint";

    auto r = test::lex(source);

    REQUIRE_FALSE(r.error.has_value());
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{egg::TokenKind::KwInt, egg::TokenKind::EndOfFile});
}


TEST_CASE("CRLF advances one line", "[spec]"){
    auto r = test::lex("int\r\nint");

    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(r.tokens.size() == 3);

    CHECK(r.tokens[0].range.begin.line == 1);
    CHECK(r.tokens[1].range.begin.line == 2);
    CHECK(r.tokens[1].range.begin.column == 1);
}


TEST_CASE("tab occupies one column", "[spec]"){
    auto r = test::lex("\tint");

    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(r.tokens.size() == 2);

    CHECK(r.tokens[0].range.begin.line == 1);
    CHECK(r.tokens[0].range.begin.column == 2);
}


TEST_CASE("cyrillic identifier character occupies one column", "[spec]"){
    auto r = test::lex("Яint");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::UnexpectedCharacter);
    CHECK(r.error->range.begin.line == 1);
    CHECK(r.error->range.begin.column == 1);
}


TEST_CASE("integer followed by identifier is invalid", "[spec]"){
    auto r = test::lex("12abc");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::InvalidIntegerSuffix);
    CHECK(r.error->source_name == "<memory>");
    CHECK(r.tokens.empty());
}


TEST_CASE("integer token keeps source lexeme", "[spec]"){
    auto r = test::lex("123");

    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(r.tokens.size() == 2);

    CHECK(r.tokens[0].kind == egg::TokenKind::IntegerLiteral);
    CHECK(r.tokens[0].lexeme == "123");
    CHECK(r.tokens[1].kind == egg::TokenKind::EndOfFile);
}


TEST_CASE("identifier is recognized", "[spec]"){
    auto r = test::lex("count");

    REQUIRE_FALSE(r.error.has_value());
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{
        egg::TokenKind::Identifier,
        egg::TokenKind::EndOfFile
    });
    CHECK(test::lexemes(r) == std::vector<std::string>{"count", ""});
}


TEST_CASE("keywords have dedicated token kinds", "[spec]"){
    auto r = test::lex("class fn int bool char vector void");

    REQUIRE_FALSE(r.error.has_value());
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{
        egg::TokenKind::KwClass,
        egg::TokenKind::KwFn,
        egg::TokenKind::KwInt,
        egg::TokenKind::KwBool,
        egg::TokenKind::KwChar,
        egg::TokenKind::KwVector,
        egg::TokenKind::KwVoid,
        egg::TokenKind::EndOfFile
    });
}


TEST_CASE("longest punctuation is selected", "[spec]"){
    auto r = test::lex("-> == != <= >= && ||");

    REQUIRE_FALSE(r.error.has_value());
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{
        egg::TokenKind::Arrow,
        egg::TokenKind::Equal,
        egg::TokenKind::NotEqual,
        egg::TokenKind::LessEqual,
        egg::TokenKind::GreaterEqual,
        egg::TokenKind::LogicalAnd,
        egg::TokenKind::LogicalOr,
        egg::TokenKind::EndOfFile
    });
}


TEST_CASE("single ampersand is unexpected", "[spec]"){
    auto r = test::lex("&");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::UnexpectedCharacter);
}


TEST_CASE("two minus signs remain separate tokens", "[spec]"){
    auto r = test::lex("1--2");

    REQUIRE_FALSE(r.error.has_value());
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{
        egg::TokenKind::IntegerLiteral,
        egg::TokenKind::Minus,
        egg::TokenKind::Minus,
        egg::TokenKind::IntegerLiteral,
        egg::TokenKind::EndOfFile
    });
}


TEST_CASE("char literal decodes ASCII value", "[spec]"){
    auto r = test::lex("'A'");

    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(r.tokens.size() == 2);

    CHECK(r.tokens[0].kind == egg::TokenKind::CharLiteral);
    CHECK(std::get<egg::CharacterValue>(r.tokens[0].value).ascii == 'A');
}


TEST_CASE("multi-character char literal is invalid", "[spec]"){
    auto r = test::lex("'ab'");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::InvalidCharacterLength);
}


TEST_CASE("non-ASCII char literal is invalid", "[spec]"){
    auto r = test::lex("'Я'");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::NonAsciiInLiteral);
}


TEST_CASE("non-printable ASCII char requires escape", "[spec]"){
    auto r = test::lex("'\x7F'");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::NonPrintableInLiteral);
}


TEST_CASE("unknown escape is invalid", "[spec]"){
    auto r = test::lex("'\\q'");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::InvalidEscape);
}


TEST_CASE("incomplete hex escape is invalid", "[spec]"){
    auto r = test::lex("'\\x7'");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::InvalidHexEscape);
}


TEST_CASE("hex escape above ASCII range is invalid", "[spec]"){
    auto r = test::lex("'\\x80'");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::EscapeOutOfAsciiRange);
}


TEST_CASE("unterminated text literal is invalid", "[spec]"){
    auto r = test::lex("\"abc");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::UnterminatedLiteral);
}


TEST_CASE("physical newline inside text literal is invalid", "[spec]"){
    auto r = test::lex("\"a\nb\"");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::NewlineInLiteral);
}


TEST_CASE("text literal decodes escapes", "[spec]"){
    auto r = test::lex("\"a\\n\\t\\\"\\\\\"");

    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(r.tokens.size() == 2);

    CHECK(r.tokens[0].kind == egg::TokenKind::TextLiteral);
    CHECK(std::get<egg::TextValue>(r.tokens[0].value).ascii == "a\n\t\"\\");
}


TEST_CASE("text literal with cyrillic is invalid", "[spec]"){
    auto r = test::lex("\"Привет\"");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::NonAsciiInLiteral);
}


TEST_CASE("BOM outside file start is invalid", "[spec]"){
    const std::string source = "int\xEF\xBB\xBF";

    auto r = test::lex(source);

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::UnexpectedBom);
}


TEST_CASE("bare carriage return is invalid", "[spec]"){
    auto r = test::lex("int\rint");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::BareCarriageReturn);
}


TEST_CASE("invalid UTF-8 byte is reported", "[spec]"){
    const std::string source = "// \xFF\nint";

    auto r = test::lex(source);

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::InvalidUtf8);
}


TEST_CASE("error preserves tokens before failure and omits EOF", "[spec]"){
    auto r = test::lex("int 12abc");

    REQUIRE(r.error.has_value());
    CHECK(r.error->code == egg::LexErrorCode::InvalidIntegerSuffix);

    REQUIRE_FALSE(r.tokens.empty());
    CHECK(r.tokens.back().kind != egg::TokenKind::EndOfFile);
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{
        egg::TokenKind::KwInt
    });
}


TEST_CASE("repeated tokenization returns the same result", "[spec]"){
    egg::Lexer lexer("int x = 1;", "test.egg");

    auto first = lexer.tokenize();
    auto second = lexer.tokenize();

    REQUIRE_FALSE(first.error.has_value());
    REQUIRE_FALSE(second.error.has_value());
    CHECK(test::kinds(first) == test::kinds(second));
    CHECK(test::lexemes(first) == test::lexemes(second));
}


TEST_CASE("small program is tokenized", "[spec]"){
    const std::string source =
        "// Программа\n"
        "int x = 1;\n"
        "print(x);\n";

    auto r = test::lex(source);

    REQUIRE_FALSE(r.error.has_value());
    CHECK(test::kinds(r) == std::vector<egg::TokenKind>{
        egg::TokenKind::KwInt,
        egg::TokenKind::Identifier,
        egg::TokenKind::Assign,
        egg::TokenKind::IntegerLiteral,
        egg::TokenKind::Semicolon,
        egg::TokenKind::Identifier,
        egg::TokenKind::LeftParen,
        egg::TokenKind::Identifier,
        egg::TokenKind::RightParen,
        egg::TokenKind::Semicolon,
        egg::TokenKind::EndOfFile
    });
}