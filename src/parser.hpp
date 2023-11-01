// This file define data structures by defining the necessary data structures for the AST and implementing a parser to construct the AST from a sequence of tokens.  This parser utilizes a recursive descent parsing strategy, operator precedence parsing, and efficient memory allocation via an arena allocator to parse the programming language constructs into an AST. It works like the tokenizer but instead of going character by character, it goes token by token peeking tokens and then consuming them. It also has a recursive descent parsing strategy, operator precedence parsing, and efficient memory allocation via an arena allocator to parse the programming language constructs into an AST.

// The main components and their roles:

// ### 1. Node Structures:
//    - Node structures are used to define the abstract syntax tree (AST) nodes. Each structure represents a different type of syntax element in the language being parsed. For example:
//      - `NodeTermIntLit` represents integer literals.
//      - `NodeTermIdent` represents identifiers.
//      - `NodeBinExprAdd`, `NodeBinExprMulti`, `NodeBinExprSub`, and `NodeBinExprDiv` represent binary expressions for addition, multiplication, subtraction, and division, respectively.
//      - `NodeStmtExit`, `NodeStmtLet`, `NodeStmtIf`, etc., represent different types of statements like exit, let, and if statements.
//    - These structures use `std::variant` to model the different variants a node can have, which is a common pattern for implementing ASTs in a type-safe way.

// ### 2. Parser Class:
//    - The `Parser` class is responsible for parsing a sequence of tokens into an AST nodes. It does this through a set of parsing functions like `parse_term`, `parse_expr`, `parse_stmt`, and `parse_prog`, each of which parses different parts of the syntax.
//    - Recursive Descent Parsing: The parsing functions follow a recursive descent parsing strategy, where `parse_expr` and `parse_stmt` call themselves and each other recursively to parse nested expressions and statements.
//    - Error Handling: When it encounters an unexpected token, it prints an error message to standard error and exits the program.
//    - Memory Allocation: The parser uses an `ArenaAllocator` to allocate memory for AST nodes. This is a common practice in compilers to manage memory efficiently and ensure that all allocated memory is freed when the compiler is done.

// ### 3. Memory Allocation:
//    - An `ArenaAllocator` is used for memory management, which is a common practice in compilers to manage memory efficiently and ensure that all allocated memory is freed when the compiler is done.

// ### 4. Token Handling:
//    - Token handling methods like `peek`, `consume`, and `try_consume` are used to inspect and consume tokens from the token stream as the parser processes the input.

// ### 5. Parsing Strategy:
//    - Operator Precedence Parsing: The `parse_expr` function seems to implement a form of operator precedence parsing to handle binary expressions with different levels of precedence.




#pragma once
#include <cassert>
#include <variant>
#include "./arena.hpp"
#include "tokenization.hpp"

// Token representing an integer literal
struct NodeTermIntLit {
    Token int_lit;
};

// Token representing an identifier
struct NodeTermIdent {
    Token ident;
};

// Forward declaration of expression node
struct NodeExpr;

// Node representing an expression enclosed in parentheses
struct NodeTermParen {
    NodeExpr* expr;
};

// Node representing a binary addition expression
struct NodeBinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// Node representing a binary multiplication expression
struct NodeBinExprMulti {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// Node representing a binary subtraction expression
struct NodeBinExprSub {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// Node representing a binary division expression
struct NodeBinExprDiv {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// Node representing any binary expression
struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMulti*, NodeBinExprSub*, NodeBinExprDiv*> var;
};

// Node representing a terminal symbol in an expression
struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
};

// Node representing an expression
struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

// Node representing an exit statement
struct NodeStmtExit {
    NodeExpr* expr;
};

// Node representing a let statement
struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

// Forward declaration of statement node
struct NodeStmt;

// Node representing a block scope
struct NodeScope {
    std::vector<NodeStmt*> stmts;
};

// Node representing an if statement
struct NodeStmtIf {
    NodeExpr* expr;
    NodeScope* scope;
};

// Node representing a statement
struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeScope*, NodeStmtIf*> var;
};

