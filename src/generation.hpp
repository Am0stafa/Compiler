// File that generates the assembly code (Code generation)
// It takes in the root node of the AST and traverses that tree and while its traversing it generates the assembly code. I have different method for generating each node type.

#pragma once
#include "parser.hpp"
#include <cassert>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

class Generator {
public:
  inline explicit Generator(NodeProg prog)
    : m_prog(std::move(prog)) // take the program out put from the parser (AST)
  {
  }
    std::stringstream m_data_section; // To store string literals

    // Utility function to generate a unique label for each string literal
    std::string make_string_label() {
        static int string_label_count = 0;
        return "str_lit_" + std::to_string(string_label_count++);
    }

    void gen_func_prologue() {
    m_output << "    push rbp\n";
    m_output << "    mov rbp, rsp\n";
    // Reserve space for local variables if needed
    }

    void gen_func_epilogue() {
        m_output << "    mov rsp, rbp\n";
        m_output << "    pop rbp\n";
        m_output << "    ret\n";
    }

    void gen_param_passing(const std::vector<Token>& params) {
      for (int i = params.size() - 1; i >= 0; --i) {
          // Assuming parameters are pushed onto the stack in reverse order before the call
          m_output << "    push [rbp + " << (i + 2) * 8 << "]\n"; // +2 for return address and old rbp
      }
    }

    void gen_func_def(const NodeFuncDef* func_def) {
      m_output << func_def->ident.value.value() << ":\n";
      gen_func_prologue();
      gen_param_passing(func_def->params);
      gen_scope(func_def->body);
      gen_func_epilogue();
    }

    void gen_func_call(const NodeFuncCall* func_call) {
      // Push arguments in reverse order
      for (auto it = func_call->args.rbegin(); it != func_call->args.rend(); ++it) {
          gen_expr(*it);
      }
      m_output << "    call " << func_call->ident.value.value() << "\n";

      // Adjust the stack pointer after the call
      if (!func_call->args.empty()) {
          m_output << "    add rsp, " << func_call->args.size() * 8 << "\n";
      }
    }

