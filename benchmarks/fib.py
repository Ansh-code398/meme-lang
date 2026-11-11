# Benchmark: Recursive Fibonacci in Python
import time

def fib(n):
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

t0 = time.perf_counter()
result = fib(20)
elapsed = (time.perf_counter() - t0) * 1000.0
print(f"Python3 | fib(20) = {result} | Time: {elapsed:.2f}ms")
