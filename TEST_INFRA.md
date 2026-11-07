# MEMElang End-to-End (E2E) Test Infrastructure Specification

## 1. Overview & Objectives

The MEMElang E2E test infrastructure provides an automated, opaque-box testing framework for validating the language extensions defined in `PROJECT.md` and `ORIGINAL_REQUEST.md`:
1. **Function Overloading** (parameter arity and type resolution, Desi & English syntax)
2. **Operator Overloading** (binary arithmetic, comparison, indexing, unary negation, Desi & English syntax)
3. **Pointer Operations** (address-of `&`/`patta`/`addr`, dereference read `*`/`asli`/`deref`, dereference write `*p = val`, Desi & English syntax)
4. **Rust-Inspired Memory Management** (ownership `malik`/`own`, move semantics `rawana`/`move`, mutability `badal`/`atal`, explicit drop `khatam`/`drop`, scoped blocks `daayra`/`scope`, borrow checker)
5. **Dual-Lexicon Ergonomics** (strict keyword equivalence between Desi meme keywords and English aliases)

---

## 2. Directory Layout & Organization

All E2E test suites and runner scripts reside in `tests/e2e/`:

```
tests/e2e/
├── run_e2e_tests.py                   # Automated Python test runner and reporter
├── run_e2e_tests.sh                   # Executable bash wrapper
├── test_overloading.meme              # Tiers 1 & 2: Function & operator overloading
├── test_pointers.meme                 # Tiers 1 & 2: Pointer address-of and dereference
├── test_memory_safety.meme            # Tiers 1 & 2: Ownership, move semantics, borrow checking
├── test_aliases.meme                  # Tiers 1 & 2: Exhaustive Desi vs English keyword equivalence
├── test_combinations.meme             # Tier 3: Pairwise cross-feature interactions
├── test_scenarios.meme                # Tier 4: Real-world multi-feature application programs
└── negative/                          # Dedicated negative compile-time/fatal boundary tests
    ├── test_err_ambiguous_overload.meme
    ├── test_err_atal_reassign.meme
    ├── test_err_double_mutable_borrow.meme
    ├── test_err_no_matching_overload.meme
    ├── test_err_null_deref.meme
    ├── test_err_rvalue_address_of.meme
    └── test_err_use_after_move.meme
```

---

## 3. Four-Tier Testing Methodology

The test suite enforces a rigorous 4-tier methodology guaranteeing deep test coverage:

### Tier 1: Feature Coverage (Core Functionality)
- **Objective**: Verify the primary (happy path) behavior of every language construct.
- **Requirements**: $\ge 5$ test cases per feature across both Desi meme syntax and English aliases.
- **Coverage**:
  - Function overloading by parameter count (arity 1, 2, 3) and types (`ank`, `bol`, `sach`).
  - Binary operator overloading (`+`, `-`, `*`), comparisons (`==`, auto-derived `!=`), indexing (`[]`, `[]=`), unary negation (`-`).
  - Pointer address-of (`&`, `patta`, `addr`, `ptr`) and dereference read/write (`*`, `asli`, `deref`, `val`).
  - Move semantics (`rawana`, `move`), ownership qualifiers (`malik`, `own`), mutability (`badal`/`mut`, `atal`/`const`), explicit drop (`khatam`/`drop`), lexical scope blocks (`daayra`/`scope`).
  - Shared borrowing (`udhaar`, `ref`) allowing concurrent readers.

### Tier 2: Boundary, Edge Cases & Error Diagnostics
- **Objective**: Verify edge conditions, numeric widening, type scoring, and error handling.
- **Requirements**: $\ge 5$ test cases per feature verifying that illegal or ambiguous states fail cleanly with descriptive meme diagnostics.
- **Coverage**:
  - Overload scoring disambiguating exact integer match from floating-point match.
  - Overloaded recursive functions with base and inductive signatures.
  - Double indirection pointers (`**pp`) and struct property pointers (`&obj.field`).
  - Boundary errors caught via runtime `panga ... lafda` exception blocks:
    - Null pointer dereference (`*nalla` triggering `"Chuna laga diya!"`).
    - Address-of temporary rvalue (`&(10+20)` triggering `"doglapan"`).
    - Use-after-move (`rawana var` then reading `var` triggering `"Pehli fursat mein nikal!"`).
    - Mutation of immutable bindings (`atal` reassignment triggering `"Shanti gadadhari Bheem!"`).
    - Double mutable borrows (`udhaar_badal` conflict triggering `"Ek mawaali ek time pe ek hi jagah!"`).
    - Move while borrowed (`udhaar` lock triggering `"Khatam nahi hua abhi!"`).
  - Dedicated standalone negative test files verifying non-zero exit codes and error output matching.

