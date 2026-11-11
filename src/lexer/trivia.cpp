#include "scanner.h"

namespace egg::detail {

std::size_t Scanner::utf8_length_at(std::size_t offset) const {
    if (offset >= src_.size())
        return 0;

    auto b = [&](std::size_t i) -> int {
        if (offset + i >= src_.size())
            return -1;
        return static_cast<unsigned char>(src_[offset + i]);
    };

    int b0 = b(0);
    if (b0 < 0x80)
        return 1;

    if (b0 >= 0xC2 && b0 <= 0xDF) {
        int b1 = b(1);
        return (b1 >= 0x80 && b1 <= 0xBF) ? 2 : 0;
    }
    if (b0 == 0xE0) {
        int b1 = b(1), b2 = b(2);
        return (b1 >= 0xA0 && b1 <= 0xBF && b2 >= 0x80 && b2 <= 0xBF) ? 3 : 0;
    }
    if ((b0 >= 0xE1 && b0 <= 0xEC) || b0 == 0xEE || b0 == 0xEF) {
        int b1 = b(1), b2 = b(2);
        return (b1 >= 0x80 && b1 <= 0xBF && b2 >= 0x80 && b2 <= 0xBF) ? 3 : 0;
    }
    if (b0 == 0xED) {
        int b1 = b(1), b2 = b(2);
        return (b1 >= 0x80 && b1 <= 0x9F && b2 >= 0x80 && b2 <= 0xBF) ? 3 : 0;
    }
    if (b0 == 0xF0) {
        int b1 = b(1), b2 = b(2), b3 = b(3);
        return (b1 >= 0x90 && b1 <= 0xBF && b2 >= 0x80 && b2 <= 0xBF && b3 >= 0x80 && b3 <= 0xBF)
                   ? 4
                   : 0;
    }
    if (b0 >= 0xF1 && b0 <= 0xF3) {
        int b1 = b(1), b2 = b(2), b3 = b(3);
        return (b1 >= 0x80 && b1 <= 0xBF && b2 >= 0x80 && b2 <= 0xBF && b3 >= 0x80 && b3 <= 0xBF)
                   ? 4
                   : 0;
    }
    if (b0 == 0xF4) {
        int b1 = b(1), b2 = b(2), b3 = b(3);
        return (b1 >= 0x80 && b1 <= 0x8F && b2 >= 0x80 && b2 <= 0xBF && b3 >= 0x80 && b3 <= 0xBF)
                   ? 4
                   : 0;
    }

    return 0; // 0x80..0xC1, 0xF5..0xFF
}

bool Scanner::is_bom_at(std::size_t offset) const {
    return src_.compare(offset, 3, "\xEF\xBB\xBF") == 0;
}

void Scanner::advance_code_point(std::size_t len) {
    pos_ += len;
    ++column_;
}

void Scanner::advance_newline() {
    if (peek() == '\r') {
        pos_ += 2;
    } else {
        pos_ += 1;
    }
    ++line_;
    column_ = 1;
}

void Scanner::skip_leading_bom() {
    if (is_bom_at(0))
        pos_ = 3;
}

bool Scanner::skip_trivia() {
    while (true) {
        int c = peek();
        if (c == ' ' || c == '\t') {
            advance_ascii();
        } else if (c == '\n') {
            advance_newline();
        } else if (c == '\r') {
            if (peek(1) == '\n') {
                advance_newline();
            } else {
                return fail(LexErrorCode::BareCarriageReturn, here(), one_past_here(),
                            "CR without LF");
            }
        } else if (c == '/' && peek(1) == '/') {
            advance_ascii(); // первый /
            advance_ascii(); // второй /
            while (true) {
                int d = peek();
                if (d == kEnd || d == '\n' || d == '\r')
                    break;
                if (d < 0x80) {
                    advance_ascii();
                } else if (is_bom_at(pos_)) {
                    SourcePosition end{pos_ + 3, line_, column_ + 1};
                    return fail(LexErrorCode::UnexpectedBom, here(), end, "BOM inside comment");
                } else {
                    std::size_t len = utf8_length_at(pos_);
                    if (len == 0) {
                        return fail(LexErrorCode::InvalidUtf8, here(), one_past_here(),
                                    "invalid UTF-8 in comment");
                    }
                    advance_code_point(len);
                }
            }
        } else {
            return true; // начинается токен
        }
    }
}

bool Scanner::fail_non_ascii() {
    if (is_bom_at(pos_)) {
        SourcePosition end{pos_ + 3, line_, column_ + 1};
        return fail(LexErrorCode::UnexpectedBom, here(), end, "unexpected BOM");
    }

    std::size_t len = utf8_length_at(pos_);
    if (len == 0) {
        return fail(LexErrorCode::InvalidUtf8, here(), one_past_here(), "invalid UTF-8");
    }

    SourcePosition end{pos_ + len, line_, column_ + 1};
    return fail(LexErrorCode::UnexpectedCharacter, here(), end, "unexpected non-ASCII character");
}

} // namespace egg::detail
