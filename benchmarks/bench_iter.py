# Benchmark 4: Numerical Iterative Computation in Python
import time

def bench_math_loop(n):
    a = 0
    b = 1
    i = 0
    MOD = 1000000007
    while i < n:
        c = (a + b) % MOD
        a = b
        b = c
        i += 1
    return a

start = time.perf_counter()
res = bench_math_loop(200000)
elapsed = (time.perf_counter() - start) * 1000.0

print(f"Python3  | Steps: 200000 | Result: {res} | Time: {elapsed:.2f}ms")
