# Benchmark 1: JSON Data Processing & Aggregation in Python
import json
import time

start = time.perf_counter()

with open("benchmarks/data/transactions.json", "r") as f:
    txns = json.load(f)

completed_count = 0
total_revenue = 0.0
category_totals = {}
category_counts = {}

for t in txns:
    if t["status"] == "completed":
        completed_count += 1
        amt = float(t["amount"])
        total_revenue += amt
        cat = t["category"]
        if cat not in category_totals:
            category_totals[cat] = 0.0
            category_counts[cat] = 0
        category_totals[cat] += amt
        category_counts[cat] += 1

elapsed = (time.perf_counter() - start) * 1000.0
print(f"Python3  | Completed: {completed_count} | Revenue: {int(total_revenue)} | Categories: {len(category_totals)} | Time: {elapsed:.2f}ms")