// Node representing the entire program
struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

class Parser {
public:
  inline explicit Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))
    , m_allocator(1024 * 1024 * 4) // 4 mb memory
  {
  }

  std::optional<NodeTerm*> parse_term(){
      if (auto int_lit = try_consume(TokenType::int_lit)) {
          auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
          term_int_lit->int_lit = int_lit.value();
          auto term = m_allocator.alloc<NodeTerm>();
          term->var = term_int_lit;
          return term;
      }
      else if (auto ident = try_consume(TokenType::ident)) {
          auto expr_ident = m_allocator.alloc<NodeTermIdent>();
          expr_ident->ident = ident.value();
          auto term = m_allocator.alloc<NodeTerm>();
          term->var = expr_ident;
          return term;
      }
      else if (auto open_paren = try_consume(TokenType::open_paren)) {
          auto expr = parse_expr();
          if (!expr.has_value()) {
              std::cerr << "Expected expression" << std::endl;
              exit(EXIT_FAILURE);
          }
          try_consume(TokenType::close_paren, "Expected `)`");
          auto term_paren = m_allocator.alloc<NodeTermParen>();
          term_paren->expr = expr.value();
          auto term = m_allocator.alloc<NodeTerm>();
          term->var = term_paren;
          return term;
      }
      else {
          return {};
      }
  }
 
  std::optional<NodeExpr*> parse_expr(int min_prec = 0){
    std::optional<NodeTerm*> term_lhs = parse_term();
    if (!term_lhs.has_value()) {
      return {}; 
    }
    auto expr_lhs = m_allocator.alloc<NodeExpr>();
    expr_lhs->var = term_lhs.value();

    while (true) {
      std::optional<Token> curr_tok = peek();
      std::optional<int> prec;

      if (curr_tok.has_value()) {
          prec = bin_prec(curr_tok->type);
          if (!prec.has_value() || prec < min_prec) {
            break;
          }
      }
      else {
        break;
      }

      Token op = consume();
      int next_min_prec = prec.value() + 1;
      auto expr_rhs = parse_expr(next_min_prec);

      if (!expr_rhs.has_value()) {
        std::cerr << "\033[31mUnable to parse expression\033[0m" << std::endl;
        exit(EXIT_FAILURE);
      }

      auto expr = m_allocator.alloc<NodeBinExpr>();
      auto expr_lhs2 = m_allocator.alloc<NodeExpr>();
      if (op.type == TokenType::plus) {
          auto add = m_allocator.alloc<NodeBinExprAdd>();
          expr_lhs2->var = expr_lhs->var;
          add->lhs = expr_lhs2;
          add->rhs = expr_rhs.value();
          expr->var = add;
      }
      else if (op.type == TokenType::star) {
          auto multi = m_allocator.alloc<NodeBinExprMulti>();
          expr_lhs2->var = expr_lhs->var;
          multi->lhs = expr_lhs2;
          multi->rhs = expr_rhs.value();
          expr->var = multi;
      }
      else if (op.type == TokenType::minus) {
        auto sub = m_allocator.alloc<NodeBinExprSub>();
        expr_lhs2->var = expr_lhs->var;
        sub->lhs = expr_lhs2;
        sub->rhs = expr_rhs.value();
        expr->var = sub;
      }
      else if (op.type == TokenType::fslash) {
        auto div = m_allocator.alloc<NodeBinExprDiv>();
        expr_lhs2->var = expr_lhs->var;
        div->lhs = expr_lhs2;
        div->rhs = expr_rhs.value();
        expr->var = div;
      }
      else {
        assert(false); // Unreachable;
      }
      expr_lhs->var = expr;
    }
    return expr_lhs;
  }

  std::optional<NodeScope*> parse_scope(){
      if (!try_consume(TokenType::open_curly).has_value()) {
          return {};
      }
      auto scope = m_allocator.alloc<NodeScope>();
      while (auto stmt = parse_stmt()) {
          scope->stmts.push_back(stmt.value());
      }
      try_consume(TokenType::close_curly, "Expected `}`");
      return scope;
  }

