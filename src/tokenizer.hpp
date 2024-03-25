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
    _minus,
    _fslash,
    _open_curly,
    _close_curly,
    _if,
    _elif,
    _else
};

inline std::string token_to_string(TokenType type) {
    switch(type) {
        case TokenType::_return:
            return "return";
            break;
        case TokenType::_int:
            return "int";
            break;
        case TokenType::_semi:
            return "`;`";
            break;
        case TokenType::_open_paren:
            return "`(`";
            break;
        case TokenType::_close_paren:
            return "`)`";
            break;
        case TokenType::_ident:
            return "identifier";
            break;
        case TokenType::_let:
            return "let";
            break;
        case TokenType::_eq:
            return "`=`";
            break;
        case TokenType::_plus:
            return "`+`";
            break;
        case TokenType::_mult:
            return "`*`";
            break;
        case TokenType::_minus:
            return "`-`";
            break;
        case TokenType::_fslash:
            return "`/`";
            break;
        case TokenType::_close_curly:
            return "`}`";
            break;
        case TokenType::_open_curly:
            return "`{`";
            break;
        case TokenType::_if:
            return "if";
            break;
        case TokenType::_elif:
            return "elif";
            break;
        case TokenType::_else:
            return "else";
            break;
        default:
            return {};
    }
}

std::optional<int> bin_prec(TokenType type) {
    switch(type) {
        case TokenType::_minus:
        case TokenType::_plus:
            return 0;
        case TokenType::_fslash:
        case TokenType::_mult:
            return 1;
        default:
            return {};
    };
}

struct Token {
    TokenType type;
    std::optional<std::string> value;
    int line;
};

class Tokenizer {
    public:
        inline explicit Tokenizer(const std::string str) : src(std::move(str)) {}
        
        [[nodiscard]] inline std::vector<Token> tokenize() {
            std::vector<Token> tokens;
            std::string buff;
            int line_count = 1;
            while (peek().has_value()) {
                if (peek().value() == '\n') {
                    consume();
                    line_count++;
                } else if (std::isspace(peek().value())) {
                    consume();
                } else if (std::isalpha(peek().value())) {
                    buff.push_back(consume());
                    while (peek().has_value() && std::isalnum(peek().value())) {
                        buff.push_back(consume());
                    }
                    if (buff == "return") {
                        tokens.push_back({ .type = TokenType::_return, .line = line_count });
                    } else if (buff == "let") {
                        tokens.push_back({ .type = TokenType::_let, .line = line_count });
                    } else if (buff == "if") {
                        tokens.push_back({ .type = TokenType::_if, .line = line_count });
                    } else if (buff == "elif") {
                        tokens.push_back({ .type = TokenType::_elif, .line = line_count });
                    } else if (buff == "else") {
                        tokens.push_back({ .type = TokenType::_else, .line = line_count });
                    } else {
                        tokens.push_back({ .type = TokenType::_ident, .value = buff, .line = line_count });
                    }
                } else if (std::isdigit(peek().value())) {
                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buff.push_back(consume());
                    }
                    tokens.push_back({ .type = TokenType::_int, .value = buff, .line = line_count });
                } else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
                    while (peek().has_value() && peek().value() != '\n') {
                        consume();
                    }
                } else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
                    consume();
                    consume();
                    while (peek().has_value()) {
                        if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                            break;
                        }
                        consume();
                    }
                    if (peek().has_value()) {
                        consume();
                    }
                    if (peek().has_value()) {
                        consume();
                    }
                } else if (peek().value() == '=') {
                    consume();
                    tokens.push_back({ .type = TokenType::_eq, .line = line_count });
                } else if (peek().value() == ';') {
                    consume();
                    tokens.push_back({ .type = TokenType::_semi, .line = line_count });
                } else if (peek().value() == '(') {
                    consume();
                    tokens.push_back({ .type = TokenType::_open_paren, .line = line_count });
                } else if (peek().value() == ')') {
                    consume();
                    tokens.push_back({ .type = TokenType::_close_paren, .line = line_count });
                } else if (peek().value() == '+') {
                    consume();
                    tokens.push_back({ .type = TokenType::_plus, .line = line_count });
                } else if (peek().value() == '*') {
                    consume();
                    tokens.push_back({ .type = TokenType::_mult, .line = line_count });
                } else if (peek().value() == '-') {
                    consume();
                    tokens.push_back({ .type = TokenType::_minus, .line = line_count });
                } else if (peek().value() == '/') {
                    consume();
                    tokens.push_back({ .type = TokenType::_fslash, .line = line_count });
                } else if (peek().value() == '{') {
                    consume();
                    tokens.push_back({ .type = TokenType::_open_curly, .line = line_count });
                } else if (peek().value() == '}') {
                    consume();
                    tokens.push_back({ .type = TokenType::_close_curly, .line = line_count });
                } else {
                    std::cerr << "Token" << peek().value() << "is invalid" << "line " << line_count << std::endl;
                    exit(EXIT_FAILURE); 
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
