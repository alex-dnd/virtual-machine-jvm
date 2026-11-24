# Тест-план парсера Egg 0.1

Проверяется полный путь `Lexer -> Parser::parse()`.
Для успешного разбора проверяются форма AST, диапазоны и имя источника.
Для ошибок проверяются код, диапазон, фактический и ожидаемые токены;
текст сообщения не является частью контракта.

| № | Что проверяем | Вход | Ожидание | Источник |
|---|---|---|---|---|
| 1 | Пустой поток | `""` | Пустой `Program`; диапазон EOF | `parser.h` |
| 2 | Пустой `main` | `fn main() {}` | Одна `FunctionDecl` с телом без операторов | spec §2 |
| 3 | Явный `-> void` | `fn main() -> void {}` | `return_type` содержит `VoidType` | spec §2 |
| 4 | Параметры функции | `fn f(int a, bool b) {}` | Два параметра в исходном порядке | spec §2 |
| 5 | Класс с полем | `class C { int x; }` | `ClassDecl` с одним `FieldDecl` | spec §2 |
| 6 | Конструктор | `class C { constructor() {} }` | `ConstructorDecl` с пустым телом | spec §2 |
| 7 | Метод | `class C { fn f() {} }` | `FunctionDecl` внутри класса | spec §2 |
| 8 | Объявление переменной | `int x;` | `VariableDecl` без инициализатора | spec §2 |
| 9 | Инициализация переменной | `int x = 1;` | `VariableDecl` с `IntegerLiteral` | spec §2 |
| 10 | Присваивание | `x = 1;` | `AssignmentStmt` | spec §2 |
| 11 | Выражение-оператор | `f();` | `ExpressionStmt` с `CallExpr` | spec §2 |
| 12 | Блок-оператор | `{}` | `Stmt` с `Block` | spec §2 |
| 13 | `if` без `else` | `if (x) {}` | `IfStmt` без else | spec §2 |
| 14 | `if` с `else` | `if (x) {} else {}` | `IfStmt` с `Block` в else | spec §2 |
| 15 | `else if` | `if (x) {} else if (y) {}` | В else находится `IfStmt` | spec §2 |
| 16 | `while` | `while (x) {}` | `WhileStmt` | spec §2 |
| 17 | `break` и `continue` | `break; continue;` | `BreakStmt`, `ContinueStmt` | spec §2 |
| 18 | `return;` | `return;` | `ReturnStmt` без значения | spec §2 |
| 19 | `return expr;` | `return 1;` | `ReturnStmt` с `IntegerLiteral` | spec §2 |
| 20 | Арифметика слева | `1 + 2 + 3;` | `(1 + 2) + 3` | `parser.h` |
| 21 | Унарные операции справа | `!!x;` | `!(!x)` | `parser.h` |
| 22 | Один уровень сравнения | `a < b;` | Один `BinaryExpr` с `Less` | `parser.h` |
| 23 | Запрет цепочки сравнений | `a < b < c;` | `ChainedComparison` | `parser.h` |
| 24 | Сравнение в скобках | `(a < b) < c;` | Успешный разбор | `parser.h` |
| 25 | Постфиксные вызовы | `make().run();` | `MemberExpr` с `CallExpr` | spec §2 |
| 26 | Вызов вызова | `make()();` | `CallExpr`, callee которого — `CallExpr` | spec §2 |
| 27 | Присваивание литералу | `1 = 2;` | `AssignmentStmt`; семантика не проверяется | spec §2 |
| 28 | `break` вне цикла | `break;` | `BreakStmt`; семантика не проверяется | spec §2 |
| 29 | Векторный литерал | `vector<int>[1, 2];` | `VectorLiteral` с типом элемента `int` | spec §2 |
| 30 | Незакрытая функция | `fn main() {` | `UnexpectedEnd` | `parser.h` |
| 31 | Незакрытая скобка | `fn main() { if (x {} }` | `UnexpectedToken` | `parser.h` |
| 32 | Лишний токен | `fn main() {} fn` | `UnexpectedToken` | `parser.h` |
| 33 | Отсутствие EOF | поток без EOF | `InvalidTokenStream` | `parser.h` |
| 34 | Два EOF | поток с двумя EOF | `InvalidTokenStream` | `parser.h` |
| 35 | Имя источника в ошибке | `@` | `source_name == "main.egg"` | `parser.h` |
| 36 | Повторный parse | `fn main() {}` | Структурно одинаковый результат | `parser.h` |