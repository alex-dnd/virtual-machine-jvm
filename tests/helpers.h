// Вспомогательные функции только для тестов (не часть библиотеки).
#pragma once
#include "egg/lexer.h"
#include <string>
#include <vector>

namespace test {

    // Разобрать строку и вернуть результат.
    inline egg::LexResult lex(const std::string& source) {
        return egg::Lexer(source).tokenize();
    }

    // Только категории токенов: удобно сравнивать списком.
    inline std::vector<egg::TokenKind> kinds(const egg::LexResult& r) {
        std::vector<egg::TokenKind> out;
        for (const auto& t : r.tokens) out.push_back(t.kind);
        return out;
    }

    // Только исходные тексты токенов.
    inline std::vector<std::string> lexemes(const egg::LexResult& r) {
        std::vector<std::string> out;
        for (const auto& t : r.tokens) out.push_back(t.lexeme);
        return out;
    }

} // namespace test
