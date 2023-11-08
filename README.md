# Hydro Language C++ Compiler

## Introductory 
It basically transforms code written in .hy file into an executable.
What it does is take the code in .hy file 1. parse it using the `tokenizer.hpp` file then 2. Create an abstract syntax tree (AST) using the `parser.hpp` then 3. take that AST and transform it into an assembly program which is then assembled and linked to create an executable for any operating system

## Grammar
grammar of the Hydro language defined using the Backus-Naur Form (BNF).

```plaintext
[Prog] -> [Stmt]*
[Stmt] ->   exit([Expr]);
          | let ident = [Expr];
          | if ([Expr])[Scope]
[Scope] -> { [Stmt]* }
[Expr] ->   [Term]
          | [BinExpr]
[BinExpr] ->   [Expr] * [Expr] { prec = 1 }
            | [Expr] / [Expr] { prec = 1 }
            | [Expr] + [Expr] { prec = 0 }
            | [Expr] - [Expr] { prec = 0 }
[Term] ->   int_lit
          | ident
          | ([Expr])
```
<img width="479" alt="Screenshot 2023-10-27 at 1 20 59 AM" src="https://github.com/Am0stafa/compiler/assets/62848968/5376e7ff-d539-4e0d-ba15-992cca41d394">

## Grammar Explanation

1. **Program ([Prog])**:
    - A program is defined as a sequence of statements ([Stmt]). The asterisk denotes zero or more occurrences of statements.

2. **Statement ([Stmt])**:
    - A statement can be one of three forms:
        - An `exit` statement with an expression enclosed in parentheses, followed by a semicolon.
        - A `let` statement, where a new identifier is being assigned a value of some expression, followed by a semicolon.
        - An `if` statement with a condition expression enclosed in parentheses, followed by a scope ([Scope]).

3. **Scope ([Scope])**:
    - A scope is defined by curly braces enclosing zero or more statements.

4. **Expression ([Expr])**:
    - An expression can either be a term ([Term]) or a binary expression ([BinExpr]).

5. **Binary Expression ([BinExpr])**:
    - A binary expression represents basic arithmetic operations between two expressions:
        - Multiplication and division have a precedence level of 1.
        - Addition and subtraction have a precedence level of 0. The precedence levels indicate the order in which operations are evaluated, with higher precedence operations being evaluated first.

6. **Term ([Term])**:
    - A term can be:
        - An integer literal (`int_lit`).
        - An identifier (`ident`).
        - An expression enclosed in parentheses, allowing for grouped expressions. \\

It supports variable shadowing
     


## Resources
  - For complex math operations by precedence:
     - link https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
