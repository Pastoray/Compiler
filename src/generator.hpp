#pragma once

#include <vector>
#include <sstream>
#include <unordered_map>
#include <cassert>

#include "./tokenizer.hpp"
#include "./parser.hpp"

class Generator {
    public:
        inline explicit Generator(const NodeProg prog) : prog(std::move(prog)) {}
        
        inline void gen_term(const NodeTerm* term) {
            struct TermVisitor {
                Generator* gen;
                void operator()(const NodeTermInt* term_int) {
                    gen->output << "    mov rax, " << term_int->_int.value.value() << "\n";
                    gen->push("rax");
                }
                void operator()(const NodeTermIdent* term_ident) {
                    if (auto var = gen->vars.find(term_ident->ident.value.value()); var == gen->vars.end()) {
                        std::cerr << "Identifier does not exist: " << term_ident->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    const Var var = gen->vars.at(term_ident->ident.value.value());
                    std::stringstream offset;
                    offset << "QWORD [rsp + " << (gen->stack_size - var.stack_loc - 1) * 8 << "]\n";
                    gen->push(offset.str());
                }
                void operator()(const NodeTermParen* term_paren) {
                    gen->gen_expr(term_paren->expr);
                }
            };
            TermVisitor visitor { .gen = this };
            std::visit(visitor, term->var);
        }

        inline void gen_bin_expr(const NodeBinExpr* bin_expr) {
            struct BinExprVisitor {
                Generator* gen;
                void operator()(const NodeBinExprSub* sub) const {
                    gen->gen_expr(sub->rhs);
                    gen->gen_expr(sub->lhs);
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->output << "    sub rax, rbx\n";
                    gen->push("rax");
                }
                void operator()(const NodeBinExprDiv* div) const {
                    gen->gen_expr(div->rhs);
                    gen->gen_expr(div->lhs);
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->output << "    div rbx\n";
                    gen->push("rax");
                }
                void operator()(const NodeBinExprAdd* add) const {
                    gen->gen_expr(add->rhs);
                    gen->gen_expr(add->lhs);
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->output << "    add rax, rbx\n";
                    gen->push("rax");
                }
                void operator()(const NodeBinExprMult* mult) const {
                    gen->gen_expr(mult->rhs);
                    gen->gen_expr(mult->lhs);
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->output << "    mul rbx\n";
                    gen->push("rax");
                }

            };
            BinExprVisitor visitor { .gen = this };
            std::visit(visitor, bin_expr->var);
        }

        inline void gen_expr(const NodeExpr* expr) {
            struct ExprVisitor {
                Generator* gen;
                void operator()(const NodeTerm* term) const {
                    gen->gen_term(term);
                }

                void operator()(const NodeBinExpr* bin_expr) const {
                    gen->gen_bin_expr(bin_expr);
                }
            };
            ExprVisitor visitor { .gen = this };
            std::visit(visitor, expr->var);
        };
        inline void gen_stmt(const NodeStmt* stmt) {
            struct StmtVisitor {
                Generator* gen;
                void operator()(const NodeStmtRet* stmt_ret) const {
                    gen->gen_expr(stmt_ret->expr);
                    gen->output << "    mov rax, 60\n";
                    gen->pop("rdi");
                    gen->output << "    syscall\n";
                }

                void operator()(const NodeStmtLet* stmt_let) const {
                    if (auto var = gen->vars.find(stmt_let->ident.value.value()); var != gen->vars.end()) {
                        std::cerr << "Identifier already declared: " << stmt_let->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen->vars.insert({ stmt_let->ident.value.value(), Var{ .stack_loc = gen->stack_size } });
                    gen->gen_expr(stmt_let->expr);
                }
            };
            StmtVisitor visitor { .gen = this };
            std::visit(visitor, stmt->var);
        }

        [[nodiscard]] inline std::string gen_prog() {
            output << "global _start\n_start:\n";

            for (const NodeStmt* stmt : prog.stmts) {
                gen_stmt(stmt);
            }

            output << "    mov rax, 60\n";
            output << "    mov rdi, 0\n";
            output << "    syscall\n";
            return output.str();
        }

    private:
        void push(std::string reg) {
            output << "    push " << reg << "\n";
            stack_size++;
        }

        void pop(std::string reg) {
            output << "    pop " << reg << "\n";
            stack_size--;            
        }

        struct Var {
            size_t stack_loc;
        };
        const NodeProg prog;
        std::stringstream output;
        size_t stack_size = 0;
        std::unordered_map<std::string, Var> vars {};

};

