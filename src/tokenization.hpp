// This file task is lexical analysis or tokenization which processing individual
// The key components and their functions in this file:

// 1.Token Enumeration (`TokenType`):
//    - This enumeration defines the different types of tokens that the tokenizer can recognize. These tokens are representations of syntactical elements in the source code such as keywords (`exit`, `let`, `if`), literals (`int_lit` for integer literals), operators (`plus`, `minus`, `star`, `fslash` for addition, subtraction, multiplication, and division respectively), and punctuation symbols (`semi`, `open_paren`, `close_paren`, `open_curly`, `close_curly` for semicolon, opening parenthesis, closing parenthesis, opening curly brace, and closing curly brace respectively).

// 2.Binary Precedence Function (`bin_prec`):
//    - This function is used to determine the precedence of binary operators. The precedence is used during parsing to resolve ambiguities in expressions.

// 3.Token Structure (`Token`):
//    - The Token structure holds information about a token, including its type and optionally its value. This allows for the representation of both operators and literals within the same structure.

// 4.Tokenizer Class (`Tokenizer`):
//    - This class is responsible for the process of tokenization, which is breaking down the input source code into a series of tokens.
//    - The constructor takes a string representing the source code to be tokenized.
//    - The `tokenize` method performs the tokenization process, iterating through the source code, recognizing tokens based on their syntactic categories, and collecting them into a vector<Token> which is returned.
//    - The `peek` and `consume` methods are utility functions used in the process of analyzing the source code. `peek` looks ahead in the source code without advancing the position, while `consume` advances the position and returns the character at the current position, it is like i++.
//    - Private members `m_src` and `m_index` hold the source code string and the current position within that string respectively.

//How the tokens will look like
// 1. For the line `let y = (10 - 2 * 3) / 2;`:
//     - Token { type: TokenType::let }
//     - Token { type: TokenType::ident, value: "y" }
//     - Token { type: TokenType::eq }
//     - Token { type: TokenType::open_paren }
//     - Token { type: TokenType::int_lit, value: "10" }
//     - Token { type: TokenType::minus }
//     - Token { type: TokenType::int_lit, value: "2" }
//     - Token { type: TokenType::star }
//     - Token { type: TokenType::int_lit, value: "3" }
//     - Token { type: TokenType::close_paren }
//     - Token { type: TokenType::fslash }
//     - Token { type: TokenType::int_lit, value: "2" }
//     - Token { type: TokenType::semi }

// 2. For the line `let x = 1;`:
//     - Token { type: TokenType::let }
//     - Token { type: TokenType::ident, value: "x" }
//     - Token { type: TokenType::eq }
//     - Token { type: TokenType::int_lit, value: "1" }
//     - Token { type: TokenType::semi }

// These tokens represent the syntactic elements found in the source code, and are categorized based on their types, such as keywords, identifiers, operators, literals, and punctuation symbols. They are ready to be fed into the next stage of the compilation process, which is parsing.


#pragma once // include the file only once
#include <string>
#include <vector>

// Syntax
enum class TokenType{
  exit, // return
  int_lit,
  semi,
  open_paren,
  close_paren,
  ident,
  let, // variable declaration
  eq,
  plus,
  star, // multiplication
  minus,
  fslash, // division forward slash
  // for scoping
  open_curly,
  close_curly,
  if_, // as if is a keyword in C++
  true_,
  false_,
  eq_eq, // ==
};

// check the precedence of binary operators and return the precedence of each. Basically return the precedence of the operator
std::optional<int> bin_prec(TokenType type){
  switch (type) {
  case TokenType::minus:
  case TokenType::plus:
    return 0;
  case TokenType::fslash:
  case TokenType::star:
    return 1;
  default:
    return {}; // return null (not a binary operator)
  }
}

struct Token {
  TokenType type;
  std::optional<std::string> value {};
};

class Tokenizer {
public:
  inline explicit Tokenizer(std::string src)
      : m_src(std::move(src))
  {
  }

  inline std::vector<Token> tokenize()

