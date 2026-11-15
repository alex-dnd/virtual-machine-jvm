#include "reader.h"

namespace egg::detail {

// Шаблоны определений: раскомментировать по мере реализации.
// Описания контрактов также приведены в reader.h.

/*
Вход разбора выражения: вызвать parse_logical_or(). Не потреблять внешние разделители ; , ) ] и =.
Не распознавать присваивание как выражение.
*/
// ast::ExprPtr Reader::parse_expression()
// {
//     TODO: реализовать.
// }

/*
Разобрать logical_and { || logical_and }. Строить BinaryExpr слева направо; сохранять диапазон
оператора и всего выражения.
*/
// ast::ExprPtr Reader::parse_logical_or()
// {
//     TODO: реализовать.
// }

/*
Разобрать comparison { && comparison }, группируя слева.
*/
// ast::ExprPtr Reader::parse_logical_and()
// {
//     TODO: реализовать.
// }

/*
Разобрать additive и не более одного из == != < <= > >= с правым additive. Следующий оператор
сравнения на этом уровне — ChainedComparison с диапазоном второго оператора. Явные скобки запускают
независимый parse_expression, поэтому (a < b) == c разрешено.
*/
// ast::ExprPtr Reader::parse_comparison()
// {
//     TODO: реализовать.
// }

/*
Разобрать product { (+ | -) product }, группируя слева.
*/
// ast::ExprPtr Reader::parse_additive()
// {
//     TODO: реализовать.
// }

/*
Разобрать unary { (* | / | %) unary }, группируя слева. Не вычислять операции и не проверять деление
на ноль.
*/
// ast::ExprPtr Reader::parse_product()
// {
//     TODO: реализовать.
// }

/*
Для ! и - потребить знак и рекурсивно вызвать parse_unary(), иначе parse_postfix(). Группировать
справа, сохранять оператор и диапазон. Не сворачивать минус в целый литерал.
*/
// ast::ExprPtr Reader::parse_unary()
// {
//     TODO: реализовать.
// }

/*
Разобрать primary и повторяющиеся . IDENT, [expression], (arguments). Последовательно оборачивать
получателя в MemberExpr, IndexExpr, CallExpr. Диапазон начинается с получателя и заканчивается
именем/закрывающим знаком. make()() здесь допустимо; проверка вызываемого объекта выполняется
позднее.
*/
// ast::ExprPtr Reader::parse_postfix()
// {
//     TODO: реализовать.
// }

/*
Разобрать целый, char, TEXT, true/false, IDENT, this, (expression) или литерал вектора. Скопировать
декодированные char/TEXT из Token::value, целые сохранить строкой цифр. Скобки представить отдельным
ParenthesizedExpr. На неподходящем токене записать список допустимых начал выражения.
*/
// ast::ExprPtr Reader::parse_primary()
// {
//     TODO: реализовать.
// }

/*
Разобрать vector < type > [arguments]. parse_type() читает только тип элемента после <; не путать
его с типом всего вектора. Создать VectorLiteral, разрешить пустой список, сохранить полный
диапазон.
*/
// ast::ExprPtr Reader::parse_vector_literal()
// {
//     TODO: реализовать.
// }

/*
Вызывается после уже потреблённого ( либо [. Разобрать пустой список или expression через запятые,
потребить closing (RightParen либо RightBracket). Запретить завершающую запятую. Пустой вектор —
успех; nullopt — ошибка.
*/
// std::optional<std::vector<ast::ExprPtr>> Reader::parse_arguments(TokenKind closing)
// {
//     TODO: реализовать.
// }

} // namespace egg::detail
