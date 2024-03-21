#pragma once

#include <variant>

#include "./tokenizer.hpp"
#include "./arena.hpp"

struct NodeTermInt {
    Token _int;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeExpr;

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeBinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprMult {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprSub {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprDiv {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMult*, NodeBinExprSub*, NodeBinExprDiv*> var;    
};

struct NodeTerm {
    std::variant<NodeTermIdent*, NodeTermInt*, NodeTermParen*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtRet {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt {
    std::variant<NodeStmtRet*, NodeStmtLet*> var;
};

struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

class Parser {
    public:
        inline explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), allocator(1024 * 1024 * 4) {}
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
            return prog;
        }
        std::optional<NodeStmt*> parse_stmt() {
            NodeStmt* stmt_node = allocator.alloc<NodeStmt>();
            if (peek().has_value() && peek().value().type == TokenType::_return) {
                NodeStmtRet* return_node = allocator.alloc<NodeStmtRet>();
                while (peek().has_value()) {
                    if (peek().value().type == TokenType::_return && peek(1).value().type == TokenType::_open_paren) {
                        consume();
                        consume();
                        if (auto node_expr = parse_expr()) {
                            return_node->expr = node_expr.value();
                        } else {
                            std::cerr << "Invalid Expression" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        try_consume(TokenType::_close_paren, "Expected ')'");
                        try_consume(TokenType::_semi, "Expected ';'");
                    }
                }
                stmt_node->var = return_node;
                return stmt_node;
            } else if (peek().has_value() && peek().value().type == TokenType::_let && peek(1).has_value() &&
                        peek(1).value().type == TokenType::_ident &&
                        peek(2).has_value() && peek(2).value().type == TokenType::_eq) {
                consume();
                NodeStmtLet* let_node = allocator.alloc<NodeStmtLet>();
                let_node->ident = consume();
                consume();
                if (auto expr = parse_expr()) {
                    let_node->expr = expr.value();
                } else {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::_semi, "Expected ';'");
                stmt_node->var = let_node;
                return stmt_node;
            } else {
                return {};
            }
        }
        std::optional<NodeBinExpr*> parse_bin_expr() {
            return {};
        }

        std::optional<NodeTerm*> parse_term() {
            if (auto int_lit = try_consume(TokenType::_int)) {
                NodeTerm* term = allocator.alloc<NodeTerm>();
                NodeTermInt* node_term_int = allocator.alloc<NodeTermInt>();
                node_term_int->_int = int_lit.value();
                term->var = node_term_int;
                return term;
            } else if (auto ident = try_consume(TokenType::_ident)) {
                NodeTerm* term = allocator.alloc<NodeTerm>();
                NodeTermIdent* node_term_ident = allocator.alloc<NodeTermIdent>();
                node_term_ident->ident = ident.value();
                term->var = node_term_ident;
                return term;
            } else if (auto open_paren = try_consume(TokenType::_open_paren)) {
                auto expr = parse_expr();
                if (!expr.has_value()) {
                    std::cerr << "Expected expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::_close_paren, "Expected ')'");
                NodeTerm* term = allocator.alloc<NodeTerm>();
                NodeTermParen* term_paren = allocator.alloc<NodeTermParen>();
                term_paren->expr = expr.value();
                term->var = term_paren;
                return term;
            } else {
                return {};
            }
        }

        std::optional<NodeExpr*> parse_expr(int min_prec = 0) {
            std::optional<NodeTerm*> term_lhs = parse_term();
            if (!term_lhs.has_value()) {
                return {};
            }

            NodeExpr* expr_lhs = allocator.alloc<NodeExpr>();
            expr_lhs->var = term_lhs.value();
            
            while (true) {
                std::optional<Token> token = peek();
                std::optional<int> prec = bin_prec(token->type);
                if (token.has_value()) {
                    if (!prec.has_value() || prec.value() < min_prec) {
                        break;
                    }
                } else {
                    break;
                }

                Token op = consume();
                int next_min_prec = prec.value() + 1;
                auto expr_rhs = parse_expr(next_min_prec);

                if (!expr_rhs.has_value()) {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                NodeBinExpr* expr = allocator.alloc<NodeBinExpr>();
                auto expr_lhs2 = allocator.alloc<NodeExpr>();
                if (op.type == TokenType::_plus) {
                    NodeBinExprAdd* add = allocator.alloc<NodeBinExprAdd>();
                    expr_lhs2->var = expr_lhs->var;
                    add->lhs = expr_lhs2;
                    add->rhs = expr_rhs.value();
                    expr->var = add;
                } else if (op.type == TokenType::_mult) {
                    NodeBinExprMult* mult = allocator.alloc<NodeBinExprMult>();
                    expr_lhs2->var = expr_lhs->var;
                    mult->lhs = expr_lhs2;
                    mult->rhs = expr_rhs.value();
                    expr->var = mult;
                } else if (op.type == TokenType::_sub) {
                    NodeBinExprSub* sub = allocator.alloc<NodeBinExprSub>();
                    expr_lhs2->var = expr_lhs->var;
                    sub->lhs = expr_lhs2;
                    sub->rhs = expr_rhs.value();
                    expr->var = sub;
                } else if (op.type == TokenType::_div) {
                    NodeBinExprDiv* div = allocator.alloc<NodeBinExprDiv>();
                    expr_lhs2->var = expr_lhs->var;
                    div->lhs = expr_lhs2;
                    div->rhs = expr_rhs.value();
                    expr->var = div;
                } else {
                    assert(false);
                }
                expr_lhs->var = expr;  
            }
            return expr_lhs;
        }
    private:
        [[nodiscard]] inline std::optional<Token> peek(int offset = 0) const {
            if (index + offset >= tokens.size()) {
                return {};
            }
            return tokens.at(index + offset);
        }

        inline Token consume() {
            return tokens.at(index++);
        }


        inline Token try_consume(TokenType type, const std::string& err_msg) {
            if (peek().has_value() && peek().value().type == type) {
                return consume();
            } else {
                std::cerr << err_msg << "\n";
                exit(EXIT_FAILURE);
            }
        }

        inline std::optional<Token> try_consume(TokenType type) {
            if (peek().has_value() && peek().value().type == type) {
                return consume();
            } else {
                return {};
            }
        }

        const std::vector<Token> tokens;
        size_t index = 0;
        ArenaAllocator allocator;
};