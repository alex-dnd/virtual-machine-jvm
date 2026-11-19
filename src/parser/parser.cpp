#include "egg/parser.h"
#include "reader.h"

#include <utility>

namespace egg {

Parser::Parser(std::vector<Token> tokens, std::string source_name)
    : tokens_(std::move(tokens)), source_name_(std::move(source_name)) {}

ParseResult Parser::parse() const {
    return detail::Reader(tokens_, source_name_).run();
}

} // namespace egg
