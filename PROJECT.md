# Project: Production-Ready Self-Hosting MEMElang Compiler, Language Extensions & Comprehensive Benchmarking Suite

## Architecture
- **Language**: MEMElang (C++20 runtime host toolchain, clang++ native compiler backend).
- **Frontend & Parser**:
  - `src/lexer/Lexer.h`: Dual-syntax tokenization supporting Desi parody keywords and standard English aliases.
  - `src/parser/Parser.h`: AST expression and statement nodes, Pratt/precedence-climbing recursive descent parser.
  - `src/types/Type.h` & `src/types/TypeChecker.cpp`: Static & gradual type analysis, signature scoring, subtyping compatibility.
- **Runtime & Environment**:
  - `src/interpreter/Value.h`: Runtime tagged union with boxed heap containers ensuring `sizeof(Value) == 32` static assertion.
  - `src/interpreter/Environment.h`: Scoped lexical environments, borrow checking, and overload symbol sets.
  - `src/interpreter/Interpreter.cpp`: AST execution engine with builtins (`file`, `sys`/`systumm`, `math`, `samay`, `collections`).
- **Native AOT Compiler Pipeline**:
  - `src/compiler/NativeCompiler.h` & `src/compiler/NativeCompiler.cpp`: Translates MEMElang AST into clean, high-performance C++20 code.
  - `src/compiler/MemeRuntime.h`: Self-contained C++ runtime providing `MemeValue`, file I/O, subprocess execution, CLI arguments, and standard modules.
- **Self-Hosted Compiler (`compiler.meme`)**:
  - Written entirely in MEMElang (`.meme`).
  - Architecture: Lexer, Parser, AST Model, Symbol Resolver, and C++20 Code Generator.
  - Produces standalone native binaries via host `clang++ -std=c++20 -O3 -I.`.
  - Self-hosting closure: Stage 0 (interpreted) $\to$ Stage 1 (native compiled) $\to$ Stage 2 (self-compiled) with 100% bitwise identical C++ emission (`diff -u`).
- **Benchmarking Suite (`benchmarks/`)**:
  - Automated runner `benchmarks/run_comprehensive_benchmarks.py`.
  - Evaluates compilation speed (LOC/sec), runtime throughput (operations/sec, min/mean/median/stdev across 7 workloads), and peak RSS memory via OS-normalized `getrusage`.
  - 4-way comparative performance matrix: Self-Hosted Native vs C++ Native vs Interpreter vs Baseline runtimes (Rust / Python / Node).

