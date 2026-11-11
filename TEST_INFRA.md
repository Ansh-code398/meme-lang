# MEMElang End-to-End (E2E) Test Infrastructure Specification

## 1. Overview & Objectives

The MEMElang E2E test infrastructure provides a comprehensive, automated, opaque-box testing framework validating the language features, self-hosting toolchain, benchmarks, and architectural invariants defined in `PROJECT.md` and `ORIGINAL_REQUEST.md`:

1. **R1: Self-Hosting Compiler & Bootstrapping Cycle**
   - Self-hosted compiler written entirely in MEMElang (`compiler.meme`).
   - Compilation of `.meme` programs into native executable binaries (`-o`).
   - Execution verification of compiled target binaries.
   - Host toolchain and runtime module parity (`sys.args`, `file` I/O, `sys.exec`/`exit`).
   - Multi-stage self-hosting closure: Stage 0 (interpreted) $\to$ Stage 1 (native compiled) $\to$ Stage 2 (self-compiled) with 100% bitwise identical C++ emission (`diff -u`).

2. **R2: Language Extensions for Large-Scale Projects**
   - **Module Packaging & Exports**: Dual-syntax `potli`/`package`/`dibbi`/`bundle`/`pkg` and `bech`/`export`/`khula`/`baahar`/`pub`.
   - **Namespaces & Scope Resolution**: Dual-syntax `mohalla`/`namespace`/`pados`/`ns`, scope resolution operator `::` (`DOUBLE_COLON`), nested namespaces.
   - **Algebraic Option & Result Types**: `Option` (`Kuch`/`Some`, `Khali`/`None`) and `Result` (`Sahi`/`Ok`, `Galat`/`Err`) boxed on heap preserving `sizeof(Value) == 32`.
   - **Error Handling & Propagation**: Postfix `?` operator (`TryPropagateExpr`) unwrapping Option/Result or returning early, `try`/`catch` (`panga`/`lafda`), `throw` (`chillao`).
   - **Advanced Pattern Matching**: Expression-based `jaanch`/`match` with variant matching, list destructuring, guards (`agar`/`if`), and default wildcard (`warna`/`default`/`_`).
   - **Project Build Tooling**: Manifest `tapri.json` detection, project compilation via `meme build`, and automated test execution via `meme test`.

3. **R3: Automated Benchmarking Suite**
   - Comprehensive benchmarking runner `benchmarks/run_comprehensive_benchmarks.py`.
   - Compilation speed profiling in lines of code per second (LOC/sec).
   - Runtime throughput across 7 standardized workloads (`fib`, `sieve`, `math_loop`, `tree_drop`, `quicksort`, `json`, `csv`) measuring ops/sec, min, mean, median, and stdev.
   - Memory usage profiling (peak RSS) using OS-normalized `getrusage` (macOS bytes vs Linux KB).
   - 4-way comparative metrics: Self-Hosted Native vs C++ Native vs Interpreter vs Baselines (Rust / Python / Node).

4. **R4: Strict Architectural Invariants & Regression Safety**
   - Repository-wide zero-tolerance invariant: 0 occurrences of the forbidden word across the entire repository.
   - Tagged union size invariant: `sizeof(Value) == 32` static assertion strictly enforced.
   - Zero test regressions on existing 25 unit test suites (`make test`) and baseline E2E suites.

---

## 2. Directory Layout & Organization

All E2E test suites, negative boundary tests, and runner scripts reside in `tests/e2e/`:

