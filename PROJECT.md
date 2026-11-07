# Project: MEMElang Advanced Language Extensions & Comparative Rust Benchmarks

## Architecture
- **Language**: MEMElang (C++20 runtime, clang++ compiler)
- **Lexer & Tokens**: `src/lexer/Lexer.h` provides unified tokenization with dual Desi meme syntax and English aliases mapping to `TokenType` enums.
- **Parser & AST**: `src/parser/Parser.h` builds typed AST expressions and statements.
- **Type System**: `src/types/Type.h` and `src/types/TypeChecker.cpp` provide static & gradual type analysis.
- **Runtime & Environment**: `src/interpreter/Value.h` (`sizeof(Value) == 32`), `src/interpreter/Environment.h`, and `src/interpreter/Interpreter.cpp` execute AST nodes with lexical environments, scoped borrow checking, and Tantrik dispatch.
- **Benchmark Suite**: `benchmarks/` contains paired MEMElang and Rust programs with automated runner `benchmarks/run_rust_benchmarks.py`.

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | Forbidden Word Sanitization | Eradicate all 18 peripheral occurrences of the forbidden word across docs/examples/tests and enforce 0 repository occurrences | M1 | Survey |
| 2 | Lexer Tokenization for Address-of (`&`) | Enable single `&` tokenization as `TokenType::ADDRESS_OF` without breaking `&&` | M2 | Survey / R2 |
| 3 | Lexer Keywords: Function Overloading | Desi (`bahuroop`, `aur_ek`) and English (`overload`, `multimethod`) aliases | M2 | Survey / R1 |
| 4 | Lexer Keywords: Operator Overloading | Desi (`tantrik`, `kriya`) and English (`operator`, `op`) aliases | M2 | Survey / R1 |
| 5 | Lexer Keywords: Pointer Operations | Address-of (`patta`, `thikana`, `addr`, `ptr`) and Deref (`asli`, `deref`, `val`, `*`) | M2 | Survey / R2 |
| 6 | Lexer Keywords: Rust Memory Model | Move (`rawana`, `move`), Own (`malik`, `own`), Mut (`badal`, `mut`), Const (`atal`, `const`), Drop (`khatam`, `drop`) | M2 | Survey / R2 |
| 7 | Parser & AST: Address-of Expressions | Parse prefix `&expr` and `patta expr` into `AddressOfExpr` | M2 | Survey / R2 |
| 8 | Parser & AST: Dereference Expressions | Parse prefix `*expr` and `asli expr` into `DerefExpr` | M2 | Survey / R2 |
| 9 | Parser & AST: Dereference Assignment | Parse `*ptr = val` and `asli ptr = val` into `DerefAssignExpr` | M2 | Survey / R2 |
| 10 | Parser & AST: Function Overload Declarations | Support overloaded function declarations sharing same identifier | M2 | Survey / R1 |
| 11 | Parser & AST: Operator Overload Declarations | Support operator overload declarations for binary and unary operators | M2 | Survey / R1 |
| 12 | Value Pointer Representation & Invariant | Integrate pointer variant into `ValueVariant` preserving `sizeof(Value) == 32` | M3 | Survey / R2, R4 |
| 13 | Type System: Pointer Types | Add `TypeKind::POINTER`, `PointerType`, and type checking for address-of/deref | M3 | Survey / R2 |
| 14 | Type System: Overload Signature Matching | Overload scoring algorithm based on parameter arity and type compatibility | M3 | Survey / R1 |
| 15 | Runtime Function Overloading Dispatch | Arity and type-based overload resolution in `CallExpr` via `OverloadSet` | M4 | Survey / R1 |
| 16 | Runtime Operator Overload Execution | Binary and unary Tantrik dispatch (`__jod__`, `__ghata__`, `__ulta__`, `__khol__`) | M4 | Survey / R1 |
| 17 | Runtime Pointer Operations Execution | Address-of reference resolution, dereference reading, and deref pointer write | M4 | Survey / R2 |
| 18 | Rust Memory Model: Ownership & Safe Lifecycle | Prevent access to invalid or dropped memory; manage lifetime scopes | M4 | Survey / R2 |
| 19 | Rust Memory Model: Move Semantics | `rawana`/`move` invalidates source variable (`isMoved`), prevents use-after-move | M4 | Survey / R2 |
| 20 | Rust Memory Model: Borrow Checking | Prevent conflicting exclusive/shared borrows and invalid pointer creation | M4 | Survey / R2 |
| 21 | Rust Memory Model: Drop Cleanup | `khatam`/`drop` invocation on instance exit or explicit destruction | M4 | Survey / R2 |
| 22 | Rust Benchmark: Recursive Fibonacci | Paired implementation in MEMElang and Rust (`fib(20)`) | M5 | Survey / R3 |
| 23 | Rust Benchmark: Sieve of Eratosthenes | Paired implementation in MEMElang and Rust (`sieve(100,000)`) | M5 | Survey / R3 |
| 24 | Rust Benchmark: Modular Math Loop | Paired implementation in MEMElang and Rust (`math_loop(200,000)`) | M5 | Survey / R3 |
| 25 | Rust Benchmark: Memory Allocation Drop | Paired implementation in MEMElang and Rust (`tree_drop(14)`) | M5 | Survey / R3 |
| 26 | Automated Benchmark Runner | Python runner measuring compilation/parsing and runtime execution speeds with comparative metrics | M5 | Survey / R3 |
| 27 | Core Regression Invariant | Ensure all 25 test suites in `src/tests/test_suite.cpp` pass via `make test` | M6 | Survey / R4 |
| 28 | C++ Runtime Size Invariant | Maintain `static_assert(sizeof(Value) == 32)` without error | M6 | Survey / R4 |
| 29 | Repository Forbidden Word Invariant | Zero occurrences of forbidden word across entire repository | M6 | Survey / R4 |
| 30 | E2E Test Suite & Adversarial Hardening | 100% pass of Tiers 1-4 tests, followed by Tier 5 adversarial coverage hardening | M6 | Survey / Acceptance |

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Cleanse & Baseline Verification | Sanitize 18 peripheral forbidden word instances; verify `make test` 25/25 pass and 0 repository occurrences | none | DONE |
| M2 | Lexer & Parser Extensions | Tokenize `&`, `*`, keywords for overloading, pointers, and memory model; AST nodes for address-of, deref, and declarations | M1 | DONE |
| M3 | Value & Type System Integration | Add pointer variant preserving `sizeof(Value) == 32`; pointer types and overload scoring in TypeChecker | M2 | DONE |
| M4 | Runtime Overloading & Safe Memory Engine | OverloadSet call resolution, unary/binary operator dispatch, pointer read/write, ownership/move/borrow safety rules | M3 | DONE |
| M5 | Comparative Rust Benchmark Suite | Implement 4 paired benchmarks in MEMElang & Rust; build automated benchmark runner script | M4 | PLANNED |
| M6 | E2E Test Suite Pass & Adversarial Hardening | Pass 100% E2E tests (Tiers 1-4), Tier 5 adversarial hardening, verify 25 suites regression-free & 0 forbidden word | M5, E2E Track | PLANNED |