    void gen_return_stmt(const NodeReturn* node_return) {
        gen_expr(node_return->expr);
        m_output << "    pop rax\n"; // Assuming the return value is at the top of the stack
        gen_func_epilogue();
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
      void operator()(const NodeBinExprEq* eq) const {
        gen.gen_expr(eq->rhs);
        gen.gen_expr(eq->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    cmp rax, rbx\n";
        gen.m_output << "    sete al\n";
        gen.m_output << "    movzx rax, al\n";
        gen.push("rax");
      }
      void operator()(const NodeBinExprAnd* andExpr) const {
        std::string labelFalse = gen.create_label();
        std::string labelEnd = gen.create_label();

        // Evaluate the left-hand side expression
        gen.gen_expr(andExpr->lhs);
        gen.pop("rax");
        gen.m_output << "    cmp rax, 0\n";  // Compare with false
        gen.m_output << "    je " << labelFalse << "\n";  // Jump if false

        // Evaluate the right-hand side expression
        gen.gen_expr(andExpr->rhs);
        gen.pop("rax");
        gen.m_output << "    cmp rax, 0\n";  // Compare with false
        gen.m_output << "    je " << labelFalse << "\n";  // Jump if false

        // Both are true
        gen.m_output << "    mov rax, 1\n";  // Set result to true
        gen.m_output << "    jmp " << labelEnd << "\n";

        // False label
        gen.m_output << labelFalse << ":\n";
        gen.m_output << "    mov rax, 0\n";  // Set result to false

        // End label
        gen.m_output << labelEnd << ":\n";
        gen.push("rax");
      }
      void operator()(const NodeBinExprOr* orExpr) const {
        std::string labelTrue = gen.create_label();
        std::string labelEnd = gen.create_label();

        // Evaluate the left-hand side expression
        gen.gen_expr(orExpr->lhs);
        gen.pop("rax");
        gen.m_output << "    cmp rax, 0\n";  // Compare with false
        gen.m_output << "    jne " << labelTrue << "\n";  // Jump if true

        // Evaluate the right-hand side expression
        gen.gen_expr(orExpr->rhs);
        gen.pop("rax");
        gen.m_output << "    cmp rax, 0\n";  // Compare with false
        gen.m_output << "    jne " << labelTrue << "\n";  // Jump if true

        // Both are false
        gen.m_output << "    mov rax, 0\n";  // Set result to false
        gen.m_output << "    jmp " << labelEnd << "\n";

        // True label
        gen.m_output << labelTrue << ":\n";
        gen.m_output << "    mov rax, 1\n";  // Set result to true

        // End label
        gen.m_output << labelEnd << ":\n";
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
          void operator()(const NodeFuncCall* func_call) const {
            gen.gen_func_call(func_call);
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

  std::string escape_string(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '\n': escaped += "\\n"; break;
            case '\t': escaped += "\\t"; break;
            case '\"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            // Other escape sequences as needed
            default: escaped += c;
        }
    }
    return escaped;
  }

  void gen_stmt(const NodeStmt* stmt){

    struct StmtVisitor {
      Generator& gen;
      void operator()(const NodeStmtExit* stmt_exit) const {
        gen.gen_expr(stmt_exit->expr);
        gen.m_output << "    mov rax, 60\n";
        gen.pop("rdi"); // pop from the stack and put it in rdi
        gen.m_output << "    syscall\n";
      }

      // operator overload that allows objects of a class to be called as if they were functions.
      // specifically designed to handle 'let' statement nodes (NodeStmtLet) in an Abstract Syntax Tree (AST).
      void operator()(const NodeStmtLet* stmt_let) const {
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

      void operator()(const NodeScope* scope) const {
        gen.gen_scope(scope);
      }

      void operator()(const NodeStmtElse* stmt_else) const {
        // Simply generate the code within the else scope, as it is unconditional.
        gen.gen_scope(stmt_else->scope);
      }

      void operator()(const NodeFuncDef* func_def) const {
          gen.gen_func_def(func_def);
      }

      void operator()(const NodeReturn* return_stmt) const {
          gen.gen_return_stmt(return_stmt);
      }

      void operator()(const NodeStmtElseIf* stmt_else_if) const {
        std::string end_label = gen.create_label(); // Label for the end of the entire if-else chain

        // Generate code for each else-if in the chain
        while (stmt_else_if != nullptr) {
          std::string next_label = gen.create_label(); // Label for the next else-if

          // Evaluate the else-if condition
          gen.gen_expr(stmt_else_if->expr);
          gen.pop("rax");
          gen.m_output << "    cmp rax, 0\n";
          gen.m_output << "    je " << next_label << "\n";

          // Generate the code for the else-if scope
          gen.gen_scope(stmt_else_if->scope);
          gen.m_output << "    jmp " << end_label << "\n";

          // Set up the next else-if (or else)
          gen.m_output << next_label << ":\n";
          stmt_else_if = stmt_else_if->next;
        }

        // End label for the entire if-else chain
        gen.m_output << end_label << ":\n";
      }

      void operator()(const NodeStmtIf* stmt_if) const {
        gen.gen_expr(stmt_if->expr);
        gen.pop("rax");
        std::string label = gen.create_label();
        gen.m_output << "    test rax, rax\n";
        gen.m_output << "    jz " << label << "\n";
        gen.gen_scope(stmt_if->scope);
        gen.m_output << label << ":\n";
      }

      void operator()(const NodeStmtWhile* stmt_while) const {
        std::string start_label = gen.create_label();
        std::string end_label = gen.create_label();

        gen.m_output << start_label << ":\n";
        gen.gen_expr(stmt_while->expr);
        gen.pop("rax");
        gen.m_output << "    cmp rax, 0\n";
        gen.m_output << "    je " << end_label << "\n";

        gen.gen_scope(stmt_while->scope);
        gen.m_output << "    jmp " << start_label << "\n";
        gen.m_output << end_label << ":\n";
      }

      void operator()(const NodeStmtFor* stmt_for) const {
        std::string start_label = gen.create_label();
        std::string end_label = gen.create_label();

        // Generate initialization
        gen.gen_expr(stmt_for->init);

        gen.m_output << start_label << ":\n";

        // Generate condition check
        gen.gen_expr(stmt_for->condition);
        gen.pop("rax");
        gen.m_output << "    cmp rax, 0\n";
        gen.m_output << "    je " << end_label << "\n";

        // Generate the for loop scope
        gen.gen_scope(stmt_for->scope);

        // Generate iteration
        gen.gen_expr(stmt_for->iteration);

        gen.m_output << "    jmp " << start_label << "\n";
        gen.m_output << end_label << ":\n";
      }

      void operator()(const NodeBoolLit* bool_lit) const {
        int boolValue = bool_lit->value ? 1 : 0;
        gen.m_output << "    mov rax, " << boolValue << "\n";
        gen.push("rax");
      }
      
      void operator()(const NodeStringLit* str_lit) const {
          std::string label = gen.make_string_label();

          // Escape the string literal and store it in the data section
          std::string escaped_str = escape_string(str_lit->value);
          gen.m_data_section << label << ": db \"" << escaped_str << "\", 0\n"; // Null-terminated string

          // Load the address of the string into a register
          gen.m_output << "    lea rax, [" << label << "]\n";
          gen.push("rax");
      }
      // Function to determine if the expression is a string expression
      bool is_string_expression(const NodeExpr* expr) {
          if (!expr) {
              return false; // Safety check in case expr is a nullptr
          }

          // Check if the variant holds a NodeStringLit
          return std::holds_alternative<NodeStringLit*>(expr->var);
      }
      // Convert an integer in a register to its ASCII string representation
      void int_to_string(const std::string& int_reg, const std::string& str_reg) {
          // Assuming int_reg contains the integer and str_reg is where the string should be stored
          // This is a simplified example. A full implementation would handle negative numbers and zero.
          gen.m_output << "    mov " << str_reg << ", rsp\n";  // Use stack for temporary storage
          gen.m_output << "    mov rsi, " << int_reg << "\n";  // Integer to convert
          gen.m_output << "    mov rbx, 10\n";                // Divisor for conversion
          std::string loop_label = gen.create_label();
          gen.m_output << loop_label << ":\n";
          gen.m_output << "    xor rdx, rdx\n";               // Clear rdx for division
          gen.m_output << "    div rbx\n";                    // rax = rsi / 10, rdx = rsi % 10
          gen.m_output << "    add dl, '0'\n";                // Convert to ASCII
          gen.m_output << "    push rdx\n";                   // Push character onto stack
          gen.m_output << "    mov rsi, rax\n";               // Prepare next digit
          gen.m_output << "    test rax, rax\n";              // Check if rax is zero
          gen.m_output << "    jnz " << loop_label << "\n";   // Repeat if not zero
          // Now, reverse the string on the stack into str_reg
          gen.reverse_stack_string(str_reg);
      }
      // Reverse a string stored on the stack into a register
      void reverse_stack_string(const std::string& str_reg) {
          // Implementation depends on your register management
          // Here's a placeholder example
          gen.m_output << "    lea " << str_reg << ", [rsp]\n"; // String start
          // Add logic to reverse the string on the stack into str_reg
      }

      void operator()(const NodeStmtPrint* stmt_print) const {
        gen.gen_expr(stmt_print->expr);

        if (is_string_expression(stmt_print->expr)) {
            // For string printing
            gen.pop("rax");  // Address of the string
            gen.m_output << "    mov rdi, rax\n"; // First argument (string pointer)
            gen.setup_string_length("rax");       // Second argument (string length)
            gen.syscall_write();
        } else {
            // For integer printing
            gen.pop("rax");  // Integer value
            gen.int_to_string("rax", "rbx"); // Convert integer to string, stored at rbx
            gen.m_output << "    mov rdi, rbx\n"; // First argument (string pointer)
            gen.setup_string_length("rbx");       // Second argument (string length)
            gen.syscall_write();
        }
    }

    // Performs the write syscall
    void syscall_write() {
        gen.m_output << "    mov rax, 1\n";  // Syscall number for write
        gen.m_output << "    mov rdx, rsi\n"; // Length of the string
        gen.m_output << "    mov rdi, 1\n";   // File descriptor (stdout)
        gen.m_output << "    syscall\n";
    }

  };
    // NodeStmtPrint definition
    struct NodeStmtPrint {
        NodeExpr* expr;
    };
    StmtVisitor visitor { .gen = *this };
    std::visit(visitor, stmt->var);
  }

  [[nodiscard]] std::string gen_prog() {
      // Start with the text section which includes the main program
      m_output << "global _start\nsection .text\n_start:\n";

      // Generate the assembly code for each statement in the program
      for (const NodeStmt* stmt : m_prog.stmts) {
          gen_stmt(stmt);
      }

      // Common exit sequence for the program
      m_output << "    mov rax, 60\n";  // syscall number for exit in x86-64 Linux
      m_output << "    mov rdi, 0\n";   // exit status
      m_output << "    syscall\n";

      // Include the data section if there are string literals
      if (!m_data_section.str().empty()) {
          m_output << "section .data\n" << m_data_section.str();
      }

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