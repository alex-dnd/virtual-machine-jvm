#include "scanner.h"

namespace egg::detail {

bool Scanner::scan_char_literal() {
    // TODO(Леша)
    return fail(LexErrorCode::UnexpectedCharacter, here(), one_past_here(),
                "not implemented: char literal");
}

bool Scanner::scan_text_literal() {
    // TODO(Леша)
    return fail(LexErrorCode::UnexpectedCharacter, here(), one_past_here(),
                "not implemented: text literal");
}

bool Scanner::decode_escape(std::string &out) {
    // TODO(Леша)
    (void)out;
    return fail(LexErrorCode::InvalidEscape, here(), one_past_here(), "not implemented: escape");
}

} // namespace egg::detail
