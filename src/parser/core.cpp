#include "reader.h"

#include <utility>

namespace egg::detail {

Reader::Reader(const std::vector<Token> &tokens, const std::string &source_name)
    : tokens_(tokens), name_(source_name) {}

ParseResult Reader::run() {
    if (!validate_stream())
        return *error_;

    const auto begin = peek().range.begin;
    while (!at(TokenKind::EndOfFile)) {
        auto declaration = parse_declaration();
        if (!declaration)
            return *error_;
        // Declarations are intentionally parsed by the declarations task.
        // Keep the successful result in source order.
        declarations_.push_back(std::move(*declaration));
    }

    return ast::Program{name_, SourceRange{begin, peek().range.end},
                        std::move(declarations_)};
}

bool Reader::validate_stream() {
    const SourcePosition origin{0, 1, 1};
    if (tokens_.empty()) {
        return fail(ParseErrorCode::InvalidTokenStream, {origin, origin},
                    std::nullopt, {}, "Token stream is empty");
    }

    for (std::size_t i = 0; i < tokens_.size(); ++i) {
        if (tokens_[i].kind == TokenKind::EndOfFile && i + 1 != tokens_.size()) {
            return fail(ParseErrorCode::InvalidTokenStream, tokens_[i].range,
                        TokenKind::EndOfFile, {}, "EOF appears before the end of the token stream");
        }
    }

    if (tokens_.back().kind != TokenKind::EndOfFile) {
        const auto end = tokens_.back().range.end;
        return fail(ParseErrorCode::InvalidTokenStream, {end, end},
                    std::nullopt, {}, "Token stream has no EOF");
    }
    return true;
}

const Token &Reader::peek(std::size_t ahead) const {
    const std::size_t remaining = tokens_.size() - pos_;
    if (ahead >= remaining)
        return tokens_.back();
    return tokens_[pos_ + ahead];
}

bool Reader::at(TokenKind kind) const { return peek().kind == kind; }

const Token &Reader::advance() {
    const Token &token = peek();
    if (token.kind != TokenKind::EndOfFile)
        ++pos_;
    return token;
}

bool Reader::match(TokenKind kind) {
    if (!at(kind))
        return false;
    advance();
    return true;
}

bool Reader::expect(TokenKind kind) {
    if (match(kind))
        return true;
    return unexpected({kind});
}

SourceRange Reader::range_from(SourcePosition begin) const {
    if (pos_ == 0)
        return {begin, begin};
    return {begin, tokens_[pos_ - 1].range.end};
}

bool Reader::fail(ParseErrorCode code, SourceRange range,
                  std::optional<TokenKind> actual,
                  std::vector<TokenKind> expected, std::string message) {
    if (!error_)
        error_ = ParseDiagnostic{code, name_, range, std::move(message),
                                 actual, std::move(expected)};
    return false;
}

bool Reader::unexpected(std::vector<TokenKind> expected) {
    const auto &token = peek();
    const auto code = token.kind == TokenKind::EndOfFile
        ? ParseErrorCode::UnexpectedEnd : ParseErrorCode::UnexpectedToken;
    return fail(code, token.range, token.kind, std::move(expected),
                "Unexpected token");
}

std::optional<ast::Identifier> Reader::parse_identifier() {
    if (!at(TokenKind::Identifier)) {
        unexpected({TokenKind::Identifier});
        return std::nullopt;
    }
    const auto token = advance();
    return ast::Identifier{token.range, token.lexeme};
}

} // namespace egg::detail
