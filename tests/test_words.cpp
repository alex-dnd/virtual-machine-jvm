#include "egg/lexer.h"

#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <string>
#include <utility>
#include <vector>

TEST_CASE("All 17 keywords are recognized", "[words]") {
    const std::vector<std::pair<std::string, egg::TokenKind>> keywords = {
        {"class", egg::TokenKind::KwClass},
        {"constructor", egg::TokenKind::KwConstructor},
        {"fn", egg::TokenKind::KwFn},
        {"int", egg::TokenKind::KwInt},
        {"bool", egg::TokenKind::KwBool},
        {"char", egg::TokenKind::KwChar},
        {"vector", egg::TokenKind::KwVector},
        {"void", egg::TokenKind::KwVoid},
        {"if", egg::TokenKind::KwIf},
        {"else", egg::TokenKind::KwElse},
        {"while", egg::TokenKind::KwWhile},
        {"break", egg::TokenKind::KwBreak},
        {"continue", egg::TokenKind::KwContinue},
        {"return", egg::TokenKind::KwReturn},
        {"this", egg::TokenKind::KwThis},
        {"true", egg::TokenKind::KwTrue},
        {"false", egg::TokenKind::KwFalse}
    };

    for (const auto &keyword : keywords) {
        INFO("keyword: " << keyword.first);
        auto result = egg::Lexer(keyword.first, "test.egg").tokenize();
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE(result.tokens.size() == 2);
        CHECK(result.tokens[0].kind == keyword.second);
        CHECK(result.tokens[0].lexeme == keyword.first);
        CHECK(result.tokens[1].kind == egg::TokenKind::EndOfFile);
    }
}

TEST_CASE("Identifiers are not confused with keywords", "[words]") {
    const std::vector<std::string> words = {
        "main", "print", "size", "True", "FALSE", "If",
        "_", "_tmp9", "my_var", "fn2", "true_", "className"
    };

    for (const auto &word : words) {
        INFO("identifier: " << word);
        auto result = egg::Lexer(word, "test.egg").tokenize();
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE(result.tokens.size() == 2);
        CHECK(result.tokens[0].kind == egg::TokenKind::Identifier);
        CHECK(result.tokens[0].lexeme == word);
    }
}

TEST_CASE("Integer literals preserve their source spelling", "[words]") {
    const std::vector<std::string> numbers = {
        "0", "42", "007", "9223372036854775808",
        std::string(100, '9')
    };

    for (const auto &number : numbers) {
        INFO("integer: " << number);
        auto result = egg::Lexer(number, "test.egg").tokenize();
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE(result.tokens.size() == 2);
        CHECK(result.tokens[0].kind == egg::TokenKind::IntegerLiteral);
        CHECK(result.tokens[0].lexeme == number);
    }
}

TEST_CASE("Letter or underscore after digits is an error", "[words]") {
    const std::vector<std::string> invalid = {
        "12abc", "0xFF", "42_", "123_a9", "3a2b"
    };

    for (const auto &source : invalid) {
        INFO("invalid integer: " << source);
        auto result = egg::Lexer(source, "test.egg").tokenize();
        REQUIRE(result.error.has_value());
        CHECK(result.error->code == egg::LexErrorCode::InvalidIntegerSuffix);
        CHECK(result.error->range.begin.byte_offset == 0);
        CHECK(result.error->range.end.byte_offset == source.size());
    }
}

TEST_CASE("All 25 punctuation tokens are recognized", "[words]") {
    const std::vector<std::pair<std::string, egg::TokenKind>> punct = {
        {"{", egg::TokenKind::LeftBrace},
        {"}", egg::TokenKind::RightBrace},
        {"(", egg::TokenKind::LeftParen},
        {")", egg::TokenKind::RightParen},
        {"[", egg::TokenKind::LeftBracket},
        {"]", egg::TokenKind::RightBracket},
        {"<", egg::TokenKind::Less},
        {">", egg::TokenKind::Greater},
        {",", egg::TokenKind::Comma},
        {";", egg::TokenKind::Semicolon},
        {".", egg::TokenKind::Dot},
        {"=", egg::TokenKind::Assign},
        {"+", egg::TokenKind::Plus},
        {"-", egg::TokenKind::Minus},
        {"*", egg::TokenKind::Star},
        {"/", egg::TokenKind::Slash},
        {"%", egg::TokenKind::Percent},
        {"!", egg::TokenKind::Bang},
        {"->", egg::TokenKind::Arrow},
        {"==", egg::TokenKind::Equal},
        {"!=", egg::TokenKind::NotEqual},
        {"<=", egg::TokenKind::LessEqual},
        {">=", egg::TokenKind::GreaterEqual},
        {"&&", egg::TokenKind::LogicalAnd},
        {"||", egg::TokenKind::LogicalOr}
    };

    for (const auto &item : punct) {
        INFO("punctuation: " << item.first);
        auto result = egg::Lexer(item.first, "test.egg").tokenize();
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE(result.tokens.size() == 2);
        CHECK(result.tokens[0].kind == item.second);
        CHECK(result.tokens[0].lexeme == item.first);
    }
}

