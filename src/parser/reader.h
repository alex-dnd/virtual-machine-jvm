#ifndef EGG_INTERNAL_READER_H
#define EGG_INTERNAL_READER_H

#include "egg/parser.h"

#include <cstddef>

// Внутренний интерфейс реализации; не входит в публичный API.
namespace egg::detail {

// Один Reader — один проход. Ссылки действуют на протяжении run().
// run() проверяет поток до вызова peek/at/advance и методов грамматики.
// Методы разбора потребляют всю конструкцию, включая её ограничители.
// Успех: узел/optional со значением; ошибка: nullptr/nullopt и error_ заполнен.
// После ошибки вызывающий метод немедленно возвращает ошибку, не продолжая разбор.
// Отката позиции и восстановления нет. Необязательные части внутри AST могут
// быть пустыми при успешном разборе; это не ошибка метода.
class Reader {
public:
    Reader(const std::vector<Token> &tokens, const std::string &source_name);
    ParseResult run();

private:
    // core.cpp
    // До любого peek() проверить ровно один EOF в конце. При преждевременном EOF указать первый
    // такой токен; без EOF — нулевой диапазон после последнего токена, actual = nullopt; пустой
    // поток — позиция {0, 1, 1}. Записать InvalidTokenStream. Лексические значения не
    // перепроверять.
    bool validate_stream();

    // Вернуть токен на расстоянии ahead; за концом возвращать конечный EOF. Не менять позицию.
    // Вызывается только после успешной validate_stream(). Избежать переполнения pos_ + ahead.
    const Token &peek(std::size_t ahead = 0) const;

    // Проверить категорию текущего токена, ничего не потребляя.
    bool at(TokenKind kind) const;

    // Вернуть текущий токен и перейти к следующему. На EOF оставаться на месте. Ссылки действуют до
    // уничтожения входного потока.
    const Token &advance();

    // При совпадении потребить токен и вернуть true, иначе вернуть false без диагностики и
    // продвижения.
    bool match(TokenKind kind);

    // При совпадении потребить токен и вернуть true. Иначе вызвать unexpected({kind}) и вернуть
    // false.
    bool expect(TokenKind kind);

    // Построить диапазон от begin до конца последнего потреблённого токена. Если ничего не
    // потреблено, вернуть {begin, begin}. EOF не потребляется. Program получает конец отдельно из
    // EOF.
    SourceRange range_from(SourcePosition begin) const;

    // Сохранить только первую диагностику, добавив name_. Всегда вернуть false. Не печатать
    // сообщения и не бросать исключения для ошибок синтаксиса.
    bool fail(ParseErrorCode code, SourceRange range, std::optional<TokenKind> actual,
              std::vector<TokenKind> expected, std::string message);

    // Вызвать fail для текущего токена: UnexpectedEnd на EOF, иначе UnexpectedToken. Передать его
    // диапазон, категорию и ожидаемые токены. Всегда вернуть false.
    bool unexpected(std::vector<TokenKind> expected);

    // Потребить Identifier, скопировать имя и диапазон. На другом токене записать
    // unexpected({Identifier}) и вернуть nullopt.
    std::optional<ast::Identifier> parse_identifier();

    // declarations.cpp
    // По class или fn вызвать соответствующий разбор. Другие токены, включая глобальные переменные,
    // — синтаксическая ошибка. Возвращать перемещаемый вариант объявления.
    std::optional<ast::Declaration> parse_declaration();

    // Разобрать class IDENT { члены }. Член: constructor, fn либо поле. Потребить обе скобки,
    // сохранить исходный порядок и диапазон. Повторные имена и несколько конструкторов оставить
    // семантическому анализу.
    std::optional<ast::ClassDecl> parse_class();

    // Разобрать type IDENT ;. Инициализаторы полей запрещены грамматикой. Диапазон включает точку с
    // запятой.
    std::optional<ast::FieldDecl> parse_field();

    // Разобрать constructor, список параметров и обязательный блок. Имя и возвращаемый тип у
    // конструктора отсутствуют.
    std::optional<ast::ConstructorDecl> parse_constructor();

    // Разобрать fn IDENT, параметры, необязательное -> type либо -> void и блок. Отсутствующий тип
    // хранить как nullopt, явный void — как VoidType. Метод и глобальная функция имеют одну форму.
    std::optional<ast::FunctionDecl> parse_function();

    // Потребить ( и ), внутри разобрать пустой список либо type IDENT через запятые. Запретить
    // завершающую запятую. Диапазон каждого параметра — от типа до имени. Пустой вектор — успех;
    // nullopt — ошибка.
    std::optional<std::vector<ast::Parameter>> parse_parameters();

    // Разобрать int, bool, char, IDENT или vector < type > рекурсивно. void здесь запрещён. Имя
    // типа не разрешать. Каждый > потребляется отдельно. Сохранить диапазон всего типа.
    std::optional<ast::Type> parse_type();

    // Без продвижения pos_ и без диагностики проверить, начинается ли здесь полный type, за которым
    // следует IDENT. Использовать локальный индекс и разбор формы типа, учитывая вложенные vector.
    // IDENT IDENT — объявление; IDENT ( — выражение; vector<T> [ — литерал. На неполном типе
    // вернуть false; основной разбор выдаст ошибку. Не обращаться к таблице имён.
    bool starts_variable_declaration() const;

