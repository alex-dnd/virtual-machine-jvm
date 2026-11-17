#include "reader.h"

namespace egg::detail {

// Шаблоны определений: раскомментировать по мере реализации.
// Описания контрактов также приведены в reader.h.

/*
По class или fn вызвать соответствующий разбор. Другие токены, включая глобальные переменные, —
синтаксическая ошибка. Возвращать перемещаемый вариант объявления.
*/
std::optional<ast::Declaration> Reader::parse_declaration()
{
    if (at(TokenKind::KwClass)) return parse_class();
    if (at(TokenKind::KwFn)) return parse_function();
    
    unexpected({TokenKind::KwClass, TokenKind::KwFn});
    return std::nullopt;
}

/*
Разобрать class IDENT { члены }. Член: constructor, fn либо поле. Потребить обе скобки, сохранить
исходный порядок и диапазон. Повторные имена и несколько конструкторов оставить семантическому
анализу.
*/
std::optional<ast::ClassDecl> Reader::parse_class()
{
    auto begin = peek().range.begin;
    if (!expect(TokenKind::KwClass)) return std::nullopt;
    
    auto name = parse_identifier();
    if (!name) return std::nullopt;
    
    if (!expect(TokenKind::LeftBrace)) return std::nullopt;
    
    std::vector<ast::ClassMember> members;
    while (!at(TokenKind::RightBrace) && !at(TokenKind::EndOfFile)) {
        if (at(TokenKind::KwConstructor)) {
            auto c = parse_constructor();
            if (!c) return std::nullopt;
            members.push_back(std::move(*c));
        } else if (at(TokenKind::KwFn)) {
            auto f = parse_function();
            if (!f) return std::nullopt;
            members.push_back(std::move(*f));
        } else {
            auto f = parse_field();
            if (!f) return std::nullopt;
            members.push_back(std::move(*f));
        }
    }
    
    if (!expect(TokenKind::RightBrace)) return std::nullopt;
    
    return ast::ClassDecl{range_from(begin), std::move(*name), std::move(members)};
}

/*
Разобрать type IDENT ;. Инициализаторы полей запрещены грамматикой. Диапазон включает точку с
запятой.
*/
std::optional<ast::FieldDecl> Reader::parse_field()
{
    auto begin = peek().range.begin;
    auto type = parse_type();
    if (!type) return std::nullopt;
    
    auto name = parse_identifier();
    if (!name) return std::nullopt;
    
    if (!expect(TokenKind::Semicolon)) return std::nullopt;
    
    return ast::FieldDecl{range_from(begin), std::move(*type), std::move(*name)};
}

/*
Разобрать constructor, список параметров и обязательный блок. Имя и возвращаемый тип у конструктора
отсутствуют.
*/
std::optional<ast::ConstructorDecl> Reader::parse_constructor()
{
    auto begin = peek().range.begin;
    if (!expect(TokenKind::KwConstructor)) return std::nullopt;
    
    auto params = parse_parameters();
    if (!params) return std::nullopt;
    
    auto body = parse_block();
    if (!body) return std::nullopt;
    
    return ast::ConstructorDecl{range_from(begin), std::move(*params), std::move(*body)};
}

/*
Разобрать fn IDENT, параметры, необязательное -> type либо -> void и блок. Отсутствующий тип хранить
как nullopt, явный void — как VoidType. Метод и глобальная функция имеют одну форму.
*/
std::optional<ast::FunctionDecl> Reader::parse_function()
{
    auto begin = peek().range.begin;
    if (!expect(TokenKind::KwFn)) return std::nullopt;
    
    auto name = parse_identifier();
    if (!name) return std::nullopt;
    
    auto params = parse_parameters();
    if (!params) return std::nullopt;
    
    std::optional<ast::ReturnType> ret_type;
    if (match(TokenKind::Arrow)) {
        if (match(TokenKind::KwVoid)) {
            ret_type = ast::VoidType{range_from(begin)};
        } else {
            auto t = parse_type();
            if (!t) return std::nullopt;
            ret_type = std::move(*t);
        }
    }
    
    auto body = parse_block();
    if (!body) return std::nullopt;
    
    return ast::FunctionDecl{range_from(begin), std::move(*name), std::move(*params), std::move(ret_type), std::move(*body)};
}

/*
Потребить ( и ), внутри разобрать пустой список либо type IDENT через запятые. Запретить завершающую
запятую. Диапазон каждого параметра — от типа до имени. Пустой вектор — успех; nullopt — ошибка.
*/
std::optional<std::vector<ast::Parameter>> Reader::parse_parameters()
{
    if (!expect(TokenKind::LeftParen)) return std::nullopt;
    
    std::vector<ast::Parameter> params;
    if (!at(TokenKind::RightParen)) {
        while (true) {
            auto begin = peek().range.begin;
            auto type = parse_type();
            if (!type) return std::nullopt;
            
            auto name = parse_identifier();
            if (!name) return std::nullopt;
            
            params.push_back(ast::Parameter{range_from(begin), std::move(*type), std::move(*name)});
            
            if (!match(TokenKind::Comma)) break;
        }
    }
    
    if (!expect(TokenKind::RightParen)) return std::nullopt;
    return params;
}

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