```
tests/e2e/
├── run_e2e_tests.py                        # Master E2E runner, classifier, and invariant auditor
├── run_e2e_tests.sh                        # Shell wrapper for master runner
├── test_overloading.meme                   # Baseline T1 & T2: Function & operator overloading
├── test_pointers.meme                      # Baseline T1 & T2: Pointer address-of and dereference
├── test_memory_safety.meme                 # Baseline T1 & T2: Ownership, move semantics, borrow checker
├── test_aliases.meme                       # Baseline T1 & T2: Dual-lexicon keyword parity
├── test_combinations.meme                  # Baseline T3: Pairwise cross-feature interactions
├── test_scenarios.meme                     # Baseline T4: Real-world application scenarios
├── test_adversarial_overloading.meme      # Baseline T5: Adversarial overloading & operator hardening
├── test_tier5_adversarial_pointers_memory.meme # Baseline T5: Adversarial pointers & drop cascades
├── test_dikha_improvements.meme           # Baseline T1: String formatting & stdlib enhancements
├── test_packaging_modules.meme            # R2 T1 & T2: Module packaging (potli) & exports (bech)
├── test_namespaces.meme                   # R2 T1 & T2: Namespaces (mohalla) & scope resolution (::)
├── test_option_result.meme                # R2 T1 & T2: Algebraic Option (Kuch/Khali) & Result (Sahi/Galat)
├── test_error_handling.meme               # R2 T1 & T2: Postfix ? propagation & exception blocks
├── test_pattern_matching.meme             # R2 T1 & T2: Advanced jaanch/match, destructuring, guards
├── test_cross_feature_combinations.meme   # R2 T3: Pairwise cross-feature combinations
├── test_real_world_scenarios.meme         # R2 T4: Real-world large-scale application architectures
├── test_compiler_bootstrap_e2e.py         # R1 T1-T4: Self-hosting compiler CLI, codegen & bootstrapping
├── test_benchmarks_e2e.py                 # R3 T1 & T2: Benchmarking suite metrics & CLI options
├── test_build_tooling_e2e.py              # R2 T1 & T2: tapri.json project build tooling
├── test_invariants_e2e.py                 # R4 T1 & T2: Repo-wide forbidden word, sizeof(Value), 0 regressions
└── negative/                               # Negative compile-time & fatal diagnostic boundary tests
    ├── test_err_ambiguous_overload.meme
    ├── test_err_atal_reassign.meme
    ├── test_err_const_pointer_mutate.meme
    ├── test_err_double_mutable_borrow.meme
    ├── test_err_loop_unhandled_use_after_move.meme
    ├── test_err_no_matching_overload.meme
    ├── test_err_null_deref.meme
    ├── test_err_rvalue_address_of.meme
    ├── test_err_use_after_move.meme
    ├── test_err_unexported_symbol.meme
    ├── test_err_namespace_not_found.meme
    ├── test_err_unwrap_none.meme
    ├── test_err_unwrap_err.meme
    ├── test_err_question_on_primitive.meme
    └── test_err_match_non_exhaustive.meme
```

---

## 3. Four-Tier Testing Methodology

The test suite enforces a 4-tier methodology (plus Tier 5 adversarial hardening) guaranteeing deep coverage across all requirements:

### Tier 1: Feature Coverage (Core Functionality)
- **Objective**: Verify primary (happy path) behavior of every language construct and toolchain component.
- **Rule**: $\ge 5$ test cases per feature covering both Desi meme syntax and English aliases.
- **Coverage**:
  - **Function & Operator Overloading**: Arity (1, 2, 3), parameter types (`ank`, `bol`, `sach`), arithmetic (`+`, `-`, `*`), comparisons (`==`, `!=`), indexing (`[]`, `[]=`), unary negation (`-`).
  - **Pointers & Memory Safety**: Address-of (`&`, `patta`), dereference read/write (`*`, `asli`), move semantics (`rawana`, `move`), ownership (`malik`, `own`), mutability (`badal`/`atal`), explicit drop (`khatam`/`drop`), lexical scopes (`daayra`/`scope`), shared borrows (`udhaar`).
  - **Module Packaging & Exports**: `potli`/`package`/`dibbi`/`bundle`/`pkg`, `bech`/`export`/`khula`/`baahar`/`pub`, exporting functions, classes, and variables.
  - **Namespaces & Scope Resolution**: `mohalla`/`namespace`/`pados`/`ns`, scope resolution `::`, calling namespaced functions, accessing constants, instantiating namespaced classes.
  - **Algebraic Option & Result**: `Kuch`/`Some`, `Khali`/`None`, `Sahi`/`Ok`, `Galat`/`Err`, `.is_some()`, `.is_none()`, `.is_ok()`, `.is_err()`, `.unwrap()`, `.unwrap_or()`, `.unwrap_err()`.
  - **Error Propagation**: Postfix `?` unwrapping on Option and Result, `panga ... lafda` (`try ... catch`), `chillao` (`throw`).
  - **Pattern Matching**: `jaanch`/`match` on primitives, Option variants, Result variants, empty list `[]`, single item `[x]`, list pair `[a, b]`, guards (`agar`/`if`).
  - **Compiler & Toolchain**: `compiler.meme` CLI flags, native binary compilation and execution, `sys.args` passing, `file` I/O, `sys.exec`, `tapri.json` build.
  - **Benchmarking Suite**: CLI runner invocation, compilation LOC/sec, throughput ops/sec, peak RSS memory via `getrusage`, 4-way comparative metrics.
  - **Architectural Invariants**: Test dir 0 forbidden words, src dir 0 forbidden words, `sizeof(Value) == 32`, 25/25 `make test` suites pass.

