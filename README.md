# JSLang

JSLang is an experimental, statically typed, ahead-of-time compiled programming language. It combines familiar JavaScript/TypeScript-style syntax with a path toward native systems programming.

> JavaScript-like syntax. Native binaries. From cloud to kernel.

JSLang is not JavaScript and does not run on Node.js or V8. Source is compiled by the `jsl` tool into native programs in future milestones.

JSLang follows JavaScript and TypeScript conventions wherever they work with static typing, deterministic native compilation, and safety. Where a dynamic runtime behavior would conflict with those constraints, JSLang uses an explicit documented alternative. This keeps the language familiar while supporting native applications and future kernel-level development.

## Status

The current compiler is **v0.0.3**. It provides source-positioned lexing through `jsl lex`, parser output through `jsl parse`, and semantic validation through `jsl check`.

**v0.0.3** introduces scopes and a symbol table. It detects duplicate declarations and undefined variables before type checking begins in v0.0.4.

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

Validate scopes and names with:

```bash
./build/jsl check hello.jsl
```

## Syntax guide for v0.0.2

### Modules

JSLang source files use the `.jsl` extension. Import named exports with JavaScript-style syntax:

```ts
import { add } from "./math.jsl";

function main(): i32 {
    return add(20, 22);
}
```

Export a function from another file:

```ts
export function add(left: i32, right: i32): i32 {
    return left + right;
}
```

v0.0.3 records imported names in the module scope, so they can be referenced in the importing file. Resolving module paths, validating exports, and linking imported symbols remain future module-system work.

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

`jsl lex <file>` prints each token with its filename, line, and column. `jsl parse <file>` prints a deterministic representation of the parsed AST. `jsl check <file>` validates scopes, duplicate declarations, and undefined variables. Run `./build/jsl` to see the current CLI usage.

## Roadmap

| Version | Focus |
| --- | --- |
| v0.0.1 | Source positions, tokens, lexer, and `jsl lex` |
| v0.0.2 | AST, recursive-descent parser, and `jsl parse` |
| v0.0.3 | Scopes, symbols, duplicate-name validation, and undefined-variable detection |
| v0.0.4 | Type checking and type inference |

See [`AGENT.md`](AGENT.md) for the project’s complete design principles and development roadmap. The evolving language references live in [`docs/`](docs/).
