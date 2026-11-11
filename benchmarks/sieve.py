# Benchmark: Sieve of Eratosthenes in Python
import time

def sieve(n):
    is_prime = [True] * (n + 1)
    is_prime[0] = False
    is_prime[1] = False
    p = 2
    while p * p <= n:
        if is_prime[p]:
            m = p * p
            while m <= n:
                is_prime[m] = False
                m += p
        p += 1
    return sum(1 for prime in is_prime if prime)

t0 = time.perf_counter()
total = sieve(100000)
elapsed = (time.perf_counter() - t0) * 1000.0
print(f"Python3 | Primes up to 100000: {total} | Time: {elapsed:.2f}ms")
