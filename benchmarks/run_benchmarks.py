import subprocess
import time
import statistics
import sys
import json
import os

# Auto-generate datasets if missing
if not os.path.exists("benchmarks/data/transactions.json") or not os.path.exists("benchmarks/data/sales_records.csv"):
    print("⏳ Generating benchmark datasets on the fly...")
    subprocess.run([sys.executable, "benchmarks/generate_data.py"], check=True)

RUNS = 3  # Number of iterations for statistical stability

BENCHMARKS = [
    {
        "name": "JSON Data Analytics (50,000 txns, 6.1 MB)",
        "commands": {
            "MEMElang": ["./meme", "benchmarks/bench_json.meme"],
            "Python 3.14": ["python3", "benchmarks/bench_json.py"],
            "Node.js v26": ["node", "benchmarks/bench_json.js"],
            "Bun v1.4": ["bun", "benchmarks/bench_json.js"]
        }
    },
    {
        "name": "CSV Financial Processing (100,000 rows, 4.3 MB)",
        "commands": {
            "MEMElang": ["./meme", "benchmarks/bench_csv.meme"],
            "Python 3.14": ["python3", "benchmarks/bench_csv.py"],
            "Node.js v26": ["node", "benchmarks/bench_csv.js"],
            "Bun v1.4": ["bun", "benchmarks/bench_csv.js"]
        }
    },
    {
        "name": "Crypto & Token Signing (10,000 SHA-256 + HMAC)",
        "commands": {
            "MEMElang": ["./meme", "benchmarks/bench_crypto.meme"],
            "Python 3.14": ["python3", "benchmarks/bench_crypto.py"],
            "Node.js v26": ["node", "benchmarks/bench_crypto.js"],
            "Bun v1.4": ["bun", "benchmarks/bench_crypto.js"]
        }
    },
    {
        "name": "Numerical Loop Execution (200,000 modular iterations)",
        "commands": {
            "MEMElang": ["./meme", "benchmarks/bench_iter.meme"],
            "Python 3.14": ["python3", "benchmarks/bench_iter.py"],
            "Node.js v26": ["node", "benchmarks/bench_iter.js"],
            "Bun v1.4": ["bun", "benchmarks/bench_iter.js"]
        }
    }
]

print("=" * 80)
print("🚀 RUNNING MULTI-LANGUAGE REAL-DATA PERFORMANCE BENCHMARKS")
print(f"Iterations per benchmark: {RUNS} | Discarding warmup runs")
print("=" * 80)

results = {}

for bench in BENCHMARKS:
    bench_name = bench["name"]
    print(f"\n📊 Benchmark: {bench_name}")
    results[bench_name] = {}

    for engine, cmd in bench["commands"].items():
        durations = []
        # Warmup run
        try:
            subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
        except Exception as e:
            print(f"  ❌ Error running {engine}: {e}")
            continue

        for run_i in range(RUNS):
            t0 = time.perf_counter()
            proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            t1 = time.perf_counter()
            if proc.returncode != 0:
                print(f"  ❌ Error on {engine}: {proc.stderr}")
                break
            durations.append((t1 - t0) * 1000.0)

        if durations:
            avg_ms = statistics.mean(durations)
            med_ms = statistics.median(durations)
            min_ms = min(durations)
            max_ms = max(durations)
            stdev_ms = statistics.stdev(durations) if len(durations) > 1 else 0.0

            results[bench_name][engine] = {
                "mean_ms": round(avg_ms, 2),
                "median_ms": round(med_ms, 2),
                "min_ms": round(min_ms, 2),
                "max_ms": round(max_ms, 2),
                "stdev_ms": round(stdev_ms, 2)
            }
            print(f"  ⚡ {engine:<12}: {avg_ms:8.2f} ms (min: {min_ms:7.2f} ms, max: {max_ms:7.2f} ms)")

# Save results to JSON
with open("benchmarks/results.json", "w") as f:
    json.dump(results, f, indent=2)

print("\n" + "=" * 80)
print("🏆 BENCHMARK RESULTS SUMMARY TABLE (Execution Time in Milliseconds)")
print("=" * 80)
header = f"{'Benchmark Task':<48} | {'MEMElang':<10} | {'Python 3.14':<12} | {'Node.js':<10} | {'Bun':<8}"
print(header)
print("-" * len(header))

for b_name, b_data in results.items():
    meme_t = f"{b_data.get('MEMElang', {}).get('median_ms', 'N/A')} ms"
    py_t = f"{b_data.get('Python 3.14', {}).get('median_ms', 'N/A')} ms"
    node_t = f"{b_data.get('Node.js v26', {}).get('median_ms', 'N/A')} ms"
    bun_t = f"{b_data.get('Bun v1.4', {}).get('median_ms', 'N/A')} ms"
    print(f"{b_name:<48} | {meme_t:<10} | {py_t:<12} | {node_t:<10} | {bun_t:<8}")

print("=" * 80)
