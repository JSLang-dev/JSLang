# JSLang language specification

This is a draft specification. v0.0.4 defines lexical tokens, source positions, parser grammar, name resolution, and the initial primitive type rules below.

## Compatibility and safety

JSLang adopts JavaScript and TypeScript syntax and familiar semantics where they are compatible with static typing, deterministic ahead-of-time compilation, and safe native execution. It does not reproduce the JavaScript runtime. Any deliberate semantic difference—for example one needed for type safety, memory safety, predictable errors, or kernel execution—must be explicitly specified here.

The language aims to prevent crashes where practical. Low-level operations and any future unsafe capability must be explicit and opt-in.

## v0.0.2 grammar

```text
program          → declaration* EOF ;
declaration      → importDeclaration | functionDeclaration ;
importDeclaration
                 → "import" "{" importedNames? "}" "from" STRING ";" ;
importedNames    → IDENTIFIER ( "," IDENTIFIER )* ;
functionDeclaration
                 → "export"? "function" IDENTIFIER "(" parameters? ")" ":" IDENTIFIER block ;
parameters       → parameter ( "," parameter )* ;
parameter        → IDENTIFIER ":" IDENTIFIER ;
block            → "{" statement* "}" ;
statement        → variableDeclaration | returnStatement | ifStatement | expressionStatement ;
variableDeclaration
                 → ( "const" | "let" ) IDENTIFIER ( ":" IDENTIFIER )? "=" expression ";" ;
returnStatement  → "return" expression? ";" ;
ifStatement      → "if" "(" expression ")" block ( "else" block )? ;
expressionStatement
                 → expression ";" ;
expression       → binaryExpression ( "?" expression ":" expression )? ;
```

The parser accepts identifiers; integer, floating-point, string, boolean, and null literals; grouping; unary `!`, `-`, and `+`; binary arithmetic, comparison, equality, and logical operators; JavaScript/TypeScript-style conditional expressions (`condition ? whenTrue : whenFalse`); dotted property access; and function calls. This includes JavaScript-style calls such as `console.log(message)`, `console.info(message)`, and `console.error(message)`. Assignment is tokenized but not yet a parsed expression or statement. Type validity and runtime behavior are deferred to semantic analysis and later compiler stages.

Source files use the `.jsl` extension. v0.0.3 records named imports and exported functions, but does not yet resolve module paths or validate imported and exported symbols.

## Name resolution

v0.0.3 creates a global scope for functions and imported names, a function scope for parameters, and a nested lexical scope for every block. A name must be declared before it is used. Duplicate declarations in the same scope are rejected. Nested blocks may shadow names from an enclosing scope.

`console` is available as a built-in global name so that JavaScript-style console calls can be parsed and name-checked. Property validation and the standard-library implementation are deferred to later milestones.

Module paths are not resolved yet. An imported name is available in the importing file, but the compiler does not yet verify that the target module exists or exports that name.

## Primitive type checking

v0.0.4 recognizes `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `isize`, `usize`, `f32`, `f64`, `bool`, `char`, `string`, and `void`. Unknown type names are compile errors.

An integer literal has type `i32`; a floating-point literal has type `f64`; a string literal has type `string`; and boolean literals have type `bool`. A variable with a type annotation, a function argument, and a returned expression must match the declared type. This version does not perform implicit numeric conversion.

Arithmetic and relational comparison operators require matching numeric operand types. Equality operators require matching operand types. Logical operators, `if` conditions, and ternary conditions require `bool`. The two result expressions of a ternary expression must have matching types.

Calls to locally declared functions validate argument count and parameter types, and their result type is the function's declared return type. Imported functions and built-in APIs remain type-unknown until module resolution and standard-library declarations are implemented.
