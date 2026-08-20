# Types

v0.0.4 introduces primitive type checking.

Supported primitive types are `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `isize`, `usize`, `f32`, `f64`, `bool`, `char`, `string`, and `void`.

Integer literals infer `i32`; floating-point literals infer `f64`; string literals infer `string`; and `true` and `false` infer `bool`. A typed variable initializer, function argument, and return value must match its declared type. Arithmetic and comparison operands must use matching numeric types, while logical operators and conditions require `bool`.

Function calls to locally declared functions validate their argument count and argument types. Imported functions and standard-library APIs are name-checked but will receive full signature validation after module and standard-library metadata are implemented.
