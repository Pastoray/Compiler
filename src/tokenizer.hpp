#pragma once

#include <iostream>
#include <vector>
#include <optional>

enum class TokenType {
    _return,
    _int,
    _semi,
    _open_paren,
    _close_paren,
    _ident,
    _let,
    _eq
};

struct Token {
    TokenType type;
    std::optional<std::string> value;
};

class Tokenizer {
    public:
        inline explicit Tokenizer(const std::string str) : src(std::move(str)), index(0) {}
        
        [[nodiscard]] inline std::vector<Token> tokenize() {
            std::vector<Token> tokens;
            std::string buff;
            while (peek().has_value()) {
                if (std::isspace(peek().value())) {
                    consume();
                } else if (std::isalpha(peek().value())) {
                    buff.push_back(consume());
                    while (peek().has_value() && std::isalnum(peek().value())) {
                        buff.push_back(consume());
                    }
                    if (buff == "return") {
                        tokens.push_back({ .type = TokenType::_return });
                    } else if (buff == "let") {
                        tokens.push_back({ .type = TokenType::_let });
                    } else {
                        tokens.push_back({ .type = TokenType::_ident, .value = buff });
                    }
                } else if (std::isdigit(peek().value())) {
                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buff.push_back(consume());
                    }
                    tokens.push_back({ .type = TokenType::_int, .value = buff });
                } else if (peek().value() == '=') {
                    consume();
                    tokens.push_back({ .type = TokenType::_eq });
                } else if (peek().value() == ';') {
                    consume();
                    tokens.push_back({ .type = TokenType::_semi });
                } else if (peek().value() == '(') {
                    consume();
                    tokens.push_back({ .type = TokenType::_open_paren });
                } else if (peek().value() == ')') {
                    consume();
                    tokens.push_back({ .type = TokenType::_close_paren });
                }
                buff.clear();
            }
            index = 0;
            return tokens;
        };

    private:
        const std::string src;
        size_t index;
        [[nodiscard]] inline std::optional<char> peek(int offset = 0) const {
            if (index + offset >= src.length()) {
                return {};
            }
            return src.at(index + offset);
        }

        inline char consume() {
            return src.at(index++);
        }
};
