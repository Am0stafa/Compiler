// This file define data structures by defining the necessary data structures for the AST and implementing a parser to construct the AST from a sequence of tokens.  This parser utilizes a recursive descent parsing strategy, operator precedence parsing, and efficient memory allocation via an arena allocator to parse the programming language constructs into an AST. It works like the tokenizer but instead of going character by character, it goes token by token peeking tokens and then consuming them. It also has a recursive descent parsing strategy, operator precedence parsing, and efficient memory allocation via an arena allocator to parse the programming language constructs into an AST.

#pragma once
#include <cassert>
#include <variant>
#include "./arena.hpp"
#include "tokenization.hpp"
#include <string>
#include <memory>
#include <vector>

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
    NodeExpr* lhs; // left
    NodeExpr* rhs; // right
};

// Node representing a binary MULTIPLICATION expression
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

// Node representing a binary expression ==
struct NodeBinExprEq {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// Node representing a binary expression &&
struct NodeBinExprAnd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// Node representing a binary expression ||
struct NodeBinExprOr {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// Node representing a scope for while loop
struct NodeStmtWhile {
    NodeExpr* expr;
    NodeScope* scope;
};

// Node representing a string literal
struct NodeStringLit {
    std::string value;
};

// Node representing a boolean literal true or false constants
struct NodeBoolLit {
    bool value;
};

// Node representing a for loop
struct NodeStmtFor {
    NodeExpr* init;
    NodeExpr* condition;
    NodeExpr* iteration;
    NodeScope* scope;
};
struct NodeFuncDef {
    Token ident;                // Function name identifier
    std::vector<Token> params;  // Parameter identifiers
    NodeScope* body;            // Function body
};

struct NodeFuncCall {
    Token ident;                // Function name identifier
    std::vector<NodeExpr*> args; // Arguments in the function call
};

struct NodeParamList {
    std::vector<Token> params;  // Parameter identifiers
};

struct NodeReturn {
    NodeExpr* expr;             // Expression to return
};

struct NodeStmtPrint {
    NodeExpr* expr; // Expression to be printed
};

// Node representing any binary expression
struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMulti*, NodeBinExprSub*, NodeBinExprDiv*, NodeBinExprEq*, NodeBinExprAnd*, NodeBinExprOr*> var;
};

// Node representing a terminal symbol in an expression
struct NodeTerm {
    std::variant<std::shared_ptr<NodeTermIntLit>, std::shared_ptr<NodeTermIdent>, std::shared_ptr<NodeStringLit>> var;
};

// Node representing an expression
struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

// Node representing an exit statement
struct NodeStmtExit {
    NodeExpr* expr;
};

// Node representing an else statement
struct NodeStmtElse {
    NodeScope* scope;
};

// Node representing an else if statement
struct NodeStmtElseIf {
    NodeExpr* expr;
    NodeScope* scope;
    NodeStmtElseIf* next; // For chaining else if clauses
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

// Node representing a boolean literal true or false constants
struct NodeBoolLit {
    bool value;
};

class Parser {
public:
  inline explicit Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))
    , m_allocator(1024 * 1024 * 4) // 4 mb memory to allocate memory for the AST nodes
  {
  }
  std::optional<NodeFuncDef*> parse_func_def() {
      auto func_token = expect(TokenType::function, "Expected 'function' keyword");

      auto ident_token = expect(TokenType::ident, "Expected function name identifier");
      auto params = parse_param_list();
      auto body = parse_scope();

      auto func_def = m_allocator.alloc<NodeFuncDef>();
      func_def->ident = ident_token.value();
      func_def->params = std::move(params);
      func_def->body = body.value();

      return func_def;
  }
  
  bool is_string_expression(const NodeExpr* expr) {
      if (!expr || !expr->term) {
          return false; // Return false if expr or its term is nullptr
      }

      // Check if the term is a string literal
      return std::holds_alternative<std::shared_ptr<NodeStringLit>>(expr->term->var);
  }

