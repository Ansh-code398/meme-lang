# ⚡ MEMElang vs Rust Comparative Performance Benchmarks

*Generated on: 2026-11-07 06:00:18 UTC*

## 🖥️ System & Toolchain Environment

- **Operating System**: macOS-27.2-arm64-arm-64bit-Mach-O
- **Architecture**: arm64
- **Rust Toolchain**: `rustc 1.97.1 (8bab26f4f 2026-07-14)`
- **MEMElang Runtime**: `MEMElang v1.0`
- **Python Benchmark Runner**: `Python 3.14.7`

## 📊 Summary Results Table

| Benchmark Task | Target | Compile / Parse Speed | Runtime Execution (Mean) | Min / Max (ms) | CPU Time (ms) | Peak RSS Memory | Relative Speedup |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Recursive Fibonacci** | **Rust (native -O)** | 524.2 ms | **6.64 ms** | 5.1 / 9.0 ms | 6.7 ms | 1.8 MB | **529.63x faster** |
| | **MEMElang** | 9.9 ms | 3518.85 ms | 3135.2 / 4144.4 ms | 2943.3 ms | 8.4 MB | Baseline (1.0x) |
| **Sieve of Eratosthenes** | **Rust (native -O)** | 646.2 ms | **5.41 ms** | 4.3 / 6.0 ms | 3.3 ms | 1.9 MB | **79.53x faster** |
| | **MEMElang** | 10.8 ms | 430.63 ms | 424.6 / 442.6 ms | 393.3 ms | 11.4 MB | Baseline (1.0x) |
| **Modular Math Loop** | **Rust (native -O)** | 160.7 ms | **11.15 ms** | 9.4 / 12.1 ms | 6.7 ms | 1.8 MB | **38.90x faster** |
| | **MEMElang** | 10.3 ms | 433.83 ms | 334.2 / 524.1 ms | 353.3 ms | 8.2 MB | Baseline (1.0x) |
| **Tree Allocation & Drop** | **Rust (native -O)** | 177.9 ms | **5.14 ms** | 4.7 / 5.9 ms | 3.3 ms | 2.4 MB | **1811.26x faster** |
| | **MEMElang** | 9.8 ms | 9302.43 ms | 8590.7 / 9969.8 ms | 8450.0 ms | 14.3 MB | Baseline (1.0x) |

## 🔍 Architectural Analysis & Insights

1. **Compilation vs Interpretation Tradeoff**:
   - MEMElang provides near-zero startup delay (parsing in milliseconds) with rapid iteration feedback.
   - Rust's ahead-of-time LLVM optimizing compiler spends significantly more time upfront (70-350ms) to produce highly optimized machine code.
2. **Runtime Execution Dynamics**:
   - In tight recursive trees and arithmetic loops, native Rust leverages CPU registers, LLVM optimization passes, and loop unrolling.
   - MEMElang executes AST evaluation with dynamic type dispatch and runtime safety tracking.
3. **Memory Management & Footprint**:
   - MEMElang maintains a lightweight footprint (~8-15 MB RSS) while tracking reference counts and borrow scopes.
