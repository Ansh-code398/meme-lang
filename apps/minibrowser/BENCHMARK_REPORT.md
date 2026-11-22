# 🌐 MiniBrowser Engine Benchmark Report
**Generated:** 2026-11-22 08:27:40  
**Platform:** darwin (arm64)  
**Rustc:** `rustc 1.97.1 (8bab26f4f 2026-07-14)`  
**Python:** `Python 3.14.6`  
**MEMElang:** Dual-Engine Native AOT Compiler & Bytecode VM  

## Executive Summary
This benchmark evaluates a complete **5-stage Web Browser Engine** implemented from scratch with **100% bit-exact parity** across:
1. **Pure Rust** (`apps/minibrowser/minibrowser.rs` compiled with `rustc -O`)
2. **MEMElang Native AOT** (`apps/minibrowser/minibrowser.meme` compiled with `./meme compile`)
3. **MEMElang Interpreter** (`apps/minibrowser/minibrowser.meme` executed with `./meme`)
4. **Python 3** (`apps/minibrowser/minibrowser.py` executed with CPython)

### Pipeline Stages Profiled:
- **Stage 1 (HTML Parser):** Lexical tag tokenization, attribute extraction, entity decoding, and hierarchical DOM tree building.
- **Stage 2 (CSS Parser):** Selector parsing (type, class, id), 3-tuple specificity calculation, declaration block parsing.
- **Stage 3 (Style Cascade):** Rule matching, specificity-ordered cascade, default User-Agent stylesheet application, computed style resolution.
- **Stage 4 (Layout & Reflow):** Box model tree reflow, word wrapping, content/margin/padding/border geometry computation (`x, y, w, h`).
- **Stage 5 (Canvas Rasterizer):** 2D character buffer rasterization, ASCII box-drawing border painter, and text placement.

## Small Workload: Component Card (10 DOM nodes, 6 CSS rules)

| Engine | Throughput (pages/sec) | Avg Pipeline Latency (ms) | Speedup vs Python | Speedup vs Interpreter | Parity with Rust | Peak RSS (MB) |
|---|---|---|---|---|---|---|
| **Pure Rust (rustc -O)** | **57,702.0** | **0.0173 ms** | 2.61x | 830.35x | 100.0% | 2.2 MB |
| **MEMElang Native AOT** | **16,376.8** | **0.0611 ms** | 0.74x | 235.67x | 28.4% | 2.6 MB |
| **MEMElang Interpreter** | **69.5** | **14.3903 ms** | 0.00x | 1.00x | 0.1% | 10.6 MB |
| **Python 3 Reference** | **22,134.2** | **0.0452 ms** | 1.00x | 318.52x | 38.4% | 13.6 MB |

### Detailed Stage-by-Stage Breakdown (Avg Latency per Page in Milliseconds):

| Engine | Stage 1: HTML Parse | Stage 2: CSS Parse | Stage 3: Style Cascade | Stage 4: Layout Reflow | Stage 5: Canvas Paint | Total Avg Pipeline |
|---|---|---|---|---|---|---|
| **Pure Rust (rustc -O)** | 0.00373 ms | 0.00351 ms | 0.00294 ms | 0.00126 ms | 0.00348 ms | **0.01733 ms** |
| **MEMElang Native AOT** | 0.01365 ms | 0.01525 ms | 0.00255 ms | 0.00437 ms | 0.02310 ms | **0.06106 ms** |
| **MEMElang Interpreter** | 2.01889 ms | 1.07548 ms | 9.49812 ms | 0.62894 ms | 1.15970 ms | **14.39031 ms** |
| **Python 3 Reference** | 0.01528 ms | 0.00826 ms | 0.00838 ms | 0.00353 ms | 0.00950 ms | **0.04518 ms** |


## Medium Workload: Blog Post (22 DOM nodes, 11 CSS rules)

