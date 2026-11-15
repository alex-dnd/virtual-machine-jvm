#include "reader.h"

namespace egg::detail {

// Шаблоны определений: раскомментировать по мере реализации.
// Описания контрактов также приведены в reader.h.

/*
Сохранить ссылки на поток и имя. pos_ = 0, error_ пуст. Не читать токены до проверки потока.
Инициализировать tokens_ и name_ списком инициализации.
*/
// Reader::Reader(const std::vector<Token> &tokens, const std::string &source_name)
// {
//     TODO: реализовать.
// }

/*
Проверить поток через validate_stream(); при ошибке вернуть диагностику. Затем разбирать
parse_declaration() до EOF, сохраняя исходный порядок. Собрать Program с именем и диапазоном от
первого токена до EOF. Один EOF даёт пустую программу. При любой ошибке вернуть только первую
диагностику, без частичного AST. Reader используется для одного прохода.
*/
// ParseResult Reader::run()
// {
//     TODO: реализовать.
// }

/*
До любого peek() проверить ровно один EOF в конце. При преждевременном EOF указать первый такой
токен; без EOF — нулевой диапазон после последнего токена, actual = nullopt; пустой поток — позиция
{0, 1, 1}. Записать InvalidTokenStream. Лексические значения не перепроверять.
*/
// bool Reader::validate_stream()
// {
//     TODO: реализовать.
// }

/*
Вернуть токен на расстоянии ahead; за концом возвращать конечный EOF. Не менять позицию. Вызывается
только после успешной validate_stream(). Избежать переполнения pos_ + ahead.
*/
// const Token &Reader::peek(std::size_t ahead) const
// {
//     TODO: реализовать.
// }

/*
Проверить категорию текущего токена, ничего не потребляя.
*/
// bool Reader::at(TokenKind kind) const
// {
//     TODO: реализовать.
// }

/*
Вернуть текущий токен и перейти к следующему. На EOF оставаться на месте. Ссылки действуют до
уничтожения входного потока.
*/
// const Token &Reader::advance()
// {
//     TODO: реализовать.
// }

/*
При совпадении потребить токен и вернуть true, иначе вернуть false без диагностики и продвижения.
*/
// bool Reader::match(TokenKind kind)
// {
//     TODO: реализовать.
// }

/*
При совпадении потребить токен и вернуть true. Иначе вызвать unexpected({kind}) и вернуть false.
*/
// bool Reader::expect(TokenKind kind)
// {
//     TODO: реализовать.
// }

/*
Построить диапазон от begin до конца последнего потреблённого токена. Если ничего не потреблено,
вернуть {begin, begin}. EOF не потребляется. Program получает конец отдельно из EOF.
*/
// SourceRange Reader::range_from(SourcePosition begin) const
// {
//     TODO: реализовать.
// }

/*
Сохранить только первую диагностику, добавив name_. Всегда вернуть false. Не печатать сообщения и не
бросать исключения для ошибок синтаксиса.
*/
// bool Reader::fail(ParseErrorCode code, SourceRange range, std::optional<TokenKind> actual,
// std::vector<TokenKind> expected, std::string message)
// {
//     TODO: реализовать.
// }

/*
Вызвать fail для текущего токена: UnexpectedEnd на EOF, иначе UnexpectedToken. Передать его
диапазон, категорию и ожидаемые токены. Всегда вернуть false.
*/
// bool Reader::unexpected(std::vector<TokenKind> expected)
// {
//     TODO: реализовать.
// }

/*
Потребить Identifier, скопировать имя и диапазон. На другом токене записать unexpected({Identifier})
и вернуть nullopt.
*/
// std::optional<ast::Identifier> Reader::parse_identifier()
// {
//     TODO: реализовать.
// }

} // namespace egg::detail