  std::vector<Token> parse_param_list() {
    expect(TokenType::open_paren, "Expected '(' for parameter list");
    std::vector<Token> params;

    while (peek().has_value() && peek()->type != TokenType::close_paren) {
        auto param = expect(TokenType::ident, "Expected parameter identifier");
        params.push_back(param.value());

        if (peek().has_value() && peek()->type == TokenType::comma) {
            consume(); // Consume comma
        }
    }

    expect(TokenType::close_paren, "Expected ')' after parameters");
    return params;
  }

  std::optional<NodeFuncCall*> parse_func_call() {
    auto ident_token = expect(TokenType::ident, "Expected function name identifier");

    expect(TokenType::open_paren, "Expected '(' for function call");
    std::vector<NodeExpr*> args;

    while (peek().has_value() && peek()->type != TokenType::close_paren) {
        auto arg = parse_expr();
        if (!arg.has_value()) {
            return {}; // Error handling
        }
        args.push_back(arg.value());

        if (peek().has_value() && peek()->type == TokenType::comma) {
            consume(); // Consume comma
        }
    }

    expect(TokenType::close_paren, "Expected ')' after function call arguments");

    auto func_call = m_allocator.alloc<NodeFuncCall>();
    func_call->ident = ident_token.value();
    func_call->args = std::move(args);

    return func_call;
  }


