#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
#include <string>
#include "./generation.hpp"

int main(int argc, char* argv[]){
  std::string file_name = argv[1];
  bool is_hydro_file = false;
  if (file_name.substr(file_name.find_last_of(".") + 1) == "hy") {
    is_hydro_file = true;
  }
  else {
    is_hydro_file = false;
    std::cerr << "Incorrect file type. File type must be .hy" << std::endl;
    std::cerr << "Correct usage is..." << std::endl;
    std::cerr << "hydro <input.hy>" << std::endl;
  }

  if (argc != 2){
    std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
    std::cerr << "hydro <input.hy>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string contents;
  {
    // Reading the file to compile
    std::stringstream contents_stream;
    std::fstream input(argv[1], std::ios::in);
    contents_stream << input.rdbuf();
    contents = contents_stream.str();
  }
  
  //Start lexing the file
  Tokenizer tokenizer(std::move(contents));
  std::vector<Token> tokens = tokenizer.tokenize();

  Parser parser(std::move(tokens));
  std::optional<NodeProg> prog = parser.parse_prog();

  if (!prog.has_value()) {
    std::cerr << "Invalid program" << std::endl;
    exit(EXIT_FAILURE);
  }

  // write the assembly code to a file
  Generator generator(prog.value());
  {
    std::fstream file("out.asm", std::ios::out);
    file << generator.gen_prog();
  }

  // execute the assembly code
  system("nasm -felf64 out.asm");
  system("ld -o out out.o");

  return EXIT_SUCCESS;
}