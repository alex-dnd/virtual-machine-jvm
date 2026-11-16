#include "reader.h"

namespace egg::detail {

// Шаблоны определений: раскомментировать по мере реализации.
// Описания контрактов также приведены в reader.h.

/*
По class или fn вызвать соответствующий разбор. Другие токены, включая глобальные переменные, —
синтаксическая ошибка. Возвращать перемещаемый вариант объявления.
*/
// std::optional<ast::Declaration> Reader::parse_declaration()
// {
//     TODO: реализовать.
// }

/*
Разобрать class IDENT { члены }. Член: constructor, fn либо поле. Потребить обе скобки, сохранить
исходный порядок и диапазон. Повторные имена и несколько конструкторов оставить семантическому
анализу.
*/
// std::optional<ast::ClassDecl> Reader::parse_class()
// {
//     TODO: реализовать.
// }

/*
Разобрать type IDENT ;. Инициализаторы полей запрещены грамматикой. Диапазон включает точку с
запятой.
*/
// std::optional<ast::FieldDecl> Reader::parse_field()
// {
//     TODO: реализовать.
// }

/*
Разобрать constructor, список параметров и обязательный блок. Имя и возвращаемый тип у конструктора
отсутствуют.
*/
// std::optional<ast::ConstructorDecl> Reader::parse_constructor()
// {
//     TODO: реализовать.
// }

/*
Разобрать fn IDENT, параметры, необязательное -> type либо -> void и блок. Отсутствующий тип хранить
как nullopt, явный void — как VoidType. Метод и глобальная функция имеют одну форму.
*/
// std::optional<ast::FunctionDecl> Reader::parse_function()
// {
//     TODO: реализовать.
// }

/*
Потребить ( и ), внутри разобрать пустой список либо type IDENT через запятые. Запретить завершающую
запятую. Диапазон каждого параметра — от типа до имени. Пустой вектор — успех; nullopt — ошибка.
*/
// std::optional<std::vector<ast::Parameter>> Reader::parse_parameters()
// {
//     TODO: реализовать.
// }

/*
Разобрать int, bool, char, IDENT или vector < type > рекурсивно. void здесь запрещён. Имя типа не
разрешать. Каждый > потребляется отдельно. Сохранить диапазон всего типа.
*/
 std::optional<ast::Type> Reader::parse_type()
{
    auto begin = peek().range.begin;

    // 1. Базовые типы (int, bool, char)
    if (match(TokenKind::KwInt)) 
        return ast::Type{range_from(begin), ast::PrimitiveType::Int};
    if (match(TokenKind::KwBool)) 
        return ast::Type{range_from(begin), ast::PrimitiveType::Bool};
    if (match(TokenKind::KwChar)) 
        return ast::Type{range_from(begin), ast::PrimitiveType::Char};

    // 2. Вектор (vector < type >)
    if (match(TokenKind::KwVector)) {
        if (!expect(TokenKind::Less)) return std::nullopt;

        auto inner = parse_type();
        if (!inner) return std::nullopt;

        if (!expect(TokenKind::Greater)) return std::nullopt;

        // В ast::VectorType лежит unique_ptr<Type>, поэтому make_unique
        return ast::Type{range_from(begin), ast::VectorType{std::make_unique<ast::Type>(std::move(*inner))}};
    }

    // 3. Пользовательский тип (IDENT)
    if (auto ident = parse_identifier()) {
        return ast::Type{range_from(begin), std::move(*ident)};
    }

    // 4. Ошибка, если ничего не подошло
    unexpected({TokenKind::KwInt, TokenKind::KwBool, TokenKind::KwChar,
                TokenKind::KwVector, TokenKind::Identifier});
    return std::nullopt;
}

/*
Без продвижения pos_ и без диагностики проверить, начинается ли здесь полный type, за которым
следует IDENT. Использовать локальный индекс и разбор формы типа, учитывая вложенные vector. IDENT
IDENT — объявление; IDENT ( — выражение; vector<T> [ — литерал. На неполном типе вернуть false;
основной разбор выдаст ошибку. Не обращаться к таблице имён.
*/
 bool Reader::starts_variable_declaration() const
{
    std::size_t idx = pos_; // Локальный индекс, pos_ не меняем!

    // Рекурсивная лямбда для проверки типа без потребления токенов
    auto check_type = [&](auto& self, std::size_t& i) -> bool {
        if (i >= tokens_.size()) return false;

        auto kind = tokens_[i].kind;
        // Базовые типы
        if (kind == TokenKind::KwInt || kind == TokenKind::KwBool || kind == TokenKind::KwChar) {
            ++i;
            return true;
        }

        // Вектор
        if (kind == TokenKind::KwVector) {
            ++i;
            if (i >= tokens_.size() || tokens_[i].kind != TokenKind::Less) return false;
            ++i;
            if (!self(self, i)) return false; // Рекурсивно проверяем внутренний тип
            if (i >= tokens_.size() || tokens_[i].kind != TokenKind::Greater) return false;
            ++i;
            return true;
        }

        // Имя пользовательского типа
        if (kind == TokenKind::Identifier) {
            ++i;
            return true;
        }

        return false;
    };

    // 1. Проверяем, что это полный тип
    if (!check_type(check_type, idx)) return false;

    // 2. Проверяем, что после типа идет IDENT (имя переменной)
    if (idx >= tokens_.size() || tokens_[idx].kind != TokenKind::Identifier) return false;

    return true;
}

} // namespace egg::detail
