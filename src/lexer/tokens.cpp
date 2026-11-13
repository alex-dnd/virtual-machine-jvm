#include "scanner.h"

namespace egg::detail {

bool Scanner::scan_identifier_or_keyword() {
    // TODO(Димка Петрик)
    return fail(LexErrorCode::UnexpectedCharacter, here(), one_past_here(),
                "not implemented: identifier");
}

bool Scanner::scan_integer() {
    // TODO(Димка Петрик)
    return fail(LexErrorCode::UnexpectedCharacter, here(), one_past_here(),
                "not implemented: integer");
}

bool Scanner::scan_punct() {
    // TODO(Димка Петрик)
    return fail(LexErrorCode::UnexpectedCharacter, here(), one_past_here(),
                "not implemented: punctuation");
}

} // namespace egg::detail