## Interface Contracts

### Lexer ↔ Parser
- `TokenType::ADDRESS_OF`: Produced by single `&` or keywords `patta`, `thikana`, `addr`, `address_of`, `ptr`.
- `TokenType::DEREF`: Produced by unary `*` or keywords `asli`, `deref`, `val`, `dereference`.
- `TokenType::OVERLOAD`: Produced by `bahuroop`, `aur_ek`, `ek_aur`, `overload`, `multimethod`.
- `TokenType::OPERATOR`: Produced by `tantrik`, `kriya`, `operator`, `op`.
- `TokenType::MOVE`: Produced by `rawana`, `khisko`, `chal_nikal`, `move`.
- `TokenType::OWNED`: Produced by `malik`, `kabza`, `malkiyat`, `own`, `owned`.
- `TokenType::MUT`: Produced by `badal`, `mut`.
- `TokenType::CONST`: Produced by `atal`, `const`.
- `TokenType::DROP`: Produced by `khatam`, `alvida`, `drop`.

### Parser ↔ Runtime / Interpreter
- `AddressOfExpr`: Holds target expression (typically identifier or member access); returns `PointerValue`.
- `DerefExpr`: Holds pointer expression; evaluates pointer and reads target value.
- `DerefAssignExpr`: Holds pointer expression and value expression; writes value into pointed memory address.
- `OverloadSet`: Replaces direct overwrite in `Environment::define`; aggregates candidate `UserFunction` pointers by identifier.
- `Overload Resolution`: Filters candidates by parameter arity; selects best candidate by type compatibility score.
- `Tantrik Unary/Binary Dispatch`: Evaluates dunder methods (`__jod__`, `__ulta__`, `__khol__`) when operator expressions are evaluated on instances.
- `Borrow / Ownership Semantics`: Updates `VariableSlot::isMoved`, `VariableSlot::hasExclusiveBorrow`, `VariableSlot::sharedBorrows`. Rejects access to moved variables with runtime error.

### Value Invariant (`sizeof(Value) == 32`)
- Pointer representation must use `std::shared_ptr<Pointer>` or boxed reference $\le 16$ bytes payload.
- Payload $\le 24$ bytes ensures `sizeof(Value) == 32` on 64-bit platforms, maintaining `static_assert(sizeof(Value) == 32)`.

## Code Layout
- `src/lexer/Lexer.h`: Lexer token definitions, keyword maps, scanner logic.
- `src/parser/Parser.h`: AST node classes, recursive descent parsing.
- `src/types/Type.h`, `src/types/TypeChecker.h`, `src/types/TypeChecker.cpp`: Type representation, signature scoring, type checking.
- `src/interpreter/Value.h`: Value representation, `sizeof(Value) == 32` static assert, variant types.
- `src/interpreter/Environment.h`: Scopes, symbol storage, overload tables, borrow state tracking.
- `src/interpreter/Interpreter.cpp`: AST evaluation, call resolution, operator dispatch, pointer deref read/write, lifecycle/drop.
- `src/tests/test_suite.cpp`: 25 core test suites (regression target).
- `benchmarks/`: Comparative benchmark implementations (`*.meme` and `*.rs`), and `run_rust_benchmarks.py`.
- `tests/e2e/`: Opaque-box E2E test suite covering Tiers 1-4.