/**
 * The `parse_stmt` function is part of the parsing process in a compiler,
 * responsible for analyzing and transforming a segment of the input program
 * into corresponding nodes in an Abstract Syntax Tree (AST) based on the
 * types of statements present in the programming language being parsed.
 *
 * Function Signature:
 * - Returns: `std::optional<NodeStmt*>`, indicating an optional pointer to
 *   a `NodeStmt` object. An empty `std::optional` is returned if no matching
 *   statement type is found or if there's a parsing error.
 *
 * Parsing Different Statement Types:
 * - Exit Statement: Checks for an `exit` statement by looking for tokens
 *   `exit` followed by an open parenthesis. A `NodeStmtExit` object is
 *   created and an expression is parsed if present.
 * - Let Statement: Checks for a `let` statement, looking for tokens `let`
 *   followed by an identifier and an equals sign. A `NodeStmtLet` object
 *   is created and an expression is parsed.
 * - Scope: Activated when an open curly brace is found, indicating a new
 *   scope. A `NodeStmt` object is created and the scope is parsed.
 * - If Statement: Checks for an `if` statement, looking for the `if` token
 *   followed by an open parenthesis. A `NodeStmtIf` object is created,
 *   an expression and a scope are parsed.
 *
 * Error Handling:
 * - Provides error feedback through `std::cerr` and may terminate the program
 *   using `exit()` if there's an invalid expression, scope, or unexpected
 *   token encountered.
 *
 * Memory Allocation:
 * - Utilizes a custom memory allocator (`m_allocator`) for managing AST node
 *   memory during the creation of `NodeStmt`, `NodeStmtExit`, `NodeStmtLet`,
 *   and `NodeStmtIf` objects.
 *
 * Token Management:
 * - Employs `peek()` and `consume()` methods for looking ahead at upcoming
 *   tokens and consuming tokens from the token stream, respectively, aiding
 *   in the recursive descent parsing process.
 */
  std::optional<NodeStmt*> parse_stmt(){
      if (peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
        consume();
        consume();
        auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
        if (auto node_expr = parse_expr()) {
          stmt_exit->expr = node_expr.value();
        }
        else {
          std::cerr << "Invalid expression" << std::endl;
            (EXIT_FAILURE);
        }
        try_consume(TokenType::close_paren, "Expected `)`");
        try_consume(TokenType::semi, "Expected `;`");
        auto stmt = m_allocator.alloc<NodeStmt>();
        stmt->var = stmt_exit;
        return stmt;
      }

      else if ( //check that the one after that is an identifier and then the one after that is an identifier
        peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value()
        && peek(1).value().type == TokenType::ident && peek(2).has_value()
        && peek(2).value().type == TokenType::eq
              ) {
          consume();
          auto stmt_let = m_allocator.alloc<NodeStmtLet>();
          stmt_let->ident = consume();
          consume();
          // if the expression is parsed correctly
          if (auto expr = parse_expr()) {
              stmt_let->expr = expr.value();
          }
          else {
              std::cerr << "Invalid expression" << std::endl;
              exit(EXIT_FAILURE);
          }
          // check if it had a semicol
          try_consume(TokenType::semi, "Expected `;`");
          auto stmt = m_allocator.alloc<NodeStmt>();
          stmt->var = stmt_let;
          return stmt; // return a node statement
      }

      else if (peek().has_value() && peek().value().type == TokenType::open_curly) {
          if (auto scope = parse_scope()) {
              auto stmt = m_allocator.alloc<NodeStmt>();
              stmt->var = scope.value();
              return stmt;
          }
          else {
              std::cerr << "Invalid scope" << std::endl;
              exit(EXIT_FAILURE);
          }
      }

      else if (auto if_ = try_consume(TokenType::if_)) {
          try_consume(TokenType::open_paren, "Expected `(`");
          auto stmt_if = m_allocator.alloc<NodeStmtIf>();
          if (auto expr = parse_expr()) {
              stmt_if->expr = expr.value();
          }
          else {
              std::cerr << "Invalid expression" << std::endl;
              exit(EXIT_FAILURE);
          }
          try_consume(TokenType::close_paren, "Expected `)`");
          if (auto scope = parse_scope()) {
              stmt_if->scope = scope.value();
          }
          else {
              std::cerr << "Invalid scope" << std::endl;
              exit(EXIT_FAILURE);
          }
          auto stmt = m_allocator.alloc<NodeStmt>();
          stmt->var = stmt_if;
          return stmt;
      }

      else {
          return {};
      }
  }