TEST_CASE("Two-character operators take priority", "[words]") {
    auto result = egg::Lexer(
        "a->b==c!=d<=e>=f&&g||h >> -- !!", "test.egg"
    ).tokenize();
    REQUIRE_FALSE(result.error.has_value());

    const std::vector<std::pair<std::string, egg::TokenKind>> expected = {
        {"a", egg::TokenKind::Identifier},
        {"->", egg::TokenKind::Arrow},
        {"b", egg::TokenKind::Identifier},
        {"==", egg::TokenKind::Equal},
        {"c", egg::TokenKind::Identifier},
        {"!=", egg::TokenKind::NotEqual},
        {"d", egg::TokenKind::Identifier},
        {"<=", egg::TokenKind::LessEqual},
        {"e", egg::TokenKind::Identifier},
        {">=", egg::TokenKind::GreaterEqual},
        {"f", egg::TokenKind::Identifier},
        {"&&", egg::TokenKind::LogicalAnd},
        {"g", egg::TokenKind::Identifier},
        {"||", egg::TokenKind::LogicalOr},
        {"h", egg::TokenKind::Identifier},
        {">", egg::TokenKind::Greater},
        {">", egg::TokenKind::Greater},
        {"-", egg::TokenKind::Minus},
        {"-", egg::TokenKind::Minus},
        {"!", egg::TokenKind::Bang},
        {"!", egg::TokenKind::Bang}
    };

    REQUIRE(result.tokens.size() == expected.size() + 1);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        INFO("token index: " << i);
        CHECK(result.tokens[i].kind == expected[i].second);
        CHECK(result.tokens[i].lexeme == expected[i].first);
    }
    CHECK(result.tokens.back().kind == egg::TokenKind::EndOfFile);
}

TEST_CASE("Unknown ASCII characters are rejected", "[words]") {
    const std::vector<std::string> invalid = {
        "@", "#", "$", "?", "~", "^", ":", "\\",
        "&", "|", std::string(1, '\0')
    };

    for (const auto &source : invalid) {
        auto result = egg::Lexer(source, "test.egg").tokenize();
        REQUIRE(result.error.has_value());
        CHECK(result.error->code == egg::LexErrorCode::UnexpectedCharacter);
        CHECK(result.error->range.begin.byte_offset == 0);
        CHECK(result.error->range.end.byte_offset == 1);
    }
}

TEST_CASE("Trivia is skipped and source positions are retained", "[words]") {
    auto result = egg::Lexer("fn\tmain // comment\n42", "test.egg").tokenize();
    REQUIRE_FALSE(result.error.has_value());
    REQUIRE(result.tokens.size() == 4);
    CHECK(result.tokens[0].lexeme == "fn");
    CHECK(result.tokens[1].lexeme == "main");
    CHECK(result.tokens[2].lexeme == "42");
    CHECK(result.tokens[2].range.begin.line == 2);
    CHECK(result.tokens[2].range.begin.column == 1);
}

TEST_CASE("Non-ASCII letter after an integer is not its suffix", "[words]") {
    auto result = egg::Lexer("12\xD0\xAF", "test.egg").tokenize();
    REQUIRE(result.error.has_value());
    REQUIRE(result.tokens.size() == 1);
    CHECK(result.tokens[0].kind == egg::TokenKind::IntegerLiteral);
    CHECK(result.tokens[0].lexeme == "12");
    CHECK(result.error->code == egg::LexErrorCode::UnexpectedCharacter);
    CHECK(result.error->range.begin.byte_offset == 2);
}