    // statements.cpp
    // Потребить {, разбирать parse_statement() до }, потребить }. Пустой блок разрешён. EOF до } —
    // UnexpectedEnd. Сохранить диапазон со скобками и порядок операторов.
    std::optional<ast::Block> parse_block();

    // Выбрать блок, if, while, break, continue, return, объявление через
    // starts_variable_declaration() либо выражение/присваивание. Для блока обернуть Block в Stmt с
    // тем же диапазоном. Пустой оператор ; не допускается.
    ast::StmtPtr parse_statement();

    // Разобрать type IDENT [= expression] ;. Инициализатор отсутствует — nullptr. Создать Stmt с
    // VariableDecl и диапазоном до ;. Типы и область видимости не проверять.
    ast::StmtPtr parse_variable_declaration();

    // Разобрать if (expression) block [else (block | if)]. Ветви обязательно в скобках {}, кроме
    // else if. Сохранить отсутствие else как monostate, цепочку — как unique_ptr<IfStmt>. Диапазон
    // охватывает все ветви; parse_statement оборачивает результат в Stmt.
    std::optional<ast::IfStmt> parse_if();

    // Разобрать while (expression) block и создать Stmt с WhileStmt. Проверку bool оставить
    // семантическому анализатору.
    ast::StmtPtr parse_while();

    // Потребить break либо continue и обязательный ;, создать соответствующий Stmt. Наличие
    // внешнего цикла здесь не проверять.
    ast::StmtPtr parse_break_or_continue();

    // Разобрать return [expression] ;. При отсутствии выражения сохранить nullptr. Контекст функции
    // и тип результата не проверять.
    ast::StmtPtr parse_return();

    // Разобрать expression, затем необязательное = expression, затем обязательный ;. Создать
    // ExpressionStmt либо AssignmentStmt; сохранить диапазон =. a = b = c отклоняется на втором =.
    // Допустимость левой части и требование вызова для ExpressionStmt проверяются позднее.
    ast::StmtPtr parse_expression_or_assignment();

    // expressions.cpp
    // Вход разбора выражения: вызвать parse_logical_or(). Не потреблять внешние разделители ; , ) ]
    // и =. Не распознавать присваивание как выражение.
    ast::ExprPtr parse_expression();

    // Разобрать logical_and { || logical_and }. Строить BinaryExpr слева направо; сохранять
    // диапазон оператора и всего выражения.
    ast::ExprPtr parse_logical_or();

    // Разобрать comparison { && comparison }, группируя слева.
    ast::ExprPtr parse_logical_and();

    // Разобрать additive и не более одного из == != < <= > >= с правым additive. Следующий оператор
    // сравнения на этом уровне — ChainedComparison с диапазоном второго оператора. Явные скобки
    // запускают независимый parse_expression, поэтому (a < b) == c разрешено.
    ast::ExprPtr parse_comparison();

    // Разобрать product { (+ | -) product }, группируя слева.
    ast::ExprPtr parse_additive();

    // Разобрать unary { (* | / | %) unary }, группируя слева. Не вычислять операции и не проверять
    // деление на ноль.
    ast::ExprPtr parse_product();

    // Для ! и - потребить знак и рекурсивно вызвать parse_unary(), иначе parse_postfix().
    // Группировать справа, сохранять оператор и диапазон. Не сворачивать минус в целый литерал.
    ast::ExprPtr parse_unary();

    // Разобрать primary и повторяющиеся . IDENT, [expression], (arguments). Последовательно
    // оборачивать получателя в MemberExpr, IndexExpr, CallExpr. Диапазон начинается с получателя и
    // заканчивается именем/закрывающим знаком. make()() здесь допустимо; проверка вызываемого
    // объекта выполняется позднее.
    ast::ExprPtr parse_postfix();

    // Разобрать целый, char, TEXT, true/false, IDENT, this, (expression) или литерал вектора.
    // Скопировать декодированные char/TEXT из Token::value, целые сохранить строкой цифр. Скобки
    // представить отдельным ParenthesizedExpr. На неподходящем токене записать список допустимых
    // начал выражения.
    ast::ExprPtr parse_primary();

    // Разобрать vector < type > [arguments]. parse_type() читает только тип элемента после <; не
    // путать его с типом всего вектора. Создать VectorLiteral, разрешить пустой список, сохранить
    // полный диапазон.
    ast::ExprPtr parse_vector_literal();

    // Вызывается после уже потреблённого ( либо [. Разобрать пустой список или expression через
    // запятые, потребить closing (RightParen либо RightBracket). Запретить завершающую запятую.
    // Пустой вектор — успех; nullopt — ошибка.
    std::optional<std::vector<ast::ExprPtr>> parse_arguments(TokenKind closing);

    const std::vector<Token> &tokens_;
    const std::string &name_;
    std::size_t pos_ = 0;
    std::optional<ParseDiagnostic> error_;
};

} // namespace egg::detail

#endif // EGG_INTERNAL_READER_H
