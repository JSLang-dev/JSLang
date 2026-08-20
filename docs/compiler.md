# Compiler

The initial compiler pipeline is source, lexer, tokens, parser, AST, checker, IR, and backend. v0.0.3 implements source positions, tokens, the lexer, a recursive-descent parser, an explicit AST, and name resolution. Use `jsl parse <file>` to print a deterministic AST representation and `jsl check <file>` to validate scopes, duplicate names, and undefined variables.
