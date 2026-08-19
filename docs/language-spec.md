# JSLang language specification

This is a draft specification. v0.0.2 defines lexical tokens, source positions, and the parser grammar below.

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

Source files use the `.jsl` extension. v0.0.2 parses named imports and exported functions, but does not yet resolve module paths or validate imported and exported symbols.
