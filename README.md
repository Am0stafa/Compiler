# Hydro Language C++ Compiler

## Introduction
The Hydro Language C++ Compiler is designed to transform code written in .hy files into executables. The process involves:
1. Parsing the code using the `tokenizer.hpp` file.
2. Creating an Abstract Syntax Tree (AST) using `parser.hpp`.
3. Transforming the AST into an assembly program, which is then assembled and linked to create an executable compatible with any operating system.

## Basic Elements

- **Keywords**: `exit`, `let`, `if`, `true`, `false`
- **Identifiers**: Sequences of alphanumeric characters, not starting with a digit and not matching any keywords.
- **Integer Literals**: Sequences of digits representing integer values.

## Grammar
The grammar of the Hydro language is defined using the Backus-Naur Form (BNF):

```plaintext
[Prog] -> [Stmt]*

[Stmt] -> exit([Expr]);
       | let ident = [Expr];
       | if ([Expr])[Scope]

[Scope] -> { [Stmt]* }

[Expr] -> [Term]
       | [BinExpr]

[BinExpr] -> [Expr] == [Expr] { prec = 2 }
         | [Expr] && [Expr] { prec = 1 }
         | [Expr] || [Expr] { prec = 1 }
         | [Expr] + [Expr]  { prec = 0 }
         | [Expr] - [Expr]  { prec = 0 }
         | [Expr] * [Expr]  { prec = -1 }
         | [Expr] / [Expr]  { prec = -1 }

[Term] -> int_lit
       | ident
       | ([Expr])
       | true
       | false
```

## Grammar Explanation

1. **Program ([Prog])**: A sequence of zero or more statements.
2. **Statement ([Stmt])**: Can be an `exit`, `let`, or `if` statement.
3. **Scope ([Scope])**: A block of zero or more statements enclosed in curly braces.
4. **Expression ([Expr])**: Either a term or a binary expression.
5. **Binary Expression ([BinExpr])**: Represents arithmetic and logical operations between two expressions.
6. **Term ([Term])**: Can be an integer literal, an identifier, a grouped expression, or boolean literals `true` and `false`.

## Additional Language Features

- **Single-Line Comments**: Start with `//` and extend to the end of the line.
- **Block Comments**: Enclosed between `/*` and `*/`, spanning multiple lines.
- Supports variable shadowing.

## Resources
- For understanding complex math operations by precedence, visit [Eli Bendersky's website](https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing).
