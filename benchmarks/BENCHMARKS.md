# 🚀 MEMElang Comprehensive Performance & Benchmark Report

**Date:** 2026-11-11T14:05:56.916963+00:00 | **Platform:** Darwin arm64

## 1. Compilation Speed (Throughput)
- **Overall Throughput:** `28.4 LOC/sec`
- **Total Lines Analyzed:** `40 LOC` in `1.408s`

| Target | Lines of Code | Compilation Time (s) | Throughput (LOC/s) | Peak RSS (MB) |
|---|---|---|---|---|
| `small_fib` | 11 | 0.704 | 15.6 | 160.7 |
| `medium_sieve` | 29 | 0.704 | 41.2 | 160.7 |


## 2. Runtime Throughput & Execution Speed Matrix
| Workload | Interpreted (s) | C++ Native (s) | Self-Hosted Native (s) | Rust Baseline (s) | Speedup (Native vs Interp) |
|---|---|---|---|---|---|
| `fib` | 2.0719s | 0.0240s | 0.0210s | 0.0272s | **86.4x** |
| `sieve` | 0.4065s | 0.1185s | N/A | 0.1234s | **3.4x** |
| `math_loop` | 0.4471s | 0.1700s | 0.1391s | 0.1738s | **2.6x** |
| `tree_drop` | 6.4969s | 0.2302s | N/A | 0.1382s | **28.2x** |
| `json` | 0.3536s | N/A | N/A | N/A | **N/A** |
| `csv` | 1.0409s | N/A | N/A | N/A | **N/A** |


## 3. Peak RSS Memory Usage
| Workload | Interpreted RSS (MB) | C++ Native RSS (MB) | Self-Hosted RSS (MB) |
|---|---|---|---|
| `fib` | 166.1 MB | 166.1 MB | 166.1 MB |
| `sieve` | 166.1 MB | 166.1 MB | 0.0 MB |
| `math_loop` | 166.1 MB | 166.1 MB | 166.1 MB |
| `tree_drop` | 168.2 MB | 168.2 MB | 0.0 MB |
| `json` | 168.2 MB | 0.0 MB | 0.0 MB |
| `csv` | 168.2 MB | 0.0 MB | 0.0 MB |

