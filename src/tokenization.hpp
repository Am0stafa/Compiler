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

// In summary, this file encapsulates the logic for lexical analysis within a compiler project, defining the structure of tokens, the process of tokenization, and some utility functions for handling binary operator precedence and character examination in the source string.


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
  let,
  eq,
  plus,
  star,
  minus,
  fslash,
  open_curly,
  close_curly,
  if_
};

std::optional<int> bin_prec(TokenType type)
{
  switch (type) {
  case TokenType::minus:
  case TokenType::plus:
    return 0;
  case TokenType::fslash:
  case TokenType::star:
    return 1;
  default:
    return {};
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
      if (std::isalpha(peek().value())) {
          buf.push_back(consume());
          // put all letters in buffer
          while (peek().has_value() && std::isalnum(peek().value())) {
            buf.push_back(consume());
          }
          if (buf == "exit") {
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
      // if not a digit, check if it is a (
      else if (peek().value() == '(') {
        consume();
        tokens.push_back({ .type = TokenType::open_paren });
      }
      // if not a (, check if it is a )
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