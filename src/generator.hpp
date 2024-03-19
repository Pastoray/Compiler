#pragma once

#include <vector>
#include <sstream>

#include "./tokenizer.hpp"
#include "./parser.hpp"


class Generator {
    public:
        inline explicit Generator(const NodeProg prog) : prog(std::move(prog)) {}
        
        inline void gen_expr(const NodeExpr& expr) {
            struct ExprVisitor {
                Generator* gen;
                void operator()(const NodeExprInt& expr_int) const {
                    gen->output << "mov rax, " << expr_int._int.value.value();
                    gen->output << "push rax\n";
                };
                void operator()(const NodeExprIdent& expr_ident) {

                };
            };
            ExprVisitor visitor { .gen = this };
            std::visit(visitor, expr.var);
        };
        
        void gen_stmt(const NodeStmt& stmt) {
            struct StmtVisitor {
                Generator* gen;
                void operator()(const NodeStmtRet& return_node) {
                    gen->gen_expr(return_node.expr);
                    gen->output << "    mov rax, 60\n";
                    gen->output << "    pop rdi\n";
                    gen->output << "    syscall\n";
                }
                void operator()(const NodeStmtLet& let_node) {

                }
            };
            StmtVisitor visitor { .gen = this };
            std::visit(visitor, stmt.var);
        }

        [[nodiscard]] inline std::string generate() {
            output << "global _start\n_start:\n";

            for (const NodeStmt& stmt : prog.stmts) {
                gen_stmt(stmt);
            }

            output << "    mov rax, 60\n";
            output << "    mov rdi, 0\n";
            output << "    syscall\n"; 
            return output.str();
        }

    private:
        const NodeProg prog;
        std::stringstream output;
};