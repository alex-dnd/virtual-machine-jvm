#include <catch2/catch_test_macros.hpp>
#include "helpers.h"

using egg::TokenKind;

TEST_CASE("trivia: заготовка", "[trivia]") {
    auto r = test::lex("");
    REQUIRE_FALSE(r.error.has_value());
    REQUIRE(test::kinds(r) == std::vector<TokenKind>{TokenKind::EndOfFile});
}
