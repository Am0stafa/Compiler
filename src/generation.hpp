// File that generates the assembly code (Code generation)
// It takes in the root node of the AST and traverses that tree and while its traversing it generates the assembly code. I have different method for generating each node type.

// It facilitate the generation of assembly code from an Abstract Syntax Tree (AST) which represents a higher-level programming language. This is a common task in compiler construction. Here's a breakdown of the significant parts of this class:

// 1. Constructor:
//    - The constructor initializes the `Generator` object with the root node of the AST (`NodeProg prog`), which it receives as an argument.

// 2. Visitors:
//    - The visitor pattern is used extensively to handle different types of nodes in the AST. Each visitor struct (`TermVisitor`, `BinExprVisitor`, `ExprVisitor`, `StmtVisitor`) contains overloads of the `operator()` method to handle different node types. This design allows for the separation of the algorithm from the object structure on which it operates.

// 3. Generation Methods:
//    - Methods like `gen_term`, `gen_bin_expr`, `gen_expr`, `gen_scope`, and `gen_stmt` are responsible for generating assembly code corresponding to different parts of the AST. They do this by creating instances of the appropriate visitor and applying them to the nodes they are responsible for.

// 4. Assembly Code Generation:
//    - Throughout these methods, assembly code is being generated and appended to a `std::stringstream` named `m_output`. The generated assembly code is typical of x86_64 assembly language, using instructions like `mov`, `sub`, `add`, `mul`, `div`, and `syscall`.

// 5. Stack Management:
//    - The `push` and `pop` methods are used to manage the stack during code generation. The `begin_scope` and `end_scope` methods handle entering and exiting scopes, adjusting the stack and cleaning up variables as necessary.

// 6. Label Generation:
//    - The `create_label` method generates unique labels for use in the assembly code, which is essential for handling control flow constructs like if-statements.

// 7. Program Generation:
//    - The `gen_prog` method orchestrates the process of generating the assembly code for the entire program. It initializes the output with a global entry point `_start`, iterates through the statements in the program, generating code for each, and finally appends the code to exit the program.

// 8. Member Variables:
//    - Various member variables like `m_prog`, `m_output`, `m_stack_size`, `m_vars`, `m_scopes`, and `m_label_count` are used to keep track of the state during code generation.

// 9. Error Handling:
//    - There's basic error handling in place, for example, checking for undeclared identifiers or re-declarations, and exiting with an error message if such issues are found.

// 10. Data Structures:
//     - Structures like `Var` and `NodeProg` presumably represent variables and program nodes, although their definitions are not provided in the snippet.

// The `Generator` class encapsulates the logic for traversing the AST and generating the corresponding assembly code, following a structured and organized approach which is modular and easy to extend for handling additional node types or generating code for different target architectures. This class is a crucial part of the back-end of a compiler, turning the high-level representation of the program into low-level assembly code that can be assembled into machine code and executed on a computer.

#pragma once
#include "parser.hpp"
#include <cassert>
#include <algorithm>

class Generator {
public:
  inline explicit Generator(NodeProg prog)
    : m_prog(std::move(prog)) // take the program out put from the parser (AST)
  {
  }

