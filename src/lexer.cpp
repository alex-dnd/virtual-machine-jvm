// Реализация библиотеки «Лексер» для Egg 0.1.
// Публичный интерфейс: include/egg/lexer.h (менять только с согласия Саши).
//
// Файл разделён на разделы. Каждый разработчик меняет ТОЛЬКО свой раздел:
//   РАЗДЕЛ 0 — общий (Глеб): состояние, токены, ошибки, главный цикл.
//   РАЗДЕЛ 1 — Нина:  UTF-8, BOM, переносы строк, пробелы, комментарии.
//   РАЗДЕЛ 2 — Димка Петрик: идентификаторы, ключевые слова, числа, знаки.
//   РАЗДЕЛ 3 — Леша: символьные и текстовые литералы, escape.
// Так при слиянии веток почти не будет конфликтов.

#include "egg/lexer.h"

#include <utility>

namespace egg {

    // Внутренние детали реализации (не в публичном API). Не анонимное
    // пространство имён, чтобы заглушки не давали предупреждений «не используется».
    namespace detail {

        // Признак «дальше нет байтов» для peek().
        constexpr int kEnd = -1;

        class Scanner {
        public:
            Scanner(const std::string& source, const std::string& source_name)
            : src_(source), name_(source_name) {}

            LexResult run();

        private:
            // ===================== РАЗДЕЛ 0 — общий (Глеб) =====================

            // Байт на расстоянии ahead от текущего места (0..255) или kEnd.
            int peek(std::size_t ahead = 0) const {
                std::size_t i = pos_ + ahead;
                if (i >= src_.size()) return kEnd;
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
            void add_token(TokenKind kind, SourcePosition begin,
                           TokenValue value = std::monostate{}) {
                Token t;
                t.kind = kind;
                t.range = SourceRange{begin, here()};
                t.lexeme = src_.substr(begin.byte_offset, pos_ - begin.byte_offset);
                t.value = std::move(value);
                tokens_.push_back(std::move(t));
                           }

                           // Записать ошибку (сохраняется только первая). Всегда возвращает false,
                           // чтобы можно было писать: return fail(...);
                           bool fail(LexErrorCode code, SourcePosition begin, SourcePosition end,
                                     std::string message) {
                               if (!error_) {
                                   error_ = LexDiagnostic{code, name_, SourceRange{begin, end},
                                   std::move(message)};
                               }
                               return false;
                                     }

                                     // Диапазон «один байт с текущего места» (для ошибок в одном символе).
                                     SourcePosition one_past_here() const {
                                         return SourcePosition{pos_ + 1, line_, column_ + 1};
                                     }

                                     // ============ РАЗДЕЛ 1 — Нина: UTF-8, BOM, переносы, пробелы ============

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

                                     // ======= РАЗДЕЛ 2 — Димка Петрик: слова, числа, знаки =======

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

                                     // ======= РАЗДЕЛ 3 — Леша: литералы и escape =======

                                     // Текущий байт — '\''. Читает символьный литерал, добавляет
                                     // CharLiteral с CharacterValue. false при ошибке.
                                     bool scan_char_literal();

                                     // Текущий байт — '"'. Читает текстовый литерал, добавляет
                                     // TextLiteral с TextValue. false при ошибке.
                                     bool scan_text_literal();

                                     // Текущий байт — '\\' внутри литерала. Разбирает escape и дописывает
                                     // получившийся символ в out. false при ошибке.
                                     bool decode_escape(std::string& out);

                                     // ===================== Состояние (Глеб) =====================
                                     const std::string& src_;
                                     const std::string& name_;
                                     std::size_t pos_ = 0;     // byte_offset, с нуля
                                     std::size_t line_ = 1;    // с единицы
                                     std::size_t column_ = 1;  // с единицы, в кодовых точках
                                     std::vector<Token> tokens_;
                                     std::optional<LexDiagnostic> error_;
        };

        // ===================== РАЗДЕЛ 0 — главный цикл (Глеб) =====================

        inline bool is_ascii_letter(int c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        }

        inline bool is_digit(int c) { return c >= '0' && c <= '9'; }

        LexResult Scanner::run() {
            skip_leading_bom();
            while (true) {
                if (!skip_trivia()) break;
                if (at_end()) {
                    SourcePosition end = here();
                    Token eof;
                    eof.kind = TokenKind::EndOfFile;
                    eof.range = SourceRange{end, end};
                    eof.value = std::monostate{};
                    tokens_.push_back(std::move(eof));
                    break;
                }
                int c = peek();
                bool ok = false;
                if (is_ascii_letter(c) || c == '_') {
                    ok = scan_identifier_or_keyword();
                } else if (is_digit(c)) {
                    ok = scan_integer();
                } else if (c == '\'') {
                    ok = scan_char_literal();
                } else if (c == '"') {
                    ok = scan_text_literal();
                } else if (c >= 0x80) {
                    ok = fail_non_ascii();
                } else {
                    ok = scan_punct();
                }
                if (!ok) break;
            }
            return LexResult{std::move(tokens_), std::move(error_)};
        }

        // ============ РАЗДЕЛ 1 — Нина ============
        // Сейчас здесь заглушки: файл компилируется, но код не готов.

        std::size_t Scanner::utf8_length_at(std::size_t offset) const {
            // TODO(Нина): проверить 1–4 байта по таблице из задачи Л2.
            if (offset >= src_.size()) return 0;
            return static_cast<unsigned char>(src_[offset]) < 0x80 ? 1 : 0;
        }

        bool Scanner::is_bom_at(std::size_t offset) const {
            // TODO(Нина)
            (void)offset;
            return false;
        }

        void Scanner::advance_code_point(std::size_t len) {
            // TODO(Нина)
            pos_ += len;
            ++column_;
        }

        void Scanner::advance_newline() {
            // TODO(Нина): LF или CRLF.
            ++pos_;
            ++line_;
            column_ = 1;
        }

        void Scanner::skip_leading_bom() {
            // TODO(Нина)
        }

        bool Scanner::skip_trivia() {
            // TODO(Нина): сейчас пропускает только пробелы.
            while (peek() == ' ') advance_ascii();
            return true;
        }

        bool Scanner::fail_non_ascii() {
            // TODO(Нина)
            return fail(LexErrorCode::UnexpectedCharacter, here(), one_past_here(),
                        "unexpected character");
        }

        // ============ РАЗДЕЛ 2 — Димка Петрик ============

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

        // ============ РАЗДЕЛ 3 — Леша ============

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

        bool Scanner::decode_escape(std::string& out) {
            // TODO(Леша)
            (void)out;
            return fail(LexErrorCode::InvalidEscape, here(), one_past_here(),
                        "not implemented: escape");
        }

    } // namespace detail

    // ===================== Публичный API (Глеб) =====================

    Lexer::Lexer(std::string source, std::string source_name)
    : source_(std::move(source)), source_name_(std::move(source_name)) {}

    LexResult Lexer::tokenize() const {
        detail::Scanner scanner(source_, source_name_);
        return scanner.run();
    }

} // namespace egg
