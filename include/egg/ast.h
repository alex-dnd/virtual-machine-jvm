#ifndef EGG_AST_H
#define EGG_AST_H

/**
 * @file ast.h
 * @brief Синтаксическое дерево Egg 0.1; C++17.
 * @details Узлы содержат синтаксис до разрешения имён и проверки типов.
 * Дерево владеет строками и дочерними узлами, перемещается без копирования.
 * Все обязательные unique_ptr в результате успешного разбора ненулевые.
 * Диапазоны полуоткрытые, включают ограничители и завершающий знак ;,
 * если они входят в соответствующую конструкцию. Комментарии не сохраняются.
 * Типы позиций переиспользуются из lexer.h.
 */

#include "egg/lexer.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace egg::ast {

struct Identifier {
    SourceRange range;
    std::string name;
};

struct Type;
using TypePtr = std::unique_ptr<Type>;

enum class PrimitiveType { Int, Bool, Char };

struct VectorType {
    TypePtr element;
};

/** @brief Запись типа; Identifier разрешается в имя класса позднее. */
struct Type {
    SourceRange range;
    std::variant<PrimitiveType, Identifier, VectorType> value;
};

/** @brief void существует только в позиции возвращаемого типа. */
struct VoidType {
    SourceRange range;
};

using ReturnType = std::variant<VoidType, Type>;

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct IntegerLiteral {
    /// Исходные десятичные цифры, включая ведущие нули; знак — отдельный узел.
    /// Диапазон int64 и особый случай -9223372036854775808 проверяются позднее.
    std::string digits;
};

struct BooleanLiteral {
    bool value;
};

struct CharacterLiteral {
    CharacterValue value;
};

struct TextLiteral {
    TextValue value; ///< Декодированные ASCII-байты, включая возможные NUL.
};

struct NameExpr {
    Identifier name;
};

struct ThisExpr {};

/** @brief Сохраняет явные скобки, в том числе для проверки формы операнда. */
struct ParenthesizedExpr {
    ExprPtr expression;
};

enum class UnaryOp { Negate, LogicalNot };

struct UnaryExpr {
    UnaryOp op;
    SourceRange operator_range;
    ExprPtr operand;
};

enum class BinaryOp {
    Multiply,
    Divide,
    Remainder,
    Add,
    Subtract,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    LogicalAnd,
    LogicalOr
};

struct BinaryExpr {
    BinaryOp op;
    SourceRange operator_range;
    ExprPtr left;
    ExprPtr right;
};

/**
 * @brief Общая форма вызова функции, метода, конструктора и встроенной операции.
 * @details Допустимость callee и аргументов проверяет семантический анализатор.
 * Например, make()() синтаксически представимо, но статически недопустимо.
 */
struct CallExpr {
    ExprPtr callee;
    std::vector<ExprPtr> arguments;
};

struct MemberExpr {
    ExprPtr receiver;
    Identifier member;
};

struct IndexExpr {
    ExprPtr receiver;
    ExprPtr index;
};

struct VectorLiteral {
    Type element_type; ///< В vector<T>[...] хранится T, а не vector<T>.
    std::vector<ExprPtr> elements;
};

using ExprValue = std::variant<IntegerLiteral, BooleanLiteral, CharacterLiteral, TextLiteral,
                               NameExpr, ThisExpr, ParenthesizedExpr, UnaryExpr, BinaryExpr,
                               CallExpr, MemberExpr, IndexExpr, VectorLiteral>;

struct Expr {
    SourceRange range;
    ExprValue value;
};

struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

struct Block {
    SourceRange range; ///< Включает обе фигурные скобки.
    std::vector<StmtPtr> statements;
};

struct VariableDecl {
    Type type;
    Identifier name;
    ExprPtr initializer; ///< nullptr означает отсутствие явной инициализации.
};

/** @brief Присваивание — оператор; допустимость target проверяется позднее. */
struct AssignmentStmt {
    ExprPtr target;
    SourceRange operator_range;
    ExprPtr value;
};

struct ExpressionStmt {
    /// Семантический анализ проверяет требование вызова в выражении-операторе.
    ExprPtr expression;
};

struct IfStmt;
using ElseBranch = std::variant<std::monostate, Block, std::unique_ptr<IfStmt>>;

struct IfStmt {
    SourceRange range; ///< Включает всю цепочку else if / else, если она есть.
    ExprPtr condition;
    Block then_branch;
    ElseBranch else_branch; ///< monostate означает отсутствие else.
};

struct WhileStmt {
    ExprPtr condition;
    Block body;
};

struct BreakStmt {};
struct ContinueStmt {};

struct ReturnStmt {
    ExprPtr value; ///< nullptr для return;.
};

using StmtValue = std::variant<Block, VariableDecl, AssignmentStmt, ExpressionStmt, IfStmt,
                               WhileStmt, BreakStmt, ContinueStmt, ReturnStmt>;

struct Stmt {
    SourceRange range;
    StmtValue value;
};

struct Parameter {
    SourceRange range;
    Type type;
    Identifier name;
};

/** @brief Одна синтаксическая форма для глобальной функции и метода класса. */
struct FunctionDecl {
    SourceRange range;
    Identifier name;
    std::vector<Parameter> parameters;
    /// nullopt — стрелка и тип отсутствуют (неявный void).
    /// Явное -> void сохраняется как VoidType.
    std::optional<ReturnType> return_type;
    Block body;
};

struct FieldDecl {
    SourceRange range;
    Type type;
    Identifier name;
};

struct ConstructorDecl {
    SourceRange range;
    std::vector<Parameter> parameters;
    Block body;
};

using ClassMember = std::variant<FieldDecl, ConstructorDecl, FunctionDecl>;

struct ClassDecl {
    SourceRange range;
    Identifier name;
    /// Исходный порядок; уникальность имён и конструктора проверяется позднее.
    std::vector<ClassMember> members;
};

using Declaration = std::variant<ClassDecl, FunctionDecl>;

struct Program {
    std::string source_name;
    /// От начала первого токена до EOF; для пустой программы — диапазон EOF.
    SourceRange range;
    std::vector<Declaration> declarations; ///< В порядке исходного текста.
};

} // namespace egg::ast

#endif // EGG_AST_H
