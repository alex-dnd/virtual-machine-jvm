#include "reader.h"

namespace egg::detail {

// Шаблоны определений: раскомментировать по мере реализации.
// Описания контрактов также приведены в reader.h.

/*
Потребить {, разбирать parse_statement() до }, потребить }. Пустой блок разрешён. EOF до } —
UnexpectedEnd. Сохранить диапазон со скобками и порядок операторов.
*/
// std::optional<ast::Block> Reader::parse_block()
// {
//     TODO: реализовать.
// }

/*
Выбрать блок, if, while, break, continue, return, объявление через starts_variable_declaration()
либо выражение/присваивание. Для блока обернуть Block в Stmt с тем же диапазоном. Пустой оператор ;
не допускается.
*/
// ast::StmtPtr Reader::parse_statement()
// {
//     TODO: реализовать.
// }

/*
Разобрать type IDENT [= expression] ;. Инициализатор отсутствует — nullptr. Создать Stmt с
VariableDecl и диапазоном до ;. Типы и область видимости не проверять.
*/
// ast::StmtPtr Reader::parse_variable_declaration()
// {
//     TODO: реализовать.
// }

/*
Разобрать if (expression) block [else (block | if)]. Ветви обязательно в скобках {}, кроме else if.
Сохранить отсутствие else как monostate, цепочку — как unique_ptr<IfStmt>. Диапазон охватывает все
ветви; parse_statement оборачивает результат в Stmt.
*/
// std::optional<ast::IfStmt> Reader::parse_if()
// {
//     TODO: реализовать.
// }

/*
Разобрать while (expression) block и создать Stmt с WhileStmt. Проверку bool оставить семантическому
анализатору.
*/
// ast::StmtPtr Reader::parse_while()
// {
//     TODO: реализовать.
// }

/*
Потребить break либо continue и обязательный ;, создать соответствующий Stmt. Наличие внешнего цикла
здесь не проверять.
*/
// ast::StmtPtr Reader::parse_break_or_continue()
// {
//     TODO: реализовать.
// }

/*
Разобрать return [expression] ;. При отсутствии выражения сохранить nullptr. Контекст функции и тип
результата не проверять.
*/
// ast::StmtPtr Reader::parse_return()
// {
//     TODO: реализовать.
// }

/*
Разобрать expression, затем необязательное = expression, затем обязательный ;. Создать
ExpressionStmt либо AssignmentStmt; сохранить диапазон =. a = b = c отклоняется на втором =.
Допустимость левой части и требование вызова для ExpressionStmt проверяются позднее.
*/
// ast::StmtPtr Reader::parse_expression_or_assignment()
// {
//     TODO: реализовать.
// }

} // namespace egg::detail