/**
 * @brief Parses a series of statements to construct a program node of an Abstract Syntax Tree (AST).
 * 
 * This function is an essential part of the AST construction phase in a compiler, enabling the translation 
 * of source code into a structured representation for further analysis and transformation.
 * 
 * @return std::optional<NodeProg> - An optional object encapsulating the NodeProg object representing the parsed program.
 * If the parsing fails at any point, an empty std::optional object is returned.
 * 
 * Detailed Breakdown:
 * 1. Function Signature: 
 *    - Returns a std::optional<NodeProg> object encapsulating a NodeProg object or nothing (in case of parsing failure).
 * 
 * 2. Local Variables:
 *    - NodeProg prog; creates an instance of NodeProg to represent the program being parsed.
 *    
 * 3. Loop:
 *    - The while (peek().has_value()) loop iterates as long as the peek() function returns a value, suggesting that peek() 
 *      is checking the next token or element in the input stream to see if there's more to parse.
 *   
 * 4. Statement Parsing:
 *    - Within the loop, parse_stmt() is called to parse individual statements.
 *    - If parse_stmt() returns a value (i.e., a successfully parsed statement), it is added to prog.stmts using the push_back method.
 *      prog.stmts is likely a vector or similar collection within the NodeProg structure, holding all the parsed statements.
 *   
 * 5. Error Handling:
 *    - If parse_stmt() fails to return a value (i.e., fails to parse a statement), an error message "Invalid statement" is output to std::cerr,
 *      and the program exits with exit(EXIT_FAILURE), indicating a failure.
 * 
 * 6. Return Value:
 *    - Once all statements have been parsed (or an error has occurred), prog is returned wrapped in a std::optional, indicating the parsed program.
 */
  std::optional<NodeProg> parse_prog(){
    NodeProg prog;
    // keep going until we can parse anything else
    while (peek().has_value()) {
        if (auto stmt = parse_stmt()) {
          prog.stmts.push_back(stmt.value());
        }
        else {
          std::cerr << "Invalid statement" << std::endl;
          exit(EXIT_FAILURE);
        }
    }
    return prog;
  }

private:
  // same as in tokenization.hpp
  [[nodiscard]] inline std::optional<Token> peek(int offset = 0) const
  {
    if (m_index + offset >= m_tokens.size()) {
      return {};
    }
    else {
      return m_tokens.at(m_index + offset);
    }
  }
  // same as in tokenization.hpp
  inline Token consume()
  {
    return m_tokens.at(m_index++);
  }

  inline Token try_consume(TokenType type, const std::string& err_msg)
  {
      if (peek().has_value() && peek().value().type == type) {
        return consume();
      }
      else {
        std::cerr << err_msg << std::endl;
        exit(EXIT_FAILURE);
      }
  }

  inline std::optional<Token> try_consume(TokenType type)
  {
      if (peek().has_value() && peek().value().type == type) {
        return consume();
      }
      else {
        return {};
      }
  }

  const std::vector<Token> m_tokens;
  size_t m_index = 0;
  ArenaAllocator m_allocator;
};