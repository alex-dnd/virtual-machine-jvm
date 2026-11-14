#include "scanner.h"

#include <string>
#include <utility>

namespace egg::detail {
namespace {

bool is_ascii_letter(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_digit(int c) {
    return c >= '0' && c <= '9';
}

bool is_identifier_char(int c) {
    return is_ascii_letter(c) || is_digit(c) || c == '_';
}

} // namespace

bool Scanner::scan_identifier_or_keyword() {
    SourcePosition begin = here();

    while (is_identifier_char(peek())) {
        advance_ascii();
    }

    std::string word = src_.substr(begin.byte_offset, pos_ - begin.byte_offset);

    static const std::pair<const char *, TokenKind> kKeywords[] = {
        {"class", TokenKind::KwClass},
        {"constructor", TokenKind::KwConstructor},
        {"fn", TokenKind::KwFn},
        {"int", TokenKind::KwInt},
        {"bool", TokenKind::KwBool},
        {"char", TokenKind::KwChar},
        {"vector", TokenKind::KwVector},
        {"void", TokenKind::KwVoid},
        {"if", TokenKind::KwIf},
        {"else", TokenKind::KwElse},
        {"while", TokenKind::KwWhile},
        {"break", TokenKind::KwBreak},
        {"continue", TokenKind::KwContinue},
        {"return", TokenKind::KwReturn},
        {"this", TokenKind::KwThis},
        {"true", TokenKind::KwTrue},
        {"false", TokenKind::KwFalse}
    };

    TokenKind kind = TokenKind::Identifier;
    for (const auto &keyword : kKeywords) {
        if (word == keyword.first) {
            kind = keyword.second;
            break;
        }
    }

    add_token(kind, begin);
    return true;
}

bool Scanner::scan_integer() {
    SourcePosition begin = here();

    while (is_digit(peek())) {
        advance_ascii();
    }

    if (is_ascii_letter(peek()) || peek() == '_') {
        while (is_identifier_char(peek())) {
            advance_ascii();
        }
        return fail(LexErrorCode::InvalidIntegerSuffix, begin, here(),
                    "letter after digits");
    }

    add_token(TokenKind::IntegerLiteral, begin);
    return true;
}

bool Scanner::scan_punct() {
    SourcePosition begin = here();

    static const std::pair<const char *, TokenKind> kDoublePunct[] = {
        {"->", TokenKind::Arrow},
        {"==", TokenKind::Equal},
        {"!=", TokenKind::NotEqual},
        {"<=", TokenKind::LessEqual},
        {">=", TokenKind::GreaterEqual},
        {"&&", TokenKind::LogicalAnd},
        {"||", TokenKind::LogicalOr}
    };

    for (const auto &punct : kDoublePunct) {
        if (peek() == punct.first[0] && peek(1) == punct.first[1]) {
            advance_ascii();
            advance_ascii();
            add_token(punct.second, begin);
            return true;
        }
    }

    TokenKind kind;
    switch (peek()) {
    case '{': kind = TokenKind::LeftBrace; break;
    case '}': kind = TokenKind::RightBrace; break;
    case '(': kind = TokenKind::LeftParen; break;
    case ')': kind = TokenKind::RightParen; break;
    case '[': kind = TokenKind::LeftBracket; break;
    case ']': kind = TokenKind::RightBracket; break;
    case '<': kind = TokenKind::Less; break;
    case '>': kind = TokenKind::Greater; break;
    case ',': kind = TokenKind::Comma; break;
    case ';': kind = TokenKind::Semicolon; break;
    case '.': kind = TokenKind::Dot; break;
    case '=': kind = TokenKind::Assign; break;
    case '+': kind = TokenKind::Plus; break;
    case '-': kind = TokenKind::Minus; break;
    case '*': kind = TokenKind::Star; break;
    case '/': kind = TokenKind::Slash; break;
    case '%': kind = TokenKind::Percent; break;
    case '!': kind = TokenKind::Bang; break;
    default:
        return fail(LexErrorCode::UnexpectedCharacter, begin, one_past_here(),
                    "unexpected character");
    }

    advance_ascii();
    add_token(kind, begin);
    return true;
}

} // namespace egg::detail
