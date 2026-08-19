# JSLang

JSLang is a statically typed, compiled programming language with JavaScript-like syntax, Go-inspired simplicity, and systems-level ambitions.

v0.0.1 is the C bootstrap compiler: a source-positioned lexer and the `jsl lex` command. Parsing is intentionally deferred to v0.0.2.

## Versioning

The root [`VERSION`](VERSION) file is the single source of truth for the compiler version. Both Make and CMake pass that value to `jsl version`; update this file for each release following Semantic Versioning.

## Build and test

```bash
make
make test
./build/jsl version
./build/jsl lex examples/hello/main.jsl
```

The same project can also be built with CMake:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```
