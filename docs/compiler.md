# Compiler

The initial compiler pipeline is source, lexer, tokens, parser, AST, checker, IR, and backend. v0.0.5 implements source positions, tokens, the lexer, a recursive-descent parser, an explicit AST, name resolution, primitive type checking, and a minimal C-emitting native backend. Use `jsl parse <file>` to print a deterministic AST representation, `jsl check <file>` to validate scopes, names, types, calls, and return values, and `jsl build <file>` to create an executable.

The v0.0.5 backend accepts exactly one parameterless `main(): i32` function whose body is a single integer-literal `return` statement. It emits temporary C source and invokes the host C compiler. This is a bootstrap backend, not a JavaScript runtime or a permanent code-generation architecture.