  {
    // token array
    std::vector<Token> tokens;
    std::string buf;
    // loop until when we peek there is no more character
    while (peek().has_value()) {
      // inside the loop consume the tokens

        // Handle Single-Line Comments
      if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
          consume(); // Consume first '/'
          consume(); // Consume second '/'
          while (peek().has_value() && peek().value() != '\n') {
              consume(); // Consume characters until end of line
          }
          continue; // Skip to next iteration after comment
      }

      // Handle Block Comments
      if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
          consume(); // Consume '/'
          consume(); // Consume '*'
          while (peek().has_value() && !(peek().value() == '*' && peek(1).value() == '/')) {
              consume(); // Consume characters inside comment block
          } 
          if (peek().has_value()) {
              consume(); // Consume closing '*'
              consume(); // Consume closing '/'
          }
          continue; // Skip to next iteration after comment
      }
      // Check for '==' operator
      if (peek().value() == '=' && peek(1).value() == '=') {
          consume(); // Consume first '='
          consume(); // Consume second '='
          tokens.push_back({ .type = TokenType::eq_eq });
          continue;
      }

      if (std::isalpha(peek().value())) {
          buf.push_back(consume());
          // put all letters in buffer
          while (peek().has_value() && std::isalnum(peek().value())) {
            buf.push_back(consume());
          }
          if (buf == "true") {
            tokens.push_back({ .type = TokenType::true_ });
            buf.clear();
          } else if (buf == "false") {
            tokens.push_back({ .type = TokenType::false_ });
            buf.clear();
          }else if (buf == "exit") {
            tokens.push_back({ .type = TokenType::exit });
            buf.clear();
          }
          else if (buf == "let") {
            tokens.push_back({ .type = TokenType::let });
            buf.clear();
          }
          else if (buf == "if") {
            tokens.push_back({ .type = TokenType::if_ });
            buf.clear();
          }
          else {
            // variable name
            tokens.push_back({ .type = TokenType::ident, .value = buf });
            buf.clear();
          }
      }
      // if not a letter, check if it is a digit
      else if (std::isdigit(peek().value())) {
        buf.push_back(consume());
        // put all digits in buffer
        while (peek().has_value() && std::isdigit(peek().value())) {
          buf.push_back(consume());
        }
        tokens.push_back({ .type = TokenType::int_lit, .value = buf });
        buf.clear();
      }
      // if not a digit, check if it is a bracket of type (
      else if (peek().value() == '(') {
        consume();
        tokens.push_back({ .type = TokenType::open_paren });
      }
      // if not a (, check if it is a bracket of type )
      else if (peek().value() == ')') {
        consume();
        tokens.push_back({ .type = TokenType::close_paren });
      }
      // if not a ), check if it is a ;
      else if (peek().value() == ';') {
        consume();
        tokens.push_back({ .type = TokenType::semi });
      }
      // if not a ;, check if it is a =
      else if (peek().value() == '=') {
        consume();
        tokens.push_back({ .type = TokenType::eq });
      }
      else if (peek().value() == '+') {
        consume();
        tokens.push_back({ .type = TokenType::plus });
      }
      else if (peek().value() == '*') {
        consume();
        tokens.push_back({ .type = TokenType::star });
      }
      else if (peek().value() == '-') {
        consume();
        tokens.push_back({ .type = TokenType::minus });
      }
      else if (peek().value() == '/') {
        consume();
        tokens.push_back({ .type = TokenType::fslash });
      }
      else if (peek().value() == '{') {
        consume();
        tokens.push_back({ .type = TokenType::open_curly });
      }
      else if (peek().value() == '}') {
        consume();
        tokens.push_back({ .type = TokenType::close_curly });
      }
      else if (std::isspace(peek().value())) {
        consume();
      }
      else {
        std::cerr << "\033[31mSyntax error\033[0m" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    m_index = 0;
    return tokens;
  }

private:
    // peak method
    [[nodiscard]] inline std::optional<char> peek(int offset = 0) const
    {
        if (m_index + offset >= m_src.length()) {
          return {};
        }
        else {
          return m_src.at(m_index + offset); // not using [] to get type checking
        }
    }

    // consume method
    inline char consume()
    {
      return m_src.at(m_index++);
    }

    const std::string m_src;
    size_t m_index = 0;
};