### Tier 2: Boundary, Edge Cases & Error Diagnostics
- **Objective**: Verify edge conditions, numeric scoring, type widening, error propagation boundaries, and diagnostic messages.
- **Rule**: $\ge 5$ test cases per feature verifying that invalid, ambiguous, or out-of-bounds states fail cleanly.
- **Coverage**:
  - Overload scoring disambiguating exact integer match from floating-point match.
  - Nested namespaces (`Outer::Inner::SubInner::member`), identifier shadowing between local and namespaced members.
  - Unexported private helper isolation (unexported symbols inaccessible from external modules).
  - Trapping `.unwrap()` on `Khali`/`None` and `Galat`/`Err` via `panga ... lafda`.
  - Nested variants (`Some(Some(x))`, `Ok(Err(e))`, `Some([x, y])`).
  - Chained `?` operator propagation pipelines with guaranteed early exit without side-effects.
  - Pattern matching precedence, complex boolean guards, list destructuring with rest elements (`[head, ...tail]`).
  - Compiler error diagnostics on missing source files, syntax errors, and invalid CLI flags.
  - Benchmark custom iteration flags, workload filters, and invalid workload validation.
  - Dedicated standalone negative test files verifying non-zero exit codes and matching error substrings.

### Tier 3: Cross-Feature Combinations (Pairwise Interactions)
- **Objective**: Verify orthogonal language features behave predictably when composed.
- **Coverage**:
  - Namespaces containing functions returning Option and Result.
  - Calling namespaced functions and pattern matching on their return values with `jaanch`.
  - Pattern matching variants combined with `?` error propagation in single expression pipelines.
  - Package exports containing namespaced classes and overloaded methods.
  - Function overloading within namespaces returning Option/Result types.
  - Pointers stored inside Option types (`Kuch(&val)`), dereferenced and mutated safely.
  - Pattern matching with destructuring and guards over heterogeneous lists of Options.
  - Pattern match expressions executing inside exception blocks (`panga ... lafda`).
  - Multi-stage pipelines chaining `?` across multiple distinct namespaces.

### Tier 4: Real-World Application Scenarios
- **Objective**: Validate language ergonomics, performance, and stability in realistic non-trivial architectures.
- **Scenarios**:
  1. `PackageManagerRegistry`: Module registry and semver dependency resolver using packages, namespaces, Option/Result, and pattern matching.
  2. `JsonParserAstEngine`: Recursive descent tokenizer and JSON AST parser using token variants, Option lookahead, Result error handling, and `?` propagation.
  3. `ConfigurationPipeline`: Application configuration loader reading settings, validating port ranges, matching environments, and resolving fallbacks via `.unwrap_or()`.
  4. `TaskWorkflowDispatcher`: State machine managing asynchronous tasks (`Pending`, `Running`, `Completed`, `Failed`) with transitions matched via guards.
  5. `ExpressionAstEvaluator`: Recursive arithmetic AST evaluator (`Add`, `Sub`, `Mul`, `Div`) evaluating nodes with `jaanch`, detecting division-by-zero, and propagating evaluations via `?`.
  6. `Self-Hosting Bootstrap Cycle`: Stage 0 $\to$ Stage 1 $\to$ Stage 2 compiling `compiler.meme` and verifying bitwise identical C++ emission (`diff -u`).