  // A generate for each AST node, just as we parse each AST node we will generate each AST node as we traverse the tree
  void gen_term(const NodeTerm* term){
  struct TermVisitor {
        Generator& gen;

        void operator()(const NodeTermIntLit* term_int_lit) const {
            gen.m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
            gen.push("rax");
        }

        // Add handling for boolean literals
        void operator()(const NodeBoolLit* bool_lit) const {
            // Convert the boolean value to an integer (0 for false, 1 for true)
            int boolValue = bool_lit->value ? 1 : 0;
            gen.m_output << "    mov rax, " << boolValue << "\n";
            gen.push("rax");
        }

    // If we need to use a variable, extract the value of the variable and put it at the top of the stack
    void operator()(const NodeTermIdent* term_ident) const{
      auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
        return var.name == term_ident->ident.value.value(); 
      });
      if (it == gen.m_vars.cend()) {
        std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
        exit(EXIT_FAILURE);
      }
      std::stringstream offset;
      // push the offset of the variable on the stack
      offset << "QWORD [rsp + " << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "]";
      gen.push(offset.str());
    }
    void operator()(const NodeTermParen* term_paren) const
    {
      gen.gen_expr(term_paren->expr);
    }
  };
    TermVisitor visitor({ .gen = *this });
    std::visit(visitor, term->var);
  }

  // Math operations
  void gen_bin_expr(const NodeBinExpr* bin_expr){
    struct BinExprVisitor {
      Generator& gen;
      void operator()(const NodeBinExprSub* sub) const
      {
        gen.gen_expr(sub->rhs);
        gen.gen_expr(sub->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    sub rax, rbx\n";
        gen.push("rax");
      }
      void operator()(const NodeBinExprAdd* add) const
      {
        gen.gen_expr(add->rhs);
        gen.gen_expr(add->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    add rax, rbx\n";
        gen.push("rax");
      }
      void operator()(const NodeBinExprMulti* multi) const
      {
        gen.gen_expr(multi->rhs);
        gen.gen_expr(multi->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    mul rbx\n";
        gen.push("rax");
      }
      void operator()(const NodeBinExprDiv* div) const
      {
        gen.gen_expr(div->rhs);
        gen.gen_expr(div->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    div rbx\n";
        gen.push("rax");
      }
  };

      BinExprVisitor visitor { .gen = *this };
      std::visit(visitor, bin_expr->var);
  }

  void gen_expr(const NodeExpr* expr){

      struct ExprVisitor {
          Generator& gen;
          void operator()(const NodeTerm* term) const
          {
              gen.gen_term(term);
          }
          void operator()(const NodeBinExpr* bin_expr) const
          {
              gen.gen_bin_expr(bin_expr);
          }
      };

      ExprVisitor visitor { .gen = *this };
      std::visit(visitor, expr->var);
  }

  void gen_scope(const NodeScope* scope)
  {
      begin_scope();
      for (const NodeStmt* stmt : scope->stmts) {
          gen_stmt(stmt);
      }
      end_scope();
  }

  void gen_stmt(const NodeStmt* stmt){

    struct StmtVisitor {
      Generator& gen;
      void operator()(const NodeStmtExit* stmt_exit) const
      {
        gen.gen_expr(stmt_exit->expr);
        gen.m_output << "    mov rax, 60\n";
        gen.pop("rdi"); // pop from the stack and put it in rdi
        gen.m_output << "    syscall\n";
      }

      // operator overload that allows objects of a class to be called as if they were functions.
      // specifically designed to handle 'let' statement nodes (NodeStmtLet) in an Abstract Syntax Tree (AST).
      void operator()(const NodeStmtLet* stmt_let) const{
        // looks for the variable name in the existing variable list to ensure that the variable has not been declared before in the same scope.
        auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
          return var.name == stmt_let->ident.value.value();  // [&] Capture all automatic (local) variables odr-used 
        });

        if (it != gen.m_vars.cend()) {
          // If the variable is already declared, if it doesn't equal the end then it doesn't exist
          std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
          exit(EXIT_FAILURE);
        }
        // the stack location
        gen.m_vars.push_back({ .name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size });
        gen.gen_expr(stmt_let->expr);
      }

      void operator()(const NodeScope* scope) const
      {
        gen.gen_scope(scope);
      }
      void operator()(const NodeStmtIf* stmt_if) const
      {
        gen.gen_expr(stmt_if->expr);
        gen.pop("rax");
        std::string label = gen.create_label();
        gen.m_output << "    test rax, rax\n";
        gen.m_output << "    jz " << label << "\n";
        gen.gen_scope(stmt_if->scope);
        gen.m_output << label << ":\n";
      }
  };

    StmtVisitor visitor { .gen = *this };
    std::visit(visitor, stmt->var);
  }

  [[nodiscard]] std::string gen_prog(){
    m_output << "global _start\n_start:\n";

    for (const NodeStmt* stmt : m_prog.stmts) {
      gen_stmt(stmt);
    }

    m_output << "    mov rax, 60\n";
    m_output << "    mov rdi, 0\n";
    m_output << "    syscall\n";
    return m_output.str();
  }

private:
  void push(const std::string& reg){
    m_output << "    push " << reg << "\n";
    m_stack_size++;
  }

  void pop(const std::string& reg){
    m_output << "    pop " << reg << "\n";
    m_stack_size--;
  }

  void begin_scope(){
    m_scopes.push_back(m_vars.size());
  }

  // when we end we want to pop the variables until we get to the last begin scope
  void end_scope(){
    size_t pop_count = m_vars.size() - m_scopes.back(); // counter to know how many variables
    m_output << "    add rsp, " << pop_count * 8 << "\n"; // subtract from the stack pointer. I multiply by 8 because each variable is 8 bytes 

    m_stack_size -= pop_count;
    for (int i = 0; i < pop_count; i++) {
      m_vars.pop_back();
    }

    m_scopes.pop_back();
  }

  // create a label for the if statement to jump to
  std::string create_label(){
    std::stringstream ss;
    ss << "label" << m_label_count++;
    return ss.str();
  }

  struct Var {
    std::string name; // the name of the variable
    size_t stack_loc; // The location on the stack where this variables value is stored.
  };

  const NodeProg m_prog;
  std::stringstream m_output;
  size_t m_stack_size = 0;
  std::vector<Var> m_vars {}; // vector of variables
  std::vector<size_t> m_scopes {};
  int m_label_count = 0;
};