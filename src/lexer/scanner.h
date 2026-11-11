#ifndef EGG_INTERNAL_SCANNER_H
#define EGG_INTERNAL_SCANNER_H

#include "egg/lexer.h"

// Внутренний интерфейс реализации; не входит в публичный API библиотеки.
namespace egg::detail {

// Признак конца входа для peek().
constexpr int kEnd = -1;

class Scanner {
public:
    Scanner(const std::string &source, const std::string &source_name)
        : src_(source), name_(source_name) {}

    LexResult run();

private:
    // Общие операции: токены и диагностика.

    // Байт на расстоянии ahead от текущего места (0..255) или kEnd.
    int peek(std::size_t ahead = 0) const {
        std::size_t i = pos_ + ahead;
        if (i >= src_.size())
            return kEnd;
        return static_cast<unsigned char>(src_[i]);
    }

    bool at_end() const { return pos_ >= src_.size(); }

    // Текущее место в файле.
    SourcePosition here() const { return SourcePosition{pos_, line_, column_}; }

    // Шаг на один ASCII-байт, который НЕ перенос строки (буква, цифра,
    // знак, пробел, табуляция, кавычка...). Столбец +1.
    void advance_ascii() {
        ++pos_;
        ++column_;
    }

    // Добавить токен от begin до текущего места. lexeme = исходные байты.
    void add_token(TokenKind kind, SourcePosition begin, TokenValue value = std::monostate{});

    // Записать ошибку (сохраняется только первая). Всегда возвращает false,
    // чтобы можно было писать: return fail(...);
    bool fail(LexErrorCode code, SourcePosition begin, SourcePosition end, std::string message);

    // Диапазон «один байт с текущего места» (для ошибок в одном символе).
    SourcePosition one_past_here() const { return SourcePosition{pos_ + 1, line_, column_ + 1}; }

    // UTF-8, BOM, позиции, пробелы и комментарии (trivia.cpp).

    // Длина корректной UTF-8 последовательности, начинающейся с байта
    // offset (1..4), или 0, если там некорректный UTF-8 или конец файла.
    std::size_t utf8_length_at(std::size_t offset) const;

    // true, если с offset начинается BOM: байты EF BB BF.
    bool is_bom_at(std::size_t offset) const;

    // Шаг на одну кодовую точку длиной len байт (len из utf8_length_at).
    // Столбец +1 независимо от длины.
    void advance_code_point(std::size_t len);

    // Шаг через перенос строки: LF или CRLF. line +1, column = 1.
    void advance_newline();

    // Вызывается один раз в начале: пропустить BOM, если он первый.
    void skip_leading_bom();

    // Пропустить пробелы, табуляции, LF, CRLF и комментарии // до конца
    // строки. false — если найдена ошибка (InvalidUtf8, UnexpectedBom,
    // BareCarriageReturn); она уже записана через fail().
    bool skip_trivia();

    // Текущий байт >= 0x80 вне литерала и комментария. Записывает ошибку:
    // InvalidUtf8, UnexpectedBom или UnexpectedCharacter. Возвращает false.
    bool fail_non_ascii();

    // Идентификаторы, числа и знаки (tokens.cpp).

    // Текущий байт — буква или '_'. Читает слово, добавляет Identifier
    // или ключевое слово (KwClass...). Возвращает true.
    bool scan_identifier_or_keyword();

    // Текущий байт — цифра. Читает [0-9]+, добавляет IntegerLiteral.
    // Буква или '_' сразу после цифр -> InvalidIntegerSuffix, false.
    bool scan_integer();

    // Остальные ASCII-символы: знаки { } ( ) ... -> == != <= >= && ||.
    // Неизвестный символ (@, #, одиночный & или |, NUL...) ->
    // UnexpectedCharacter, false. Байты >= 0x80 сюда не попадают.
    bool scan_punct();

    // Литералы и escape-последовательности (literals.cpp).

    // Текущий байт — '\''. Читает символьный литерал, добавляет
    // CharLiteral с CharacterValue. false при ошибке.
    bool scan_char_literal();

    // Текущий байт — '"'. Читает текстовый литерал, добавляет
    // TextLiteral с TextValue. false при ошибке.
    bool scan_text_literal();

    // Текущий байт — '\\' внутри литерала. Разбирает escape и дописывает
    // получившийся символ в out. false при ошибке.
    bool decode_escape(std::string &out);

    // Состояние одного прохода.
    const std::string &src_;
    const std::string &name_;
    std::size_t pos_ = 0;    // byte_offset, с нуля
    std::size_t line_ = 1;   // с единицы
    std::size_t column_ = 1; // с единицы, в кодовых точках
    std::vector<Token> tokens_;
    std::optional<LexDiagnostic> error_;
};

} // namespace egg::detail

#endif // EGG_INTERNAL_SCANNER_H
