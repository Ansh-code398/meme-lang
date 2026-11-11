# Benchmark: Modular Math Loop in Python
import time

def bench_math_loop(n):
    a = 0
    b = 1
    i = 0
    MOD = 1000000007
    while i < n:
        c = (a * 3 + b // 2 + i) % MOD
        a = b
        b = c
        i += 1
    return a

t0 = time.perf_counter()
result = bench_math_loop(200000)
elapsed = (time.perf_counter() - t0) * 1000.0
print(f"Python3 | Math Loop 200000 | Result: {result} | Time: {elapsed:.2f}ms")
