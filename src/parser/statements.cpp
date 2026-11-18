#include "reader.h"

#include <memory>
#include <utility>
#include <vector>

namespace egg::detail {
    std::optional<ast::Block> Reader::parse_block() {
        auto begin = peek().range.begin;

        if (!expect(TokenKind::LeftBrace))
            return std::nullopt;

        std::vector<ast::StmtPtr> statements;

        while (!at(TokenKind::RightBrace)) {
            if (at(TokenKind::EndOfFile)) {
                unexpected({TokenKind::RightBrace});
                return std::nullopt;
            }

            auto statement = parse_statement();

            if (!statement)
                return std::nullopt;

            statements.push_back(std::move(statement));
        }

        if (!expect(TokenKind::RightBrace))
            return std::nullopt;

        return ast::Block{
            range_from(begin),
            std::move(statements)
        };
    }

    ast::StmtPtr Reader::parse_statement() {
        if (at(TokenKind::LeftBrace)) {
            auto block = parse_block();

            if (!block)
                return nullptr;

            auto range = block->range;

            return std::make_unique<ast::Stmt>(
                ast::Stmt{range, std::move(*block)}
            );
        }

        if (at(TokenKind::KwIf)) {
            auto branch = parse_if();

            if (!branch)
                return nullptr;

            auto range = branch->range;

            return std::make_unique<ast::Stmt>(
                ast::Stmt{range, std::move(*branch)}
            );
        }

        if (at(TokenKind::KwWhile))
            return parse_while();

        if (at(TokenKind::KwBreak) ||
            at(TokenKind::KwContinue))
            return parse_break_or_continue();

        if (at(TokenKind::KwReturn))
            return parse_return();

        if (starts_variable_declaration())
            return parse_variable_declaration();

        return parse_expression_or_assignment();
    }

    ast::StmtPtr Reader::parse_variable_declaration() {
        auto begin = peek().range.begin;

        auto type = parse_type();
        if (!type)
            return nullptr;

        auto name = parse_identifier();
        if (!name)
            return nullptr;

        ast::ExprPtr initializer;

        if (match(TokenKind::Assign)) {
            initializer = parse_expression();

            if (!initializer)
                return nullptr;
        }

        if (!expect(TokenKind::Semicolon))
            return nullptr;

        ast::VariableDecl decl{
            std::move(*type),
            std::move(*name),
            std::move(initializer)
        };

        return std::make_unique<ast::Stmt>(
            ast::Stmt{range_from(begin), std::move(decl)}
        );
    }

    std::optional<ast::IfStmt> Reader::parse_if() {
        auto begin = peek().range.begin;

        if (!expect(TokenKind::KwIf))
            return std::nullopt;

        if (!expect(TokenKind::LeftParen))
            return std::nullopt;

        auto condition = parse_expression();
        if (!condition)
            return std::nullopt;

        if (!expect(TokenKind::RightParen))
            return std::nullopt;

        auto then_branch = parse_block();
        if (!then_branch)
            return std::nullopt;

        ast::ElseBranch else_branch;

        if (match(TokenKind::KwElse)) {
            if (at(TokenKind::KwIf)) {
                auto next_if = parse_if();

                if (!next_if)
                    return std::nullopt;

                else_branch =
                    std::make_unique<ast::IfStmt>(
                        std::move(*next_if)
                    );
            } else {
                auto else_block = parse_block();

                if (!else_block)
                    return std::nullopt;

                else_branch = std::move(*else_block);
            }
        }

        return ast::IfStmt{
            range_from(begin),
            std::move(condition),
            std::move(*then_branch),
            std::move(else_branch)
        };
    }

    ast::StmtPtr Reader::parse_while() {
        auto begin = peek().range.begin;

        if (!expect(TokenKind::KwWhile))
            return nullptr;

        if (!expect(TokenKind::LeftParen))
            return nullptr;

        auto condition = parse_expression();
        if (!condition)
            return nullptr;

        if (!expect(TokenKind::RightParen))
            return nullptr;

        auto body = parse_block();
        if (!body)
            return nullptr;

        ast::WhileStmt loop{
            std::move(condition),
            std::move(*body)
        };

        return std::make_unique<ast::Stmt>(
            ast::Stmt{range_from(begin), std::move(loop)}
        );
    }

    ast::StmtPtr Reader::parse_break_or_continue() {
        auto begin = peek().range.begin;
        bool is_break = at(TokenKind::KwBreak);

        if (!is_break && !at(TokenKind::KwContinue)) {
            unexpected({
                TokenKind::KwBreak,
                TokenKind::KwContinue
            });
            return nullptr;
        }

        advance();

        if (!expect(TokenKind::Semicolon))
            return nullptr;

        auto range = range_from(begin);

        if (is_break) {
            return std::make_unique<ast::Stmt>(
                ast::Stmt{range, ast::BreakStmt{}}
            );
        }

        return std::make_unique<ast::Stmt>(
            ast::Stmt{range, ast::ContinueStmt{}}
        );
    }

    ast::StmtPtr Reader::parse_return() {
        auto begin = peek().range.begin;

        if (!expect(TokenKind::KwReturn))
            return nullptr;

        ast::ExprPtr value;

        if (!at(TokenKind::Semicolon)) {
            value = parse_expression();

            if (!value)
                return nullptr;
        }

        if (!expect(TokenKind::Semicolon))
            return nullptr;

        return std::make_unique<ast::Stmt>(
            ast::Stmt{
                range_from(begin),
                ast::ReturnStmt{std::move(value)}
            }
        );
    }

    ast::StmtPtr Reader::parse_expression_or_assignment() {
        auto begin = peek().range.begin;

        auto left = parse_expression();
        if (!left)
            return nullptr;

        if (at(TokenKind::Assign)) {
            auto operator_range = advance().range;

            auto right = parse_expression();
            if (!right)
                return nullptr;

            if (!expect(TokenKind::Semicolon))
                return nullptr;

            ast::AssignmentStmt assignment{
                std::move(left),
                operator_range,
                std::move(right)
            };

            return std::make_unique<ast::Stmt>(
                ast::Stmt{
                    range_from(begin),
                    std::move(assignment)
                }
            );
        }

        if (!expect(TokenKind::Semicolon))
            return nullptr;

        return std::make_unique<ast::Stmt>(
            ast::Stmt{
                range_from(begin),
                ast::ExpressionStmt{std::move(left)}
            }
        );
    }
}