# JSLang

JSLang is an experimental, statically typed, ahead-of-time compiled programming language. It combines familiar JavaScript/TypeScript-style syntax with a path toward native systems programming.

> JavaScript-like syntax. Native binaries. From cloud to kernel.

JSLang is not JavaScript and does not run on Node.js or V8. The `jsl` tool compiles its supported subset into native executables.

JSLang follows JavaScript and TypeScript conventions wherever they work with static typing, deterministic native compilation, and safety. Where a dynamic runtime behavior would conflict with those constraints, JSLang uses an explicit documented alternative. This keeps the language familiar while supporting native applications and future kernel-level development.

## Status

The current compiler is **v0.0.9**. It provides source-positioned lexing through `jsl lex`, parser output through `jsl parse`, semantic and type validation through `jsl check`, and native executable generation through `jsl build`.

**v0.0.9** adds native `if`/`else` execution, including nested blocks and boolean conditional expressions.

The root [`VERSION`](VERSION) file is the single source of truth for the released compiler version.

## Language overview

JSLang is designed for explicit, predictable code:

- Static types use machine-sized names such as `i32`, `u64`, and `f64`; there is no universal JavaScript `number` type.
- Functions use a TypeScript-like declaration syntax.
- `const` declares an immutable binding; `let` declares a mutable binding.
- Statements end with semicolons, and blocks use braces.
- Errors, concurrency, low-level memory access, interfaces, and JSLangK are planned language features.
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

Build the compiler, then compile and run the program:

```bash
make
./build/jsl build hello.jsl -o hello
./hello
```

You can also inspect its tokens with:

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

Build the basic executable example:

```bash
./build/jsl build examples/executable/main.jsl
./main
echo $?
```

The resulting exit code is `42`. The compiler can also build [the functions example](examples/functions/main.jsl):

```bash
./build/jsl build examples/functions/main.jsl -o functions-app
./functions-app
echo $?
```

The backend supports `i32` functions and parameters; local `i32`, `bool`, `string`, and struct values; integer, boolean, and string literals; unary, binary, and ternary expressions; `if`/`else`; direct calls; returns; structs; and the documented console APIs. Imports and the remaining types are not yet executable.

### Console API

JSLang provides JavaScript-style native console calls:

```ts
console.log("Hello JSLang");
console.info(42);
console.warn("warning");
console.error("error");
console.write("Without a newline");
```

`log` and `info` write to stdout with a newline. `warn` and `error` write to stderr with a newline. `write` writes to stdout without a newline. `read()` and `readLine()` are available as string-returning expressions, for example `const name: string = console.readLine();`.

Build and run the console example:

```bash
./build/jsl build examples/console/main.jsl -o console-app
./console-app
```

## Syntax guide

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

Use `const` for values that do not change and `let` for values that do. Type annotations are optional when the initializer has an inferable primitive type. Integer literals infer `i32`, floating-point literals infer `f64`, string literals infer `string`, and boolean literals infer `bool`.

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

JSLang parses literals, identifiers, parenthesized expressions, unary operators, binary operators, conditional expressions, property access, and function calls.

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

The lexer recognizes arithmetic operators (`+`, `-`, `*`, `/`, `%`), comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`), logical operators (`!`, `&&`, `||`), and assignment (`=`). Type checking requires matching numeric types for arithmetic and comparisons, matching types for equality, and `bool` operands for logical operators and conditions.

### Conditional expressions

Use the JavaScript/TypeScript ternary form when an expression selects between two values:

```ts
const absolute: i32 = value < 0 ? -value : value;
```

The condition is evaluated first, followed by exactly one of the two result expressions.

### Blocks, returns, and conditionals

Blocks group statements in braces. Use `return` to leave a function. Native builds support `if` and optional `else` blocks.

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

`jsl lex <file>` prints each token with its filename, line, and column. `jsl parse <file>` prints a deterministic representation of the parsed AST. `jsl check <file>` validates scopes, names, primitive types, function calls, and return values. Run `./build/jsl` to see the current CLI usage.

`jsl build <file>` writes an executable named after the input file in the current directory. Use `jsl build <file> -o <path>` to select the output path.

## Roadmap

| Version | Focus |
| --- | --- |
| v0.0.1 | Source positions, tokens, lexer, and `jsl lex` |
| v0.0.2 | AST, recursive-descent parser, and `jsl parse` |
| v0.0.3 | Scopes, symbols, duplicate-name validation, and undefined-variable detection |
| v0.0.4 | Primitive types, inference, assignment checks, binary expressions, returns, and function calls |
| v0.0.5 | Minimal native executable backend and `jsl build` |
| v0.0.6 | Multiple functions, `i32` parameters, local variables, calls, and expressions in the native backend |
| v0.0.7 | Native JavaScript-style console API and basic string I/O |
| v0.0.8 | Fixed-shape structs, typed fields, struct literals, and native C layouts |
| v0.0.9 | Native `if`/`else` execution and boolean conditional expressions |

The evolving language references live in [`docs/`](docs/).
