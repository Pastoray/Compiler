#pragma once

#include <vector>
#include <sstream>
#include <map>
#include <algorithm>

#include "./tokenizer.hpp"
#include "./parser.hpp"

class Generator {
    public:
        inline explicit Generator(const NodeProg prog) : prog(std::move(prog)) {}
        
        inline void gen_term(const NodeTerm* term) {
            struct TermVisitor {
                Generator& gen;
                void operator()(const NodeTermInt* term_int) {
                    gen.output << "    mov rax, " << term_int->_int.value.value() << "\n";
                    gen.push("rax");
                }
                void operator()(const NodeTermIdent* term_ident) {
                auto itr = std::find_if(
                            gen.vars.cbegin(),
                            gen.vars.cend(),
                            [&](const Var& var) { return var.name == term_ident->ident.value.value(); });
                    if (itr == gen.vars.cend()) {
                            std::cerr << "Identifier does not exist: " << term_ident->ident.value.value() << std::endl;
                            exit(EXIT_FAILURE);
                    }
                    std::stringstream offset;
                    offset << "QWORD [rsp + " << (gen.stack_size - (*itr).stack_loc - 1) * 8 << "]";
                    gen.push(offset.str());
                }
                void operator()(const NodeTermParen* term_paren) {
                    gen.gen_expr(term_paren->expr);
                }
            };
            TermVisitor visitor { .gen = *this };
            std::visit(visitor, term->var);
        }

        inline void gen_bin_expr(const NodeBinExpr* bin_expr) {
            struct BinExprVisitor {
                Generator& gen;
                void operator()(const NodeBinExprSub* sub) const {
                    gen.gen_expr(sub->rhs);
                    gen.gen_expr(sub->lhs);
                    gen.pop("rax");
                    gen.pop("rbx");
                    gen.output << "    sub rax, rbx\n";
                    gen.push("rax");
                }
                void operator()(const NodeBinExprDiv* div) const {
                    gen.gen_expr(div->rhs);
                    gen.gen_expr(div->lhs);
                    gen.pop("rax");
                    gen.pop("rbx");
                    gen.output << "    div rbx\n";
                    gen.push("rax");
                }
                void operator()(const NodeBinExprAdd* add) const {
                    gen.gen_expr(add->rhs);
                    gen.gen_expr(add->lhs);
                    gen.pop("rax");
                    gen.pop("rbx");
                    gen.output << "    add rax, rbx\n";
                    gen.push("rax");
                }
                void operator()(const NodeBinExprMult* mult) const {
                    gen.gen_expr(mult->rhs);
                    gen.gen_expr(mult->lhs);
                    gen.pop("rax");
                    gen.pop("rbx");
                    gen.output << "    mul rbx\n";
                    gen.push("rax");
                }

            };
            BinExprVisitor visitor { .gen = *this };
            std::visit(visitor, bin_expr->var);
        }

        inline void gen_expr(const NodeExpr* expr) {
            struct ExprVisitor {
                Generator& gen;
                void operator()(const NodeTerm* term) const {
                    gen.gen_term(term);
                }

                void operator()(const NodeBinExpr* bin_expr) const {
                    gen.gen_bin_expr(bin_expr);
                }
            };
            ExprVisitor visitor { .gen = *this };
            std::visit(visitor, expr->var);
        };

        void gen_scope(const NodeScope* scope) {
            begin_scope();
            for (NodeStmt* stmt : scope->stmts) {
                gen_stmt(stmt);
            }
            end_scope();
        }

        void gen_if_pred(const NodeIfPred* if_pred, const std::string& end_label) {
            struct IfPredVisitor {
                Generator& gen;
                const std::string& end_label;

                void operator()(const NodeIfPredElif* elif) const {
                    gen.gen_expr(elif->expr);
                    gen.pop("rax");
                    std::string label = gen.gen_label();
                    gen.output << "    test rax, rax\n";
                    gen.output << "    jz " << label << "\n";
                    gen.gen_scope(elif->scope);
                    gen.output << "    jmp " << end_label << "\n";
                    if (elif->pred.has_value()) {
                        gen.output << label << ":\n";
                        gen.gen_if_pred(elif->pred.value(), end_label);
                    }

                }
                void operator()(const NodeIfPredElse* else_) {
                    gen.gen_scope(else_->scope);
                }
            };
            IfPredVisitor visitor { .gen = *this, .end_label = end_label };
            std::visit(visitor, if_pred->var);
        }

        inline void gen_stmt(const NodeStmt* stmt) {
            struct StmtVisitor {
                Generator& gen;
                void operator()(const NodeStmtRet* stmt_ret) const {
                    gen.gen_expr(stmt_ret->expr);
                    gen.output << "    mov rax, 60\n";
                    gen.pop("rdi");
                    gen.output << "    syscall\n";
                }

                void operator()(const NodeStmtLet* stmt_let) const {
                    auto itr = std::find_if(
                        gen.vars.cbegin(),
                        gen.vars.cend(),
                        [&](const Var& var) { return var.name == stmt_let->ident.value.value(); });
                    if (itr != gen.vars.cend()) {
                        std::cerr << "Identifier already declared: " << stmt_let->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen.vars.push_back({ .name = stmt_let->ident.value.value(), .stack_loc = gen.stack_size });
                    gen.gen_expr(stmt_let->expr);
                }
                void operator()(const NodeScope* scope) const {
                    gen.gen_scope(scope);
                }
                void operator()(const NodeStmtIf* stmt_if) const {
                    gen.gen_expr(stmt_if->expr);
                    gen.pop("rax");
                    std::string label = gen.gen_label();
                    gen.output << "    test rax, rax\n";
                    gen.output << "    jz " << label << "\n";
                    gen.gen_scope(stmt_if->scope);
                    if (stmt_if->pred.has_value()) {
                        const std::string end_label = gen.gen_label();
                        gen.output << "    jmp " << end_label << "\n";
                        gen.output << label << ":\n";
                        gen.gen_if_pred(stmt_if->pred.value(), end_label);
                        gen.output << end_label;
                    } else {
                        gen.output << label << ":\n";
                    }
                }
                void operator()(const NodeStmtAssign* stmt_assign) const {
                    auto itr = std::find_if(
                        gen.vars.cbegin(),
                        gen.vars.cend(),
                        [&](const Var& var) { return var.name == stmt_assign->ident.value.value(); });
                    if (itr == gen.vars.cend()) {
                        std::cerr << "Undeclared Identifier: " << stmt_assign->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen.gen_expr(stmt_assign->expr);
                    gen.output << "    mov [rsp + " << (gen.stack_size - itr->stack_loc - 1) * 8 << "], rax\n";
                }
            };
            StmtVisitor visitor { .gen = *this };
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

        void begin_scope() {
            scopes.push_back(vars.size());
        }

        void end_scope() {
            size_t pop_count = vars.size() - scopes.back();
            output << "    add rsp, " << pop_count * 8 << "\n";
            stack_size -= pop_count;
            for (int i = 0; i < pop_count; i++) {
                vars.pop_back();
            }
            scopes.pop_back();
        }

        std::string gen_label() {
            std::stringstream ss;
            ss << "label" << label_count++;
            return ss.str();
        }

        struct Var {
            std::string name;
            size_t stack_loc;
        };
        const NodeProg prog;
        std::stringstream output;
        size_t stack_size = 0;
        std::vector<Var> vars {};
        std::vector<size_t> scopes {};
        int label_count = 0;
};

