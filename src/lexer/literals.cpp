#include "scanner.h"

namespace egg::detail {

namespace {
int hex_value(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}
} // namespace

bool Scanner::scan_char_literal() {
    SourcePosition begin = here();
    std::string decoded;

    if (!scan_quoted('\'', begin, decoded))
        return false;

    if (decoded.size() != 1) {
        return fail(LexErrorCode::InvalidCharacterLength, begin, here(),
                    "Character literal must contain exactly one character");
    }

    add_token(TokenKind::CharLiteral, begin, CharacterValue{static_cast<std::uint8_t>(decoded[0])});

    return true;
}

bool Scanner::scan_text_literal() {
    SourcePosition begin = here();
    std::string decoded;

    if (!scan_quoted('"', begin, decoded))
        return false;

    add_token(TokenKind::TextLiteral, begin, TextValue{decoded});

    return true;
}

bool Scanner::validate_literal_character() {
    int c = peek();
    if (c == '\n' || c == '\r') {
        return fail(LexErrorCode::NewlineInLiteral, here(), one_past_here(),
                    "Newline inside literal");
    }

    if (c >= 0x80) {
        if (is_bom_at(pos_)) {
            return fail(LexErrorCode::UnexpectedBom, here(),
                        SourcePosition{pos_ + 3, line_, column_ + 1}, "Unexpected BOM");
        }

        std::size_t len = utf8_length_at(pos_);

        if (len == 0) {
            return fail(LexErrorCode::InvalidUtf8, here(), one_past_here(), "Invalid UTF-8");
        }

        SourcePosition end{pos_ + len, line_, column_ + 1};

        return fail(LexErrorCode::NonAsciiInLiteral, here(), end, "Non-ASCII character in literal");
    }

    if ((c >= 0 && c < 0x20) || c == 0x7F) {
        return fail(LexErrorCode::NonPrintableInLiteral, here(), one_past_here(),
                    "Non-printable character in literal");
    }

    return true;
}

bool Scanner::decode_escape(std::string &out) {
    SourcePosition start = here();
    advance_ascii(); // пропускаем '\'

    int c = peek();

    if (c == kEnd) {
        return fail(LexErrorCode::InvalidEscape, start, here(), "Incomplete escape sequence");
    }

    if (!validate_literal_character())
        return false;

    char decoded;

    switch (c) {
    case 'n':
        decoded = '\n';
        break;
    case 'r':
        decoded = '\r';
        break;
    case 't':
        decoded = '\t';
        break;
    case '0':
        decoded = '\0';
        break;
    case '\\':
        decoded = '\\';
        break;
    case '\'':
        decoded = '\'';
        break;
    case '"':
        decoded = '"';
        break;

    case 'x': {
        advance_ascii(); // пропускаем x

        if (!validate_literal_character())
            return false;
        int hi = hex_value(peek());
        if (hi >= 0)
            advance_ascii();
        if (!validate_literal_character())
            return false;
        int lo = hex_value(peek());

        if (hi < 0 || lo < 0) {
            return fail(LexErrorCode::InvalidHexEscape, start, here(), "Invalid hex escape");
        }

        int value = hi * 16 + lo;

        advance_ascii();

        if (value > 0x7F) {
            return fail(LexErrorCode::EscapeOutOfAsciiRange, start, here(),
                        "Hex escape exceeds ASCII range");
        }

        out.push_back(static_cast<char>(value));
        return true;
    }

    default:
        return fail(LexErrorCode::InvalidEscape, start, one_past_here(), "Unknown escape sequence");
    }

    out.push_back(decoded);
    advance_ascii();
    return true;
}

bool Scanner::scan_quoted(char quote, SourcePosition begin, std::string &out) {
    advance_ascii(); // открывающая кавычка

    while (true) {
        int c = peek();

        if (c == kEnd) {
            return fail(LexErrorCode::UnterminatedLiteral, begin, here(), "Unterminated literal");
        }

        if (c == quote) {
            advance_ascii();
            return true;
        }

        if (!validate_literal_character())
            return false;

        if (c == '\\') {
            if (!decode_escape(out))
                return false;

            continue; // в decode_escape уже сдвинут указатель, поэтому просто переходим к новой
                      // итерации
        }

        out.push_back(static_cast<char>(c));
        advance_ascii();
    }
}

} // namespace egg::detail
