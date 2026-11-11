#include "scanner.h"

#include <utility>

namespace egg::detail {
namespace {
inline bool is_ascii_letter(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

inline bool is_digit(int c) { return c >= '0' && c <= '9'; }

} // namespace

void Scanner::add_token(TokenKind kind, SourcePosition begin, TokenValue value) {
    Token t;
    t.kind = kind;
    t.range = SourceRange{begin, here()};
    t.lexeme = src_.substr(begin.byte_offset, pos_ - begin.byte_offset);
    t.value = std::move(value);
    tokens_.push_back(std::move(t));
}

bool Scanner::fail(LexErrorCode code, SourcePosition begin, SourcePosition end,
                   std::string message) {
    if (!error_) {
        error_ = LexDiagnostic{code, name_, SourceRange{begin, end}, std::move(message)};
    }
    return false;
}

LexResult Scanner::run() {
    skip_leading_bom();
    while (true) {
        if (!skip_trivia())
            break;
        if (at_end()) {
            SourcePosition end = here();
            Token eof;
            eof.kind = TokenKind::EndOfFile;
            eof.range = SourceRange{end, end};
            eof.value = std::monostate{};
            tokens_.push_back(std::move(eof));
            break;
        }
        int c = peek();
        bool ok = false;
        if (is_ascii_letter(c) || c == '_') {
            ok = scan_identifier_or_keyword();
        } else if (is_digit(c)) {
            ok = scan_integer();
        } else if (c == '\'') {
            ok = scan_char_literal();
        } else if (c == '"') {
            ok = scan_text_literal();
        } else if (c >= 0x80) {
            ok = fail_non_ascii();
        } else {
            ok = scan_punct();
        }
        if (!ok)
            break;
    }
    return LexResult{std::move(tokens_), std::move(error_)};
}

} // namespace egg::detail
