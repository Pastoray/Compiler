#pragma once

#include <variant>
#include "./tokenizer.hpp"

struct NodeExprInt {
    Token _int;
};

struct NodeExprIdent {
    Token ident;
};

struct NodeExpr {
    std::variant<NodeExprInt, NodeExprIdent> var;
};

struct NodeStmt {
    std::variant<NodeStmtRet, NodeStmtLet> var;
};

struct NodeStmtRet {
    NodeExpr expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr expr;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
};

class Parser {
    public:
        inline explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), index(0) {}
        std::optional<NodeProg> parse_prog() {
            NodeProg prog;
            while (peek().has_value()) {
                if (auto stmt = parse_stmt()) {
                    prog.stmts.push_back(stmt.value());
                } else {
                    std::cerr << "Invalid Statement" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            index = 0;
        };
        std::optional<NodeStmt> parse_stmt() {
            if (peek().has_value() && peek().value().type == TokenType::_return) {
                NodeStmtRet return_node;
                while (peek().has_value()) {
                    if (peek().value().type == TokenType::_return && peek(1).value().type == TokenType::_open_paren) {
                        consume();
                        consume();
                        if (auto node_expr = parse_expr()) {
                            return_node = NodeStmtRet{ .expr = node_expr.value() };
                        } else {
                            std::cerr << "Invalid Expression" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        if (peek().has_value() && peek().value().type == TokenType::_close_paren) {
                            consume();
                        } else {
                            std::cerr << "Expected ')'" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        if (peek().has_value() && peek().value().type == TokenType::_semi) {
                            consume ();
                        } else {
                            std::cerr << "Expected ';'" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                return NodeStmt{ .var = return_node }; 
            } else if (peek().has_value() && peek().value().type == TokenType::_let && peek(1).has_value() &&
                        peek(1).value().type == TokenType::_ident &&
                        peek(2).has_value() && peek(2).value().type == TokenType::_eq) {
                consume();
                NodeStmtLet let_node = NodeStmtLet{ .ident = consume() };
                consume();
                if (auto expr = parse_expr()) {
                    let_node.expr = expr.value();
                } else {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if (peek().has_value() && peek().value().type == TokenType::_semi) {
                    consume();
                } else {
                    std::cerr << "Expected ';'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                return NodeStmt{ .var = let_node };
            } else {
                return {};
            }
        };
        std::optional<NodeExpr> parse_expr() {
            std::optional<NodeExpr> expr_node;
            if (peek().has_value() && peek().value().type == TokenType::_int) {
                return NodeExpr { NodeExprInt { ._int = consume() } };
            } else if (peek().has_value() && peek().value().type == TokenType::_ident) {
                return NodeExpr { .var = NodeExprIdent { .ident = consume() } };
            } else {
                return {};
            }
        }
    private:
        const std::vector<Token> tokens;
        size_t index;
        [[nodiscard]] inline std::optional<Token> peek(int offset = 0) const {
            if (index + offset >= tokens.size()) {
                return {};
            }
            return tokens.at(index + offset);
        }

        inline Token consume() {
            return tokens.at(index++);
        }
};