  std::optional<NodeTerm*> parse_term(){
      if (auto token = try_consume(TokenType::true_)) {
        auto node = m_allocator.alloc<NodeBoolLit>();
        node->value = true;
        auto term = m_allocator.alloc<NodeTerm>();
        term->var = node;
        return term;
      }
      else if (auto token = try_consume(TokenType::false_)) {
        auto node = m_allocator.alloc<NodeBoolLit>();
        node->value = false;
        auto term = m_allocator.alloc<NodeTerm>();
        term->var = node;
        return term;
      }
      else if (auto int_lit = try_consume(TokenType::int_lit)) {
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
      else if (auto func_def = parse_func_def()) {
        auto stmt = m_allocator.alloc<NodeStmt>();
        stmt->var = func_def.value();
        return stmt;
      }
      else {
        return {};
      }
  }
 
  // Parses an expression with optional binary operations, adhering to a specified minimum precedence.
  // The function will parse a left-hand side term and then look ahead to determine if a binary operator follows.
  // If a binary operator is found and its precedence is greater or equal to `min_prec`, the function will recursively parse the right-hand side expression with a higher precedence. This process ensures that the 
  // expression is parsed according to operator precedence rules. If a binary operator is not followed by a valid
  // expression, the function reports an error and terminates. The function returns an optional pointer to a 
  // `NodeExpr` which may be an expression or a binary expression depending on the input. If parsing fails 
  // at any point, an empty optional is returned.
  //create the full trie to make the code generation for it
  std::optional<NodeExpr*> parse_expr(int min_prec = 0){
    // Check if the current token sequence represents a function call
    if (peek().has_value() && peek()->type == TokenType::ident && 
        peek(1).has_value() && peek(1)->type == TokenType::open_paren) {
      return parse_func_call(); // parse_func_call handles the parsing of function calls
    }
    
    std::optional<NodeTerm*> term_lhs = parse_term();
    if (!term_lhs.has_value()) {
      return {}; 
    }
    // before the loop, we have a term
    auto expr_lhs = m_allocator.alloc<NodeExpr>();
    expr_lhs->var = term_lhs.value();

    // we need to see if as an expresion has a binary operator can be only a single term
    while (true) {
    std::optional<Token> curr_tok = peek(); // optional token
    std::optional<int> prec; // optional precedence

    if (curr_tok.has_value()) { // if it doesn't have a value then we break out of the loop
      prec = bin_prec(curr_tok->type);
      if (!prec.has_value() || prec < min_prec) { // if it doesn't have a value or the precedence is less than the min prec then we break out of the loop
        break;
      }
    }
    else { // no value then we break out of the loop
      break;
    }

    Token op = consume(); // consume the next token
    int next_min_prec = prec.value() + 1; // the next precedence
    auto expr_rhs = parse_expr(next_min_prec);

    if (!expr_rhs.has_value()) {
      std::cerr << "\033[31mUnable to parse expression\033[0m" << std::endl;
      exit(EXIT_FAILURE);
    }

    auto expr = m_allocator.alloc<NodeBinExpr>();
    auto expr_lhs2 = m_allocator.alloc<NodeExpr>(); // to avoid the dangling pointer

    // know the expression type of the next token. The lhs and rhs put them together
    // Specifically, it checks whether the current operator token (`op`) is of type `TokenType::plus`,
    // indicating an addition operation between two expressions. If the token matches, it creates a new `NodeBinExprAdd` node,
    // which represents a binary addition expression in the AST. This new node will have its `lhs` (left-hand side) and `rhs`
    // (right-hand side) properties set to the previously parsed left expression (`expr_lhs2`) and the right expression (`expr_rhs`) respectively.
    // The left expression (`expr_lhs2`) is also updated to inherit the variable from the original left expression (`expr_lhs`).
    // Finally, the variable of the encompassing expression (`expr`) is set to the newly created `NodeBinExprAdd` node,
    // effectively building the AST subtree for this binary expression and preparing the parser to continue parsing any further expressions.
    if (op.type == TokenType::plus) { // know the type of the next token
      auto add = m_allocator.alloc<NodeBinExprAdd>();
      expr_lhs2->var = expr_lhs->var; // create a new expression here and take the var out of it
      add->lhs = expr_lhs2;
      add->rhs = expr_rhs.value();
      expr->var = add;
    }
    else if (op.type == TokenType::eq_eq) {
        auto eq = m_allocator.alloc<NodeBinExprEq>();
        expr_lhs2->var = expr_lhs->var;
        eq->lhs = expr_lhs2;
        eq->rhs = expr_rhs.value();
        expr->var = eq;
    }
    else if (op.type == TokenType::and_and) {
        auto andExpr = m_allocator.alloc<NodeBinExprAnd>();
        expr_lhs2->var = expr_lhs->var;
        andExpr->lhs = expr_lhs2;
        andExpr->rhs = expr_rhs.value();
        expr->var = andExpr;
    } else if (op.type == TokenType::or_or) {
        auto orExpr = m_allocator.alloc<NodeBinExprOr>();
        expr_lhs2->var = expr_lhs->var;
        orExpr->lhs = expr_lhs2;
        orExpr->rhs = expr_rhs.value();
        expr->var = orExpr;
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
    // reset the left hand side to the expression
    expr_lhs->var = expr;

    }
    
    // once we are done with the loop, we return the expression which is in the left hand side
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

  std::optional<NodeStmtElseIf*> parse_else_if_chain() {
    auto expr = parse_expr();
    if (!expr.has_value()) {
        std::cerr << "Expected an expression after 'else if'" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto scope = parse_scope();
    if (!scope.has_value()) {
        std::cerr << "Expected a scope after 'else if' condition" << std::endl;
        exit(EXIT_FAILURE);
    }

    auto stmt_else_if = m_allocator.alloc<NodeStmtElseIf>();
    stmt_else_if->expr = expr.value();
    stmt_else_if->scope = scope.value();
    stmt_else_if->next = nullptr;

    // Check for a chained else if
    if (peek().has_value() && peek().value().type == TokenType::else_if) {
        stmt_else_if->next = parse_else_if_chain().value();
    }

    return stmt_else_if;
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
        std::cerr << "Invalid scope {}" << std::endl;
        exit(EXIT_FAILURE);
      }
      auto stmt = m_allocator.alloc<NodeStmt>();
      stmt->var = stmt_if;
      return stmt;
    }
    
    else if (auto else_token = try_consume(TokenType::else_)) {
        auto scope = parse_scope();
        if (!scope.has_value()) {
            std::cerr << "Expected a scope after 'else'" << std::endl;
            exit(EXIT_FAILURE);
        }
        auto stmt_else = m_allocator.alloc<NodeStmtElse>();
        stmt_else->scope = scope.value();

        auto stmt = m_allocator.alloc<NodeStmt>();
        stmt->var = stmt_else;
        return stmt;
    }
    
    else if (auto else_if_token = try_consume(TokenType::else_if)) {
        auto stmt_else_if = parse_else_if_chain();
        if (!stmt_else_if.has_value()) {
            std::cerr << "Invalid else if chain" << std::endl;
            exit(EXIT_FAILURE);
        }

        auto stmt = m_allocator.alloc<NodeStmt>();
        stmt->var = stmt_else_if.value();
        return stmt;
    }

    else if (auto while_token = try_consume(TokenType::while_)) {
        auto expr = parse_expr();
        if (!expr.has_value()) {
            std::cerr << "Expected an expression after 'while'" << std::endl;
            exit(EXIT_FAILURE);
        }
        auto scope = parse_scope();
        if (!scope.has_value()) {
            std::cerr << "Expected a scope after 'while' condition" << std::endl;
            exit(EXIT_FAILURE);
        }

        auto stmt_while = m_allocator.alloc<NodeStmtWhile>();
        stmt_while->expr = expr.value();
        stmt_while->scope = scope.value();

        auto stmt = m_allocator.alloc<NodeStmt>();
        stmt->var = stmt_while;
        return stmt;
    }

    else if (auto for_token = try_consume(TokenType::for_)) {
        try_consume(TokenType::open_paren, "Expected `(` after 'for'");
        auto init = parse_expr(); // Assuming initialization is an expression
        try_consume(TokenType::semi, "Expected `;` after initialization");
        auto condition = parse_expr();
        try_consume(TokenType::semi, "Expected `;` after condition");
        auto iteration = parse_expr();
        try_consume(TokenType::close_paren, "Expected `)` after iteration");

        auto scope = parse_scope();
        if (!scope.has_value()) {
            std::cerr << "Expected a scope after 'for' loop" << std::endl;
            exit(EXIT_FAILURE);
        }

        auto stmt_for = m_allocator.alloc<NodeStmtFor>();
        stmt_for->init = init.value();
        stmt_for->condition = condition.value();
        stmt_for->iteration = iteration.value();
        stmt_for->scope = scope.value();

        auto stmt = m_allocator.alloc<NodeStmt>();
        stmt->var = stmt_for;
        return stmt;
    }
    
    else if (auto token = try_consume(TokenType::string_lit)) {
      auto node = m_allocator.alloc<NodeStringLit>();
      node->value = token->value.value(); // Assuming token->value holds the string
      return node;
    }

    else if (auto token = try_consume(TokenType::true_)) {
      auto node = m_allocator.alloc<NodeBoolLit>();
      node->value = true;
      return node;
    }

    else if (auto token = try_consume(TokenType::false_)) {
        auto node = m_allocator.alloc<NodeBoolLit>();
        node->value = false;
        return node;
    }
    else if (auto print_token = try_consume(TokenType::print)) {
      // Expecting an expression after 'print'
      auto expr = parse_expr();
      if (!expr.has_value()) {
        std::cerr << "Expected expression after 'print'" << std::endl;
        exit(EXIT_FAILURE);
      }

      // Expecting a semicolon after the print expression
      try_consume(TokenType::semi, "Expected `;` after print statement");

      auto stmt_print = m_allocator.alloc<NodeStmtPrint>();
      stmt_print->expr = expr.value();
      
      auto stmt = m_allocator.alloc<NodeStmt>();
      stmt->var = stmt_print;
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