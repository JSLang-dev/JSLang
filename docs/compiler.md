# Compiler

The initial compiler pipeline is source, lexer, tokens, parser, AST, checker, IR, and backend. v0.0.4 implements source positions, tokens, the lexer, a recursive-descent parser, an explicit AST, name resolution, and primitive type checking. Use `jsl parse <file>` to print a deterministic AST representation and `jsl check <file>` to validate scopes, names, types, calls, and return values.
