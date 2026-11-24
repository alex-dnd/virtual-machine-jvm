#include "reader.h"

#include <memory>
#include <utility>

namespace egg::detail {
namespace {

template <typename Node>
ast::ExprPtr make_expression(SourceRange range, Node node)
{
    return std::make_unique<ast::Expr>(ast::Expr{range, std::move(node)});
}

std::optional<ast::BinaryOp> comparison_operator(TokenKind kind)
{
    switch (kind) {
    case TokenKind::Equal: return ast::BinaryOp::Equal;
    case TokenKind::NotEqual: return ast::BinaryOp::NotEqual;
    case TokenKind::Less: return ast::BinaryOp::Less;
    case TokenKind::LessEqual: return ast::BinaryOp::LessEqual;
    case TokenKind::Greater: return ast::BinaryOp::Greater;
    case TokenKind::GreaterEqual: return ast::BinaryOp::GreaterEqual;
    default: return std::nullopt;
    }
}

}

ast::ExprPtr Reader::parse_expression()
{
    return parse_logical_or();
}

ast::ExprPtr Reader::parse_logical_or()
{
    const auto begin = peek().range.begin;
    auto left = parse_logical_and();
    if (!left)
        return nullptr;

    while (at(TokenKind::LogicalOr)) {
        const auto operator_range = advance().range;
        auto right = parse_logical_and();
        if (!right)
            return nullptr;

        left = make_expression(range_from(begin), ast::BinaryExpr{
            ast::BinaryOp::LogicalOr, operator_range, std::move(left), std::move(right)
        });
    }

    return left;
}

ast::ExprPtr Reader::parse_logical_and()
{
    const auto begin = peek().range.begin;
    auto left = parse_comparison();
    if (!left)
        return nullptr;

    while (at(TokenKind::LogicalAnd)) {
        const auto operator_range = advance().range;
        auto right = parse_comparison();
        if (!right)
            return nullptr;

        left = make_expression(range_from(begin), ast::BinaryExpr{
            ast::BinaryOp::LogicalAnd, operator_range, std::move(left), std::move(right)
        });
    }

    return left;
}

ast::ExprPtr Reader::parse_comparison()
{
    const auto begin = peek().range.begin;
    auto left = parse_additive();
    if (!left)
        return nullptr;

    const auto op = comparison_operator(peek().kind);
    if (!op)
        return left;

    const auto operator_range = advance().range;
    auto right = parse_additive();
    if (!right)
        return nullptr;

    const auto &next = peek();
    if (comparison_operator(next.kind)) {
        fail(ParseErrorCode::ChainedComparison, next.range, next.kind, {},
             "chained comparison");
        return nullptr;
    }

    return make_expression(range_from(begin), ast::BinaryExpr{
        *op, operator_range, std::move(left), std::move(right)
    });
}

ast::ExprPtr Reader::parse_additive()
{
    const auto begin = peek().range.begin;
    auto left = parse_product();
    if (!left)
        return nullptr;

    while (at(TokenKind::Plus) || at(TokenKind::Minus)) {
        const auto &token = advance();
        const auto op = token.kind == TokenKind::Plus ? ast::BinaryOp::Add
                                                    : ast::BinaryOp::Subtract;
        auto right = parse_product();
        if (!right)
            return nullptr;

        left = make_expression(range_from(begin), ast::BinaryExpr{
            op, token.range, std::move(left), std::move(right)
        });
    }

    return left;
}

ast::ExprPtr Reader::parse_product()
{
    const auto begin = peek().range.begin;
    auto left = parse_unary();
    if (!left)
        return nullptr;

    while (at(TokenKind::Star) || at(TokenKind::Slash) || at(TokenKind::Percent)) {
        const auto &token = advance();
        auto op = ast::BinaryOp::Multiply;
        if (token.kind == TokenKind::Slash)
            op = ast::BinaryOp::Divide;
        else if (token.kind == TokenKind::Percent)
            op = ast::BinaryOp::Remainder;

        auto right = parse_unary();
        if (!right)
            return nullptr;

        left = make_expression(range_from(begin), ast::BinaryExpr{
            op, token.range, std::move(left), std::move(right)
        });
    }

    return left;
}

ast::ExprPtr Reader::parse_unary()
{
    if (!at(TokenKind::Bang) && !at(TokenKind::Minus))
        return parse_postfix();

    const auto begin = peek().range.begin;
    const auto &token = advance();
    const auto op = token.kind == TokenKind::Bang ? ast::UnaryOp::LogicalNot
                                                 : ast::UnaryOp::Negate;
    auto operand = parse_unary();
    if (!operand)
        return nullptr;

    return make_expression(range_from(begin), ast::UnaryExpr{
        op, token.range, std::move(operand)
    });
}

ast::ExprPtr Reader::parse_postfix()
{
    const auto begin = peek().range.begin;
    auto receiver = parse_primary();
    if (!receiver)
        return nullptr;

    for (;;) {
        if (match(TokenKind::Dot)) {
            auto member = parse_identifier();
            if (!member)
                return nullptr;

            receiver = make_expression(range_from(begin), ast::MemberExpr{
                std::move(receiver), std::move(*member)
            });
        } else if (match(TokenKind::LeftBracket)) {
            auto index = parse_expression();
            if (!index || !expect(TokenKind::RightBracket))
                return nullptr;

            receiver = make_expression(range_from(begin), ast::IndexExpr{
                std::move(receiver), std::move(index)
            });
        } else if (match(TokenKind::LeftParen)) {
            auto arguments = parse_arguments(TokenKind::RightParen);
            if (!arguments)
                return nullptr;

            receiver = make_expression(range_from(begin), ast::CallExpr{
                std::move(receiver), std::move(*arguments)
            });
        } else {
            return receiver;
        }
    }
}

ast::ExprPtr Reader::parse_primary()
{
    const auto &token = peek();
    const auto begin = token.range.begin;

    switch (token.kind) {
    case TokenKind::IntegerLiteral:
        advance();
        return make_expression(range_from(begin), ast::IntegerLiteral{token.lexeme});
    case TokenKind::CharLiteral:
        advance();
        return make_expression(range_from(begin), ast::CharacterLiteral{
            std::get<CharacterValue>(token.value)
        });
    case TokenKind::TextLiteral:
        advance();
        return make_expression(range_from(begin), ast::TextLiteral{std::get<TextValue>(token.value)});
    case TokenKind::KwTrue:
    case TokenKind::KwFalse:
        advance();
        return make_expression(range_from(begin), ast::BooleanLiteral{token.kind == TokenKind::KwTrue});
    case TokenKind::Identifier: {
        auto identifier = parse_identifier();
        if (!identifier)
            return nullptr;

        return make_expression(range_from(begin), ast::NameExpr{std::move(*identifier)});
    }
    case TokenKind::KwThis:
        advance();
        return make_expression(range_from(begin), ast::ThisExpr{});
    case TokenKind::LeftParen: {
        advance();
        auto expression = parse_expression();
        if (!expression || !expect(TokenKind::RightParen))
            return nullptr;

        return make_expression(range_from(begin), ast::ParenthesizedExpr{std::move(expression)});
    }
    case TokenKind::KwVector:
        return parse_vector_literal();
    default:
        unexpected({TokenKind::IntegerLiteral, TokenKind::CharLiteral, TokenKind::TextLiteral,
                    TokenKind::KwTrue, TokenKind::KwFalse, TokenKind::Identifier,
                    TokenKind::KwThis, TokenKind::LeftParen, TokenKind::KwVector,
                    TokenKind::Bang, TokenKind::Minus});
        return nullptr;
    }
}

ast::ExprPtr Reader::parse_vector_literal()
{
    const auto begin = peek().range.begin;
    if (!expect(TokenKind::KwVector) || !expect(TokenKind::Less))
        return nullptr;

    auto element_type = parse_type();
    if (!element_type || !expect(TokenKind::Greater) || !expect(TokenKind::LeftBracket))
        return nullptr;

    auto elements = parse_arguments(TokenKind::RightBracket);
    if (!elements)
        return nullptr;

    return make_expression(range_from(begin), ast::VectorLiteral{
        std::move(*element_type), std::move(*elements)
    });
}

std::optional<std::vector<ast::ExprPtr>> Reader::parse_arguments(TokenKind closing)
{
    std::vector<ast::ExprPtr> arguments;
    if (match(closing))
        return arguments;

    do {
        auto argument = parse_expression();
        if (!argument)
            return std::nullopt;

        arguments.push_back(std::move(argument));
    } while (match(TokenKind::Comma));

    if (!expect(closing))
        return std::nullopt;

    return arguments;
}

}