### Tier 5: Adversarial Hardening (Challenger Suites)
- **Objective**: Stress-test extreme edge cases, deep indirection, cyclic drops, and complex overload resolution.
- **Coverage**:
  - `test_adversarial_overloading.meme`: Mixed Desi/English keywords in single scope, class hierarchy overload inheritance, closures capturing state, chained operator expressions.
  - `test_tier5_adversarial_pointers_memory.meme`: 4-level and 5-level pointer indirection chains, nested container pointers (`&grid[1][0].val`), moves inside loops, cyclic reference drops.

---

## 4. Test Execution & Assertion Model

### 4.1 Opaque-Box Execution
Tests execute against the compiled `./meme` binary without internal mockups:
```bash
./meme tests/e2e/test_packaging_modules.meme
python3 tests/e2e/run_e2e_tests.py
```

### 4.2 In-Language Assertion Helper
Positive `.meme` suites utilize structured assertion logging:
```meme
jugaad assert_eq(actual, expected, test_name: bol) {
    dekh actual != expected {
        dikha f"[FAIL] {test_name}: Expected '{expected}', got '{actual}'";
        chillao f"Assertion failed in {test_name}";
    }
    dikha f"[PASS] {test_name}";
}
```
If an assertion fails, execution aborts with a runtime error (`chillao`), producing a non-zero exit code. If all assertions pass, each test emits `[PASS] <test_name>` and the script exits with code 0.

### 4.3 Negative Boundary Test Execution
Negative test cases specify expected diagnostic substrings in a header directive:
```meme
// EXPECT_ERROR: Khufiya
```
The test runner executes `./meme <negative_file.meme>`, verifies `returncode != 0`, and checks that stdout or stderr contains the expected diagnostic string.

### 4.4 Bootstrapping Closure Execution
The self-hosting bootstrap test verifies the fixed-point convergence cycle:
```bash
./meme compiler.meme compiler.meme -o meme_stage1
./meme_stage1 compiler.meme -o meme_stage2
diff -u meme_stage1.cpp meme_stage2.cpp
```
An exit code of 0 confirms bitwise identical C++ emission.

---

## 5. Test Runner CLI Reference

The master automated test runner `tests/e2e/run_e2e_tests.py` supports flexible execution:

| Objective | Command |
| :--- | :--- |
| **Run Full E2E Test Suite** | `python3 tests/e2e/run_e2e_tests.py` (or `./tests/e2e/run_e2e_tests.sh`) |
| **List Test Catalog & Inventory** | `python3 tests/e2e/run_e2e_tests.py --list` |
| **Run Specific Tier** | `python3 tests/e2e/run_e2e_tests.py --tier 1` (or `2`, `3`, `4`, `5`) |
| **Run Specific Suite** | `python3 tests/e2e/run_e2e_tests.py --suite namespaces` |
| **Run Negative Tests Only** | `python3 tests/e2e/run_e2e_tests.py --negative-only` |
| **Run Baseline Suites Only** | `python3 tests/e2e/run_e2e_tests.py --baseline-only` |
| **Run Staged Suites Only** | `python3 tests/e2e/run_e2e_tests.py --staged-only` |
| **Enforce Strict 100% Pass (M6)** | `python3 tests/e2e/run_e2e_tests.py --strict` |
| **Verbose Output** | `python3 tests/e2e/run_e2e_tests.py -v` |
| **Custom Binary Path** | `python3 tests/e2e/run_e2e_tests.py --meme /path/to/meme` |

---

## 6. Strict Integrity & Invariant Constraints

1. **Zero Forbidden Words**:
   - The forbidden word must NEVER appear in test code, comments, string literals, filenames, or outputs.
   - The test runner enforces an automated pre-flight regex audit across test directories and repository-wide files.
2. **No Source Modifications by Test Writers**:
   - Test tracks strictly write and modify test code (`tests/e2e/`) and documentation (`TEST_INFRA.md`, `TEST_READY.md`). No modifications to `src/` are permitted.
3. **Regression Safety**:
   - All 25 baseline C++ test suites in `src/tests/test_suite.cpp` pass cleanly via `make test`.
   - All 337 baseline E2E test cases pass with zero regressions.
4. **Memory Footprint & Tagged Union Invariant**:
   - All language additions maintain `sizeof(Value) == 32` static compile-time assertion.
