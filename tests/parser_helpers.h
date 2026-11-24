#pragma once

#include "egg/lexer.h"
#include "egg/parser.h"

#include <string>
#include <utility>

namespace test {

inline egg::ParseResult parse(const std::string &source,
                              const std::string &source_name = "<test>") {
    auto lexed = egg::Lexer(source, source_name).tokenize();
    if (lexed.error)
        return egg::ParseDiagnostic{egg::ParseErrorCode::UnexpectedToken,
            source_name, lexed.error->range, lexed.error->message, std::nullopt, {}};
    return egg::Parser(std::move(lexed.tokens), source_name).parse();
}

} // namespace test
