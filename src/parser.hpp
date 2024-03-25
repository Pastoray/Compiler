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

struct NodeStmt;

struct NodeScope {
    std::vector<NodeStmt*> stmts;
};

struct NodeIfPred;

struct NodeIfPredElif {
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeIfPredElse {
    NodeScope* scope;
};

struct NodeStmtIf {
    NodeExpr* expr {};
    NodeScope* scope {};
    std::optional<NodeIfPred*> pred; 
};

struct NodeIfPred {
    std::variant<NodeIfPredElif*, NodeIfPredElse*> var;
};

struct NodeStmtAssign {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt {
    std::variant<NodeStmtRet*, NodeStmtLet*, NodeScope*, NodeStmtIf*, NodeStmtAssign*> var;
};

struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

class Parser {
    public:
        inline explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), allocator(1024 * 1024 * 4) {}

        void error_expected(const std::string& msg) const {
            std::cerr << "[Parse Error] Expected " << msg << " on line " << peek(-1).value().line << std::endl; 
            exit(EXIT_FAILURE);
        }

        std::optional<NodeProg> parse_prog() {
            NodeProg prog;
            while (peek().has_value()) {
                if (auto stmt = parse_stmt()) {
                    prog.stmts.push_back(stmt.value());
                } else {
                    error_expected("statement");
                }
            }
            index = 0;
            return prog;
        }

        std::optional<NodeScope*> parse_scope() {
            if (!try_consume(TokenType::_open_curly)) {
                return {};
            }
            int stmt_line = 0;
            NodeScope* scope = allocator.alloc<NodeScope>();
            while (auto stmt = parse_stmt()) {
                scope->stmts.push_back(stmt.value());
            }
            try_consume_err(TokenType::_close_curly);
            return scope;
        }

        std::optional<NodeIfPred*> parse_if_pred() {
            if (try_consume(TokenType::_elif)) {
                NodeIfPredElif* elif_pred = allocator.alloc<NodeIfPredElif>();
                try_consume_err(TokenType::_open_paren);
                if (auto expr = parse_expr()) {
                    elif_pred->expr = expr.value();
                } else {
                    error_expected("expression");
                }
                try_consume_err(TokenType::_close_paren);
                if (auto scope = parse_scope()) {
                    elif_pred->scope = scope.value();
                } else {
                    error_expected("scope");
                }
                elif_pred->pred = parse_if_pred();
                NodeIfPred* if_pred = allocator.alloc<NodeIfPred>();
                if_pred->var = elif_pred;
                return if_pred;
            } else if (try_consume(TokenType::_else)) {
                NodeIfPredElse* else_pred = allocator.alloc<NodeIfPredElse>();
                if (auto scope = parse_scope()) {
                    else_pred->scope = scope.value();
                } else {
                    error_expected("scope");
                }
                NodeIfPred* if_pred = allocator.alloc<NodeIfPred>();
                if_pred->var = else_pred;
                return if_pred;
            }
            return {};
        }

        std::optional<NodeStmt*> parse_stmt() {
            NodeStmt* stmt_node = allocator.alloc<NodeStmt>();
            if (peek().has_value() && peek().value().type == TokenType::_return) {
                NodeStmtRet* return_node = allocator.alloc<NodeStmtRet>();
                if (peek().value().type == TokenType::_return && peek(1).value().type == TokenType::_open_paren) {
                    consume();
                    consume();
                    if (auto node_expr = parse_expr()) {
                        return_node->expr = node_expr.value();
                    } else {
                        error_expected("expression");
                    }
                    try_consume_err(TokenType::_close_paren);
                    try_consume_err(TokenType::_semi);
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
                    error_expected("expression");
                }
                try_consume_err(TokenType::_semi);
                stmt_node->var = let_node;
                return stmt_node;
            } else if (peek().has_value() && peek().value().type == TokenType::_open_curly) {
                if (auto scope = parse_scope()) {
                    NodeStmt* stmt = allocator.alloc<NodeStmt>();
                    stmt->var = scope.value();
                    return stmt;
                } else {
                    error_expected("scope");
                }
            } else if (try_consume(TokenType::_if)) {
                NodeStmtIf* stmt_if = allocator.alloc<NodeStmtIf>();
                try_consume_err(TokenType::_open_paren);
                if (auto expr = parse_expr()) {
                    stmt_if->expr = expr.value();
                } else {
                    error_expected("expression");
                }
                try_consume_err(TokenType::_close_paren);
                if (auto scope = parse_scope()) {
                    stmt_if->scope = scope.value();
                } else {
                    error_expected("scope");
                }
                stmt_if->pred = parse_if_pred();
                NodeStmt* stmt = allocator.alloc<NodeStmt>();
                stmt->var = stmt_if;
                return stmt; 
            } else if (peek().has_value() && peek().value().type == TokenType::_ident &&
                        peek(1).has_value() && peek(1).value().type == TokenType::_eq) {
                NodeStmtAssign* assign_node = allocator.alloc<NodeStmtAssign>();
                assign_node->ident = consume();
                consume();
                if (auto expr = parse_expr()) {
                    assign_node->expr = expr.value();
                } else {
                    error_expected("expression");
                }
                try_consume_err(TokenType::_semi);
                stmt_node->var = assign_node;
                return stmt_node;
            } else {
                return {};
            }
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
                    error_expected("expression");
                }
                try_consume_err(TokenType::_close_paren);
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
                    error_expected("expression");
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
                } else if (op.type == TokenType::_minus) {
                    NodeBinExprSub* sub = allocator.alloc<NodeBinExprSub>();
                    expr_lhs2->var = expr_lhs->var;
                    sub->lhs = expr_lhs2;
                    sub->rhs = expr_rhs.value();
                    expr->var = sub;
                } else if (op.type == TokenType::_fslash) {
                    NodeBinExprDiv* div = allocator.alloc<NodeBinExprDiv>();
                    expr_lhs2->var = expr_lhs->var;
                    div->lhs = expr_lhs2;
                    div->rhs = expr_rhs.value();
                    expr->var = div;
                } else {
                    error_expected("operand");
                }
                expr_lhs->var = expr;  
            }
            return expr_lhs;
        }
    private:
        [[nodiscard]] inline std::optional<Token> peek(const int offset = 0) const {
            if (index + offset >= tokens.size()) {
                return {};
            }
            return tokens.at(index + offset);
        }

        inline Token consume() {
            return tokens.at(index++);
        }


        inline Token try_consume_err(TokenType type) {
            if (peek().has_value() && peek().value().type == type) {
                return consume();
            }
            error_expected(token_to_string(type));
            return {};
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