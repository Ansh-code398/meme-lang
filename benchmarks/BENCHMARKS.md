# 🚀 MEMElang Comprehensive Performance & Benchmark Report

**Date:** 2026-11-11T15:18:15.454797+00:00 | **Platform:** Darwin arm64

## 1. Compilation Speed (Throughput)
- **Overall AOT Compilation Throughput:** `19.5 LOC/sec`
- **Frontend AST Translation Throughput (-c):** `10220.5 LOC/sec`
- **Total Lines Analyzed:** `98 LOC` in `5.025s`

| Target | Lines of Code | Backend / Mode | Compilation Time (s) | Throughput (LOC/s) | Peak RSS (MB) |
|---|---|---|---|---|---|
| `small_fib` | 11 | C++ Native (`./meme compile`) | 1.942s | 5.7 | 145.6 MB |
| `small_fib` | 11 | C++ Frontend (`--parse-only`) | 0.0151s | 728.8 | 7.1 MB |
| `small_fib` | 11 | Self-Hosted Native (`./meme_stage2`) | 1.634s | 6.7 | 155.2 MB |
| `small_fib` | 11 | Self-Hosted AST Translation (`-c`) | 0.0071s | 1545.0 | 2.5 MB |
| `medium_sieve` | 29 | C++ Native (`./meme compile`) | 1.419s | 20.4 | 148.4 MB |
| `medium_sieve` | 29 | C++ Frontend (`--parse-only`) | 0.0172s | 1684.5 | 7.1 MB |
| `medium_sieve` | 29 | Self-Hosted Native (`./meme_stage2`) | 2.299s | 12.6 | 147.2 MB |
| `medium_sieve` | 29 | Self-Hosted AST Translation (`-c`) | 0.0104s | 2780.4 | 3.0 MB |
| `medium_quicksort` | 58 | C++ Native (`./meme compile`) | 1.664s | 34.9 | 162.1 MB |
| `medium_quicksort` | 58 | C++ Frontend (`--parse-only`) | 0.0130s | 4450.6 | 7.2 MB |
| `medium_quicksort` | 58 | Self-Hosted Native (`./meme_stage2`) | 1.652s | 35.1 | 156.9 MB |
| `medium_quicksort` | 58 | Self-Hosted AST Translation (`-c`) | 0.0119s | 4871.7 | 4.2 MB |
| `macro_compiler` | 1528 | C++ Native (`./meme compile`) | N/A (Failed) | N/A | N/A |
| `macro_compiler` | 1528 | C++ Frontend (`--parse-only`) | 0.0213s | 71837.6 | 9.9 MB |
| `macro_compiler` | 1528 | Self-Hosted Native (`./meme_stage2`) | 6.487s | 235.5 | 351.9 MB |
| `macro_compiler` | 1528 | Self-Hosted AST Translation (`-c`) | 0.1296s | 11786.8 | 61.4 MB |


## 2. Runtime Throughput & Execution Speed Matrix
| Workload | Interpreted (s) | C++ Native (s) | Self-Hosted Native (s) | Rust Baseline (s) | Node Baseline (s) | Python Baseline (s) | Speedup (C++ vs Interp) | vs Rust |
|---|---|---|---|---|---|---|---|---|
| `fib` | 2.9128s | 0.0071s | 0.0059s | 0.0052s | 0.0404s | 0.0199s | **411.9x** | 0.73x |
| `sieve` | 0.4829s | 0.0058s | 0.0059s | 0.0059s | 0.0404s | 0.0201s | **83.0x** | 1.01x |
| `math_loop` | 0.5183s | 0.0083s | 0.0065s | 0.0053s | 0.0399s | 0.0334s | **62.4x** | 0.64x |
| `tree_drop` | 7.6037s | 0.0075s | N/A | 0.0045s | 0.0379s | 0.0218s | **1009.4x** | 0.60x |
| `quicksort` | 1.6960s | 0.0131s | 0.0160s | 0.0124s | 0.0468s | 0.0288s | **129.1x** | 0.94x |
| `json` | 0.7186s | N/A | N/A | 0.0173s | 0.1172s | 0.1206s | **N/A** | N/A |
| `csv` | 1.8850s | N/A | N/A | 0.0406s | 0.1405s | 0.2709s | **N/A** | N/A |


## 3. Peak RSS Memory Usage Matrix
| Workload | Interpreted RSS | C++ Native RSS | Self-Hosted Native RSS | Rust RSS | Node RSS | Python RSS |
|---|---|---|---|---|---|---|
| `fib` | 8.3 MB | 1.4 MB | 1.5 MB | 1.6 MB | 51.4 MB | 10.6 MB |
| `sieve` | 11.4 MB | 1.6 MB | 1.6 MB | 1.7 MB | 56.0 MB | 11.5 MB |
| `math_loop` | 8.2 MB | 1.5 MB | 1.5 MB | 1.6 MB | 58.3 MB | 10.8 MB |
| `tree_drop` | 14.1 MB | 6.5 MB | N/A | 2.1 MB | 53.1 MB | 12.1 MB |
| `quicksort` | 9.0 MB | 4.0 MB | 4.0 MB | 1.6 MB | 58.8 MB | 11.2 MB |
| `json` | 98.8 MB | N/A | N/A | 7.8 MB | 93.8 MB | 51.5 MB |
| `csv` | 112.3 MB | N/A | N/A | 1.9 MB | 87.6 MB | 11.7 MB |

