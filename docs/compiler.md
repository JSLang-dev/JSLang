# Compiler

The initial compiler pipeline is source, lexer, tokens, parser, AST, checker, IR, and backend. v0.0.2 implements source positions, tokens, the lexer, a recursive-descent parser, and an explicit AST. Use `jsl parse <file>` to print a deterministic AST representation.
