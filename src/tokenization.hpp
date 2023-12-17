// This file task is lexical analysis or tokenization which processing individual

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
  else_,
  else_if,
  true_,
  false_,
  eq_eq, // ==
  and_and,  // for '&&' operator
  or_or,    // for '||' operator
  while_,
  for_,
  string_lit, 
  bool_lit,
  function,  
  return_,
  comma,
  print,
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

      // Check for '&&' operator
      if (peek().value() == '&' && peek(1).value() == '&') {
        consume(); // Consume first '&'
        consume(); // Consume second '&'
        tokens.push_back({ .type = TokenType::and_and });
        continue;
      }
      
      // Handle string literals
      if (peek() == '"') {
          consume(); // Consume the opening quote
          std::string str_val;
          bool is_escaped = false; // Track escaped state

          while (peek().has_value()) {
              char current_char = peek();

              if (is_escaped) {
                  // Handle various escape sequences
                  switch (current_char) {
                      case 'n': str_val.push_back('\n'); break;
                      case 't': str_val.push_back('\t'); break;
                      case '"': str_val.push_back('"'); break;
                      case '\\': str_val.push_back('\\'); break;
                      // ... other escape sequences as needed ...
                      default: 
                          // Handle unknown escape sequences
                          std::cerr << "Unknown escape sequence: \\" << current_char << std::endl;
                          exit(EXIT_FAILURE);
                  }
                  consume(); // Consume the escaped character
                  is_escaped = false; // Reset escaped state
              } else {
                  if (current_char == '\\') {
                      is_escaped = true; // Next character is escaped
                      consume(); // Consume the backslash
                  } else if (current_char == '"') {
                      consume(); // Consume the closing quote
                      break; // End of string literal
                  } else {
                      str_val.push_back(current_char); // Add character to string
                      consume(); // Consume the character
                  }
              }
          }

          if (!peek().has_value() || peek() != '"') {
              // Handle error: unclosed string literal
              std::cerr << "Syntax error: unclosed string literal" << std::endl;
              exit(EXIT_FAILURE);
          }

          tokens.push_back({ .type = TokenType::string_lit, .value = str_val });
      }

      // Check for '||' operator
      if (peek().value() == '|' && peek(1).value() == '|') {
        consume(); // Consume first '|'
        consume(); // Consume second '|'
        tokens.push_back({ .type = TokenType::or_or });
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
          } 
          else if (buf == "false") {
            tokens.push_back({ .type = TokenType::false_ });
            buf.clear();
          }
          else if (buf == "exit") {
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
          else if (buf == "else") {
            tokens.push_back({ .type = TokenType::else_ });
            buf.clear();
          }
          else if (buf == "else if") {
            tokens.push_back({ .type = TokenType::else_if });
            buf.clear();
          }
          else if (buf == "while") {
            tokens.push_back({ .type = TokenType::while_ });
            buf.clear();
          }
          else if (buf == "for") {
            tokens.push_back({ .type = TokenType::for_ });
            buf.clear();
          }
          else if (buf == "function") {
            tokens.push_back({ .type = TokenType::function });
            buf.clear();
          }
          else if (buf == "return") {
            tokens.push_back({ .type = TokenType::return_ });
            buf.clear();
          }
          else if (buf == "true" || buf == "false") {
            TokenType type = (buf == "true") ? TokenType::true_ : TokenType::false_;
            tokens.push_back({ .type = type });
            buf.clear();
          }
          else if (buf == "print") {
            tokens.push_back({ .type = TokenType::print });
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
      else if (peek() == ',') {
        consume();
        tokens.push_back({ .type = TokenType::comma });
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