### Tier 3: Cross-Feature Combinations (Pairwise Interactions)
- **Objective**: Verify that orthogonal language features operate predictably when combined.
- **Requirements**: Pairwise interaction matrix covering overloads, pointers, and memory management.
- **Coverage**:
  - Overloaded functions resolving value parameters vs pointer parameters (`f(x: ank)` vs `f(p: patta[ank])`).
  - Operator overloading on owned structs transferred via move semantics (`malik` + `tantrik` + `rawana`).
  - Pointer dereference assignments executed within operator methods (`tantrik []=`).
  - Pointers taken to variables currently under active shared borrow (`udhaar`).
  - Moving a pointer variable (`rawana ptr`) vs moving the pointee value.
  - Explicit drop (`khatam`) on instances with custom overloaded operators.
  - Scoped blocks (`daayra`) allocating local slots, taking pointers, and invoking overloaded functions.
  - Classes combining method overloads and operator overloads.
  - Chained operator evaluation on moved temporary expressions.
  - Consistency between mutable borrows and dereference writes.

### Tier 4: Real-World Application Scenarios
- **Objective**: Validate language ergonomics and stability in realistic, non-trivial programs.
- **Requirements**: Complete multi-feature application architectures.
- **Coverage**:
  1. `AnokhaPtr[T]`: Smart unique-pointer container providing exclusive ownership, move semantics, dereference reading, and automatic RAII destruction.
  2. `PhysicsVector2D`: 2D physics engine utilizing operator overloading (`+`, `-`, `*`), string formatting (`__dikha__`), and overloaded force calculation functions.
  3. `SthalNode`: Singly linked list constructed using pointer address-of and dereference mutations, traversal, and cleanup.
  4. `DbSessionPool`: Scoped resource connection pool exercising lexical scope blocks (`daayra`), shared borrows, and deterministic RAII cleanup on scope exit.
  5. `PolymorphicMessageDispatcher`: Event routing architecture utilizing function overloading for distinct event payload classes (`ClickEvent`, `KeyEvent`, `SystemEvent`).

---

## 4. Test Execution & Assertion Model

### 4.1 Opaque-Box Execution
Tests execute directly against the compiled `./meme` binary without modifying or relying on internal compiler mockups:
```bash
./meme tests/e2e/test_overloading.meme
```

### 4.2 Assertion Mechanism
Positive test suites utilize an in-language assertion helper:
```meme
jugaad assert_eq(actual, expected, test_name: bol) {
    dekh actual != expected {
        dikha f"[FAIL] {test_name}: Expected '{expected}', got '{actual}'";
        chillao f"Assertion failed in {test_name}";
    }
    dikha f"[PASS] {test_name}";
}
```
If an assertion fails, execution terminates immediately with a runtime error (`chillao`), yielding a non-zero exit code. If all assertions pass, each test emits `[PASS] <test_name>` and the script exits with code 0.

### 4.3 Negative Boundary Test Execution
Negative test cases specify their expected error substring in a header directive:
```meme
// EXPECT_ERROR: Confuse kar diya
```
The test runner executes `./meme <negative_test.meme>`, asserts that the process exits with a non-zero exit code (`returncode != 0`), and verifies that standard error / standard output contains the expected diagnostic string.

---

## 5. Test Runner CLI Reference

The automated test runner is available via Python (`run_e2e_tests.py`) and an executable Bash wrapper (`run_e2e_tests.sh`).

### Commands

| Objective | Command |
| :--- | :--- |
| **Run Full E2E Test Suite** | `./tests/e2e/run_e2e_tests.sh` (or `python3 tests/e2e/run_e2e_tests.py`) |
| **List Test Catalog** | `python3 tests/e2e/run_e2e_tests.py --list` |
| **Run Specific Tier** | `python3 tests/e2e/run_e2e_tests.py --tier 1` (or `2`, `3`, `4`) |
| **Run Negative Tests Only** | `python3 tests/e2e/run_e2e_tests.py --negative-only` |
| **Verbose Output** | `python3 tests/e2e/run_e2e_tests.py -v` |
| **Custom Binary Path** | `python3 tests/e2e/run_e2e_tests.py --meme /path/to/meme` |

---

## 6. Strict Integrity & Invariant Constraints

1. **Zero Forbidden Words**:
   - The forbidden word must NEVER appear in test code, comments, string literals, filenames, or outputs.
   - The test runner enforces an automated pre-flight regex audit across all test files before execution.
2. **No Source Modifications**:
   - The E2E test track strictly modifies only test files (`tests/e2e/`) and documentation (`TEST_INFRA.md`, `TEST_READY.md`). No modifications to `src/` are permitted.
3. **Regression Safety**:
   - The existing 25 core test suites in `src/tests/test_suite.cpp` remain completely intact and executable via `make test`.
4. **Memory Footprint**:
   - Pointer and borrow designs are compatible with `static_assert(sizeof(Value) == 32)`.