---

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | Forbidden Word Repository Cleansing | Sanitize all 10 legacy occurrences of the forbidden word in `meme_minimal.cpp` and recompile binary to enforce strict 0 repository-wide occurrences | M1 | Survey / R4 |
| 2 | Runtime Size & Regression Verification | Confirm `sizeof(Value) == 32` static assert and pass 25/25 `make test` suites and 337/337 `tests/e2e` cases without regressions | M1 | Survey / R4 |
| 3 | Lexer & Parser: Module Packaging & Exports | Dual-syntax keywords `potli`/`package` and `bech`/`export`, AST `PackageDeclStmt` and `ExportStmt` | M2 | Survey / R2 |
| 4 | Lexer & Parser: Namespaces & Scope Resolution | Dual-syntax `mohalla`/`namespace`, `DOUBLE_COLON` (`::`), AST `NamespaceDeclStmt` and `ScopedAccessExpr` | M2 | Survey / R2 |
| 5 | Value & Types: Algebraic Option & Result Types | `Option` (`Kuch`/`Some`, `Khali`/`None`) and `Result` (`Sahi`/`Ok`, `Galat`/`Err`) boxed in `std::shared_ptr` to preserve `sizeof(Value) == 32` | M2 | Survey / R2, R4 |
| 6 | Lexer & Parser: Error Propagation Operator | Postfix `?` operator (`TryPropagateExpr`) for early-return error unwrapping | M2 | Survey / R2 |
| 7 | Lexer & Parser: Advanced Pattern Matching | Expression-based `jaanch`/`match` (`JaanchExpr`), variant matching, list destructuring, and guards (`agar`/`if`) | M2 | Survey / R2 |
| 8 | Project Build Tooling Manifest Support | Upgrade `src/main.cpp` `meme build` and `meme test` to detect and compile `tapri.json` projects into native binaries | M2 | Survey / R2 |
| 9 | Native Compiler: CLI Argument Plumbing | Plumb `argc`/`argv` in `NativeCompiler.cpp` main entrypoint into `sys.args` | M3 | Survey / R1, R2 |
| 10 | Native Runtime: File I/O Module | Add `FileModule file;` in `MemeRuntime.h` (`read`, `write`, `exists`, `lines`) | M3 | Survey / R1 |
| 11 | Native Runtime: Systumm / OS Module | Add `SystummModule sys, os;` in `MemeRuntime.h` (`args`, `exec` via popen, `cwd`, `platform`, `exit`) | M3 | Survey / R1 |
| 12 | Native Runtime: Map / Dictionary Storage | Add `MemeValue::TYPE_MAP` with `std::unordered_map<std::string, MemeValue>` in `MemeRuntime.h` | M3 | Survey / R1 |
| 13 | Native Runtime: String Utility Methods | Parity string methods (`charAt`, `chars`, `slice`, `length`, `starts_with`, `find`) in `MemeRuntime.h` | M3 | Survey / R1 |
| 14 | Native Compiler: Exception Codegen | Implement `TryCatchStmt` and `ThrowStmt` code emission in `NativeCompiler.cpp` | M3 | Survey / R1, R2 |
| 15 | Native Compiler: Match & Enum Codegen | Implement `JaanchStmt`/`JaanchExpr` code emission in `NativeCompiler.cpp` | M3 | Survey / R1, R2 |
| 16 | Self-Hosted Lexer Module in MEMElang | Lexical scanner in `.meme` reading source characters and emitting structured tokens | M4 | Survey / R1 |
| 17 | Self-Hosted AST & Parser in MEMElang | Recursive descent / precedence climbing parser in `.meme` building expression and statement AST nodes | M4 | Survey / R1 |
| 18 | Self-Hosted Semantic & Scope Resolver | Symbol table and scope management in `.meme` tracking declarations and functions | M4 | Survey / R1 |
| 19 | Self-Hosted C++ Code Generator | AST visitor in `.meme` translating MEMElang constructs into C++20 code referencing `MemeRuntime.h` | M4 | Survey / R1 |
| 20 | Self-Hosted CLI Driver & Toolchain Shellout | Compiler driver in `.meme` parsing `sys.args`, writing `.cpp`, and invoking host `clang++ -std=c++20 -O3` | M4 | Survey / R1 |
| 21 | Self-Hosting Bootstrap Stage 0 | Execute `compiler.meme` under host `./meme` to compile `compiler.meme` into native executable `meme_stage1` | M4 | Survey / R1 |
| 22 | Self-Hosting Bootstrap Stage 1 | Execute native executable `meme_stage1` to compile `compiler.meme` into native executable `meme_stage2` | M4 | Survey / R1 |
| 23 | Self-Hosting Bootstrap Stage 2 Closure | Verify bitwise identical C++ emission (`diff -u meme_stage1.cpp meme_stage2.cpp` == 0) and fixed-point closure (`meme_stage3`) | M4 | Survey / R1 |
| 24 | Compilation Speed Benchmarks | Multi-scale compilation benchmarks (small: `fib`, medium: `dataframe`, macro: `compiler.meme`, stress: synthetic) reporting LOC/sec and phase timings | M5 | Survey / R3 |
| 25 | Runtime Throughput Benchmarks | 7 algorithmic & data workloads (`fib`, `sieve`, `math_loop`, `tree_drop`, `quicksort`, `json`, `csv`) reporting ops/sec, min/mean/median/stdev | M5 | Survey / R3 |
| 26 | Memory Usage Benchmarks (Peak RSS) | OS-normalized `getrusage` memory profiling (macOS bytes vs Linux KB) measuring compiler and runtime peak RSS | M5 | Survey / R3 |
| 27 | 4-Way Comparative Performance Reporting | Automated runner `benchmarks/run_comprehensive_benchmarks.py` generating JSON metrics and Markdown comparison table against Rust, Python, Node | M5 | Survey / R3 |
| 28 | Comprehensive E2E Test Suite Pass (Tiers 1-4) | 100% pass of requirement-driven opaque-box E2E test suite published by E2E Testing Track | M6 | Survey / Acceptance |
| 29 | Tier 5 Adversarial Coverage Hardening | White-box adversarial testing by Challenger agents targeting compiler edge cases, bootstrap edge cases, and runtime invariants | M6 | Survey / Acceptance |
| 30 | Master Invariants & Zero Regression Audit | Final forensic audit confirming zero forbidden words repository-wide, `sizeof(Value) == 32`, and all existing test suites passing | M6 | Survey / Acceptance |

