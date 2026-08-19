# JSLang

JSLang is an experimental, statically typed, ahead-of-time compiled programming language. It combines familiar JavaScript/TypeScript-style syntax with a path toward native systems programming.

> JavaScript-like syntax. Native binaries. From cloud to kernel.

JSLang is not JavaScript and does not run on Node.js or V8. Source is compiled by the `jsl` tool into native programs in future milestones.

JSLang follows JavaScript and TypeScript conventions wherever they work with static typing, deterministic native compilation, and safety. Where a dynamic runtime behavior would conflict with those constraints, JSLang uses an explicit documented alternative. This keeps the language familiar while supporting native applications and future kernel-level development.

## Status

The current compiler is **v0.0.2**. It provides source-positioned lexing through `jsl lex` and parser output through `jsl parse`.

**v0.0.2** introduces the AST and parsing support for functions, typed parameters, blocks, variable declarations, return statements, expression statements, `if` statements, unary and binary expressions, and function calls.

The root [`VERSION`](VERSION) file is the single source of truth for the released compiler version.

## Language overview

JSLang is designed for explicit, predictable code:

- Static types use machine-sized names such as `i32`, `u64`, and `f64`; there is no universal JavaScript `number` type.
- Functions use a TypeScript-like declaration syntax.
- `const` declares an immutable binding; `let` declares a mutable binding.
- Statements end with semicolons, and blocks use braces.
- Errors, concurrency, low-level memory access, structs, interfaces, and JSLangK are planned language features; they are not all available in v0.0.2.
- Safety takes priority over dynamic convenience: undefined behavior and low-level operations must be explicit, documented, and auditable.

## Your first JSLang program

Create `hello.jsl`:

```ts
function main(): i32 {
    const message: string = "Hello, JSLang";
    console.log(message);
    return 0;
}
```

For now, `console.log` illustrates the intended JavaScript-style API. The compiler has not yet implemented type checking, the standard library, or native code generation, so this program cannot run yet. You can inspect its tokens with the current compiler:

```bash
./build/jsl lex hello.jsl
```

Inspect its parsed structure with:

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
    console.info(name);
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

v0.0.2 parses literals, identifiers, parenthesized expressions, unary operators, binary operators, conditional expressions, property access, and function calls.

```ts
function calculate(value: i32): i32 {
    const doubled: i32 = value * 2;
    return -(doubled + 1);
}

function main(): i32 {
    const result: i32 = calculate(20);
    console.error(result);
    return result;
}
```

The lexer recognizes arithmetic operators (`+`, `-`, `*`, `/`, `%`), comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`), logical operators (`!`, `&&`, `||`), and assignment (`=`). Semantic rules—including operator type validation—belong to later milestones.

### Conditional expressions

Use the JavaScript/TypeScript ternary form when an expression selects between two values:

```ts
const absolute: i32 = value < 0 ? -value : value;
```

The condition is evaluated first, followed by exactly one of the two result expressions.

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

`jsl lex <file>` prints each token with its filename, line, and column. `jsl parse <file>` prints a deterministic representation of the parsed AST. Run `./build/jsl` to see the current CLI usage.

## Roadmap

| Version | Focus |
| --- | --- |
| v0.0.1 | Source positions, tokens, lexer, and `jsl lex` |
| v0.0.2 | AST, recursive-descent parser, and `jsl parse` |
| v0.0.3 | Scopes, symbols, and duplicate-name validation |
| v0.0.4 | Type checking and type inference |

See [`AGENT.md`](AGENT.md) for the project’s complete design principles and development roadmap. The evolving language references live in [`docs/`](docs/).
