# JSLang

JSLang is an experimental, statically typed, ahead-of-time compiled programming language. It combines familiar JavaScript/TypeScript-style syntax with Go-inspired simplicity and a path toward native systems programming.

> JavaScript-like syntax. Go-like simplicity. Native binaries. From cloud to kernel.

JSLang is not JavaScript and does not run on Node.js or V8. Source is compiled by the `jsl` tool into native programs in future milestones.

## Status

The current released compiler is **v0.0.1** and provides source-positioned lexing through `jsl lex`.

**v0.0.2** is the next parser milestone. It introduces the AST and parsing support for functions, typed parameters, blocks, variable declarations, return statements, expression statements, `if` statements, unary and binary expressions, and function calls. Until that implementation lands, `jsl parse` is documented as the intended command, not an available command.

The root [`VERSION`](VERSION) file is the single source of truth for the released compiler version.

## Language overview

JSLang is designed for explicit, predictable code:

- Static types use machine-sized names such as `i32`, `u64`, and `f64`; there is no universal JavaScript `number` type.
- Functions use a TypeScript-like declaration syntax.
- `const` declares an immutable binding; `let` declares a mutable binding.
- Statements end with semicolons, and blocks use braces.
- Errors, concurrency, low-level memory access, structs, interfaces, and JSLangK are planned language features; they are not all available in v0.0.2.

## Your first JSLang program

Create `hello.jsl`:

```ts
function main(): i32 {
    const message: string = "Hello, JSLang";
    println(message);
    return 0;
}
```

For now, `println` illustrates the intended language syntax. The compiler has not yet implemented type checking, a standard library, or native code generation, so this program cannot run yet. You can inspect its tokens with the current compiler:

```bash
./build/jsl lex hello.jsl
```

Once v0.0.2 is implemented, its structure can be checked with:

```bash
./build/jsl parse hello.jsl
```

## Syntax guide for v0.0.2

### Functions

Declare functions with `function`. Parameters and return values use `name: Type` annotations. The initial language requires explicit return types.

```ts
function add(left: i32, right: i32): i32 {
    return left + right;
}
```

Use `void` when a function does not return a value:

```ts
function greet(name: string): void {
    println(name);
}
```

### Variables

Use `const` for values that do not change and `let` for values that do. Type annotations are optional when the value has an inferable type; type checking and inference arrive after the parser milestone.

```ts
const answer: i32 = 42;
let counter: i32 = 0;

const title = "JSLang";
let enabled = true;
```

### Primitive types

| Kind | Types |
| --- | --- |
| Signed integers | `i8`, `i16`, `i32`, `i64` |
| Unsigned integers | `u8`, `u16`, `u32`, `u64` |
| Floating point | `f32`, `f64` |
| Other | `bool`, `char`, `string`, `void`, `usize`, `isize` |

### Expressions and calls

v0.0.2 parses literals, identifiers, parenthesized expressions, unary operators, binary operators, and function calls.

```ts
function calculate(value: i32): i32 {
    const doubled: i32 = value * 2;
    return -(doubled + 1);
}

function main(): i32 {
    const result: i32 = calculate(20);
    return result;
}
```

The lexer recognizes arithmetic operators (`+`, `-`, `*`, `/`, `%`), comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`), logical operators (`!`, `&&`, `||`), and assignment (`=`). Semantic rules—including operator type validation—belong to later milestones.

### Blocks, returns, and conditionals

Blocks group statements in braces. Use `return` to leave a function. The parser milestone also supports `if` and optional `else` blocks.

```ts
function absolute(value: i32): i32 {
    if (value < 0) {
        return -value;
    } else {
        return value;
    }
}
```

## Build and test

The bootstrap compiler is written in C and builds with Make:

```bash
make
make test
./build/jsl version
./build/jsl lex examples/hello/main.jsl
```

Or build it with CMake:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

`jsl lex <file>` prints each token with its filename, line, and column. This is the only language-processing command currently implemented. Run `./build/jsl` to see the current CLI usage.

## Roadmap

| Version | Focus |
| --- | --- |
| v0.0.1 | Source positions, tokens, lexer, and `jsl lex` |
| v0.0.2 | AST, recursive-descent parser, and `jsl parse` |
| v0.0.3 | Scopes, symbols, and duplicate-name validation |
| v0.0.4 | Type checking and type inference |

See [`AGENT.md`](AGENT.md) for the project’s complete design principles and development roadmap. The evolving language references live in [`docs/`](docs/).
