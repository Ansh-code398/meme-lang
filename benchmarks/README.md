# ⚡ MEMElang Performance Benchmarks

Comprehensive real-world performance benchmarks comparing **MEMElang** against **Python 3.14**, **Node.js v26**, and **Bun v1.4**.

## 📊 Results Summary

All benchmarks run on Apple Silicon macOS with real-world datasets across 3 statistical runs (warmups discarded).

### Execution Time (in Milliseconds - Lower is Faster)

| Benchmark Task | MEMElang | Python 3.14 | Node.js v26 | Bun v1.4 |
| :--- | :--- | :--- | :--- | :--- |
| **JSON Analytics** (50,000 txns, 6.1 MB) | **408.7 ms** | 52.7 ms | 38.4 ms | 18.6 ms |
| **CSV Processing** (100,000 rows, 4.3 MB) | **1,259.6 ms** | 96.5 ms | 54.5 ms | 26.4 ms |
| **Crypto & Token Signing** (10,000 SHA-256 + HMAC) | **151.3 ms** | 49.0 ms | 54.1 ms | 25.2 ms |
| **Numerical Loop** (200,000 modular iterations) | **703.7 ms** | 18.3 ms | 30.8 ms | 6.4 ms |

---

### HTTP Server Throughput (1,000 Real HTTP Requests)

| Server Engine | Short-Lived Handshake | Average Latency | Keep-Alive Throughput |
| :--- | :--- | :--- | :--- |
| **Bun v1.4 (`Bun.serve`)** | **7,970.7 req/s** | 0.13 ms | ~18,200 req/s |
| **MEMElang (`express / elysia`)** | **7,886.1 req/s** | **0.13 ms** | **17,520.8 req/s** 🚀 |
| **Node.js v26 (`http.createServer`)** | **6,690.8 req/s** | 0.15 ms | 17,102.9 req/s |
| **Python 3.14 (`http.server`)** | **6,394.8 req/s** | 0.16 ms | 6,525.0 req/s |

---

### Process Memory Footprint (RAM / RSS)

| Runtime | Idle RAM (RSS) | Efficiency vs Node.js |
| :--- | :--- | :--- |
| **MEMElang** | **7.7 MB** | **6.1x lighter than Node.js** 🏆 |
| **Bun v1.4** | **10.2 MB** | 4.6x lighter |
| **Python 3.14** | **13.9 MB** | 3.4x lighter |
| **Node.js v26** | **47.3 MB** | Baseline |

---

## 🏃 Running the Benchmarks

```bash
# 1. Run all data benchmarks
python3 benchmarks/run_benchmarks.py

# 2. Run live HTTP server benchmark
python3 benchmarks/bench_http.py

# 3. Run pure MEMElang DataFrame benchmark
./meme benchmarks/bench_dataframe.meme
```