---

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Cleanse & Baseline Verification | Sanitize 10 legacy occurrences of the forbidden word in `meme_minimal.cpp`, recompile binary, verify 0 occurrences repository-wide, verify `sizeof(Value) == 32`, and verify all existing tests pass | none | DONE |
| M2 | Language Extensions: Packaging, Data Structures & Error Handling | Implement `package`/`export`, `namespace`/`::`, `Option`/`Result`, `?` operator, `match`/`jaanch` expressions, and `meme build` manifest support | M1 | DONE |
| M3 | Native Compiler Runtime & Codegen Parity | Implement `sys.args` CLI plumbing, `FileModule`, `SystummModule`, `MemeValue::TYPE_MAP`, string methods in `MemeRuntime.h`, and `try`/`catch`/`throw`/`match` codegen in `NativeCompiler.cpp` | M2 | DONE |
| M4 | Self-Hosting Compiler Implementation & Bootstrapping Cycle | Write self-hosted compiler in `compiler.meme`, execute Stage 0 $\to$ Stage 1 $\to$ Stage 2, and verify bitwise identical fixed-point bootstrap closure | M3 | DONE |
| M5 | Comprehensive Benchmarking Suite | Build `benchmarks/run_comprehensive_benchmarks.py` measuring compilation speed, runtime throughput, memory usage, and 4-way comparative performance matrix | M4 | DONE |
| M6 | Final E2E Pass & Adversarial Hardening | Pass 100% of E2E test suite (Tiers 1-4), execute Tier 5 adversarial coverage hardening, and perform final forensic audit | M5, E2E Track | DONE |

---

## Interface Contracts

### Lexer ↔ Parser
- `TokenType::PACKAGE`: Produced by `potli`, `dibbi`, `bundle`, `package`, `pkg`.
- `TokenType::EXPORT`: Produced by `bech`, `khula`, `baahar`, `export`, `pub`.
- `TokenType::NAMESPACE`: Produced by `mohalla`, `pados`, `namespace`, `ns`.
- `TokenType::DOUBLE_COLON`: Produced by `::` for scoped member resolution.
- `TokenType::QUESTION`: Postfix `?` operator for error propagation unwrapping.
- `TokenType::OPTION_SOME` / `OPTION_NONE`: Produced by `Kuch`/`Some` and `Khali`/`None`.
- `TokenType::RESULT_OK` / `RESULT_ERR`: Produced by `Sahi`/`Ok` and `Galat`/`Err`.

### Value Tagged Union Invariant (`sizeof(Value) == 32`)
- Primitive variants (`long long`, `double`, `bool`, `char`) and `std::string` (24 bytes).
- All compound heap types (`MemeOption`, `MemeResult`, `MemeList`, `MemeMap`, `Pointer`, `Instance`) MUST be boxed in `std::shared_ptr<T>` (16 bytes).
- Maximum variant payload is 24 bytes + 1 byte discriminant aligned to 8 bytes = exactly 32 bytes on 64-bit systems.
- Compile-time assertion `static_assert(sizeof(Value) == 32)` strictly enforced.

### Compiler ↔ Native Runtime (`MemeRuntime.h`)
- CLI Args: `sys.args` is a `std::vector<std::string>` populated from `argc`/`argv` in `main`.
- File I/O:
  - `file.read(path)` $\to$ returns file contents as `std::string`.
  - `file.write(path, content)` $\to$ writes string to path, returns `bool`.
  - `file.exists(path)` $\to$ returns `bool`.
  - `file.lines(path)` $\to$ returns `std::vector<std::string>`.
- Systumm Execution:
  - `sys.exec(cmd)` $\to$ executes command via `popen`, returns stdout as string.
  - `sys.exit(code)` $\to$ terminates process with code.
- Map Operations:
  - `MemeValue` supports dictionary lookups: `val[key]`, `val.has(key)`, `val.keys()`.

### Bootstrapping Closure Contract
- Stage 0: `./meme compiler.meme compiler.meme -o meme_stage1` $\to$ emits `meme_stage1.cpp` and compiles `meme_stage1`.
- Stage 1: `./meme_stage1 compiler.meme -o meme_stage2` $\to$ emits `meme_stage2.cpp` and compiles `meme_stage2`.
- Stage 2: `diff -u meme_stage1.cpp meme_stage2.cpp` returns 0 differences (exit code 0). Fixed-point convergence confirmed.

---

## Code Layout
- `src/lexer/Lexer.h`: Unified tokenization, keyword tables, lexical scanning.
- `src/parser/Parser.h`: AST expression and statement nodes, recursive descent parser.
- `src/types/Type.h`, `src/types/TypeChecker.cpp`: Type checker, signature scoring, gradual typing.
- `src/interpreter/Value.h`: Value variant, boxed representations, `sizeof(Value) == 32` static assertion.
- `src/interpreter/Environment.h`: Scoped symbol tables, borrow tracker, overload sets.
- `src/interpreter/Interpreter.cpp`: AST evaluation engine, built-in modules.
- `src/compiler/NativeCompiler.h`, `NativeCompiler.cpp`: C++20 AOT code generation pipeline.
- `src/compiler/MemeRuntime.h`: Header-only C++ runtime for compiled binaries.
- `src/main.cpp`: CLI toolchain routing (`compile`, `build`, `test`, `run`).
- `compiler.meme`: Self-hosted MEMElang compiler written in `.meme`.
- `benchmarks/run_comprehensive_benchmarks.py`: Automated benchmarking runner script.
- `tests/e2e/`: Opaque-box requirement-driven E2E test suite (Tiers 1-4).
