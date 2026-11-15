#include "egg/lexer.h"
#include "scanner.h"

#include <utility>

namespace egg {

Lexer::Lexer(std::string source, std::string source_name)
    : source_(std::move(source)), source_name_(std::move(source_name)) {}

LexResult Lexer::tokenize() const {
    detail::Scanner scanner(source_, source_name_);
    return scanner.run();
}

} // namespace egg
