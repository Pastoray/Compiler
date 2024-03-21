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
    _eq,
    _plus,
    _mult,
    _sub,
    _div
};

std::optional<int> bin_prec(TokenType type) {
    switch(type) {
        case TokenType::_sub:
        case TokenType::_plus:
            return 0;
        case TokenType::_div:
        case TokenType::_mult:
            return 1;
        default:
            return {};
    }
}

struct Token {
    TokenType type;
    std::optional<std::string> value;
};

class Tokenizer {
    public:
        inline explicit Tokenizer(const std::string str) : src(std::move(str)) {}
        
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
                } else if (peek().value() == '+') {
                    consume();
                    tokens.push_back({ .type = TokenType::_plus });
                } else if (peek().value() == '*') {
                    consume();
                    tokens.push_back({ .type = TokenType::_mult });
                } else if (peek().value() == '-') {
                    consume();
                    tokens.push_back({ .type = TokenType::_sub });
                } else if (peek().value() == '/') {
                    consume();
                    tokens.push_back({ .type = TokenType::_div });
                }
                buff.clear();
            }
            index = 0;
            return tokens;
        };

    private:
        [[nodiscard]] inline std::optional<char> peek(int offset = 0) const {
            if (index + offset >= src.length()) {
                return {};
            }
            return src.at(index + offset);
        }

        inline char consume() {
            return src.at(index++);
        }
        const std::string src;
        size_t index = 0;
};
