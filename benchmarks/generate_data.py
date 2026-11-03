import json
import csv
import random
import time

random.seed(42)

print("⏳ Generating real benchmark datasets...")

# 1. Transactions JSON (50,000 records)
categories = ["Electronics", "Fashion", "Home & Kitchen", "Books", "Beauty", "Sports"]
statuses = ["completed", "completed", "completed", "pending", "refunded"]

txns = []
for i in range(50000):
    txns.append({
        "id": f"txn_{100000 + i}",
        "user_id": random.randint(1000, 9999),
        "category": random.choice(categories),
        "amount": round(random.uniform(5.0, 500.0), 2),
        "status": random.choice(statuses),
        "timestamp": 1729000000 + i * 15
    })

with open("benchmarks/data/transactions.json", "w") as f:
    json.dump(txns, f)

print("  ✔ benchmarks/data/transactions.json created (50,000 records)")

# 2. Sales Records CSV (100,000 rows)
regions = ["North", "South", "East", "West", "Central"]
products = ["Laptop", "Smartphone", "Headphones", "Desk Chair", "Coffee Maker", "Backpack"]

with open("benchmarks/data/sales_records.csv", "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["order_id", "customer_id", "region", "product", "units_sold", "unit_price", "discount"])
    for i in range(100000):
        writer.writerow([
            1000000 + i,
            random.randint(10000, 99999),
            random.choice(regions),
            random.choice(products),
            random.randint(1, 20),
            round(random.uniform(15.0, 1200.0), 2),
            round(random.choice([0.0, 0.05, 0.10, 0.15, 0.20]), 2)
        ])

print("  ✔ benchmarks/data/sales_records.csv created (100,000 rows)")

# 3. Auth Payloads JSON (10,000 records)
payloads = []
roles = ["admin", "user", "editor", "analyst"]
for i in range(10000):
    payloads.append({
        "sub": f"user_{i}",
        "role": random.choice(roles),
        "dept": random.choice(["Engineering", "Sales", "Ops", "Finance"]),
        "nonce": f"nonce_{random.randint(1000000, 9999999)}"
    })

with open("benchmarks/data/auth_payloads.json", "w") as f:
    json.dump(payloads, f)

print("  ✔ benchmarks/data/auth_payloads.json created (10,000 records)")
print("✨ Datasets ready for benchmarking!")
