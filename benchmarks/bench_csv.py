# Benchmark 2: CSV Processing & Financial Calculations in Python
import csv
import time

start = time.perf_counter()

high_value_orders = 0
total_net_revenue = 0.0
regional_revenue = {}
row_count = 0

with open("benchmarks/data/sales_records.csv", "r", newline="") as f:
    reader = csv.DictReader(f)
    for r in reader:
        row_count += 1
        units = float(r["units_sold"])
        price = float(r["unit_price"])
        disc = float(r["discount"])
        net = units * price * (1.0 - disc)

        total_net_revenue += net

        if net > 500.0:
            high_value_orders += 1
            reg = r["region"]
            if reg not in regional_revenue:
                regional_revenue[reg] = 0.0
            regional_revenue[reg] += net

elapsed = (time.perf_counter() - start) * 1000.0
print(f"Python3  | Rows: {row_count} | HighValue: {high_value_orders} | NetRev: {int(total_net_revenue)} | Regions: {len(regional_revenue)} | Time: {elapsed:.2f}ms")
