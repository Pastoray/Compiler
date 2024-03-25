#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "./tokenizer.hpp"
#include "./generator.hpp"
#include "./parser.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect Usage!" << std::endl;
        std::cerr << "Correct Usage : ./main.exe <input.ps>" << std::endl;
        return EXIT_FAILURE;
    }
    
    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();

    Parser parser(std::move(tokens));
    std::optional<NodeProg> prog = parser.parse_prog();

    if (!prog.has_value()) {
        std::cerr << "No return statement found" << std::endl;
        exit(EXIT_FAILURE);
    };

    Generator generator(prog.value());
    std::string code = generator.gen_prog();

    {
        std::fstream file("../output.asm", std::ios::out);
        file << code;
    }

    system("nasm -felf64 ../output.asm");
    system("ld -o ../output ../output.o");

    return EXIT_SUCCESS;
}