| Engine | Throughput (pages/sec) | Avg Pipeline Latency (ms) | Speedup vs Python | Speedup vs Interpreter | Parity with Rust | Peak RSS (MB) |
|---|---|---|---|---|---|---|
| **Pure Rust (rustc -O)** | **68,183.8** | **0.0147 ms** | 6.88x | 3396.18x | 100.0% | 2.2 MB |
| **MEMElang Native AOT** | **13,614.6** | **0.0735 ms** | 1.37x | 678.13x | 20.0% | 2.8 MB |
| **MEMElang Interpreter** | **20.1** | **49.8092 ms** | 0.00x | 1.00x | 0.0% | 11.0 MB |
| **Python 3 Reference** | **9,916.9** | **0.1008 ms** | 1.00x | 493.95x | 14.5% | 13.4 MB |

### Detailed Stage-by-Stage Breakdown (Avg Latency per Page in Milliseconds):

| Engine | Stage 1: HTML Parse | Stage 2: CSS Parse | Stage 3: Style Cascade | Stage 4: Layout Reflow | Stage 5: Canvas Paint | Total Avg Pipeline |
|---|---|---|---|---|---|---|
| **Pure Rust (rustc -O)** | 0.00314 ms | 0.00271 ms | 0.00248 ms | 0.00166 ms | 0.00279 ms | **0.01467 ms** |
| **MEMElang Native AOT** | 0.01861 ms | 0.01474 ms | 0.00284 ms | 0.00829 ms | 0.02698 ms | **0.07345 ms** |
| **MEMElang Interpreter** | 5.71507 ms | 2.09872 ms | 37.55596 ms | 1.62485 ms | 2.80423 ms | **49.80920 ms** |
| **Python 3 Reference** | 0.03940 ms | 0.01330 ms | 0.01894 ms | 0.00905 ms | 0.01991 ms | **0.10084 ms** |


## Heavy Workload: Analytics Dashboard (75 DOM nodes, 15 CSS rules)

| Engine | Throughput (pages/sec) | Avg Pipeline Latency (ms) | Speedup vs Python | Speedup vs Interpreter | Parity with Rust | Peak RSS (MB) |
|---|---|---|---|---|---|---|
| **Pure Rust (rustc -O)** | **23,229.9** | **0.0430 ms** | 6.54x | 4683.04x | 100.0% | 2.4 MB |
| **MEMElang Native AOT** | **5,593.2** | **0.1788 ms** | 1.57x | 1127.56x | 24.1% | 3.1 MB |
| **MEMElang Interpreter** | **5.0** | **201.5954 ms** | 0.00x | 1.00x | 0.0% | 11.6 MB |
| **Python 3 Reference** | **3,553.6** | **0.2814 ms** | 1.00x | 716.39x | 15.3% | 13.7 MB |

### Detailed Stage-by-Stage Breakdown (Avg Latency per Page in Milliseconds):

| Engine | Stage 1: HTML Parse | Stage 2: CSS Parse | Stage 3: Style Cascade | Stage 4: Layout Reflow | Stage 5: Canvas Paint | Total Avg Pipeline |
|---|---|---|---|---|---|---|
| **Pure Rust (rustc -O)** | 0.00976 ms | 0.00418 ms | 0.00931 ms | 0.00448 ms | 0.00995 ms | **0.04305 ms** |
| **MEMElang Native AOT** | 0.04864 ms | 0.02078 ms | 0.00837 ms | 0.01877 ms | 0.07782 ms | **0.17879 ms** |
| **MEMElang Interpreter** | 14.73054 ms | 3.09304 ms | 170.51688 ms | 4.91395 ms | 8.32844 ms | **201.59536 ms** |
| **Python 3 Reference** | 0.10123 ms | 0.01967 ms | 0.06437 ms | 0.02474 ms | 0.07114 ms | **0.28141 ms** |


## Parity Verification Results
All 4 engines were verified using `test_minibrowser_parity.py` with bit-exact checks across:
- ✅ **DOM Trees:** Exact match of all elements, void tags, text nodes, IDs, and class lists.
- ✅ **Style Cascade:** Exact match of specificity calculations, inheritance, and computed properties.
- ✅ **Layout Box Coordinates:** Bit-exact coordinates `(x, y, width, height)` and wrapped line counts.
- ✅ **Canvas ASCII Framebuffers:** Identical 2D character-by-character grid rendering.

**Result: 27/27 Tests Passed (100% Complete Parity).**
