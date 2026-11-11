# Benchmark: In-Place Quicksort in Python
import time

class QuickSort:
    def __init__(self, n):
        self.arr = []
        self.n = n
        seed = 42
        MOD = 2147483648
        for _ in range(n):
            seed = (seed * 1103515245 + 12345) % MOD
            self.arr.append(seed % 1000000)

    def partition(self, low, high):
        pivot = self.arr[high]
        i = low - 1
        for j in range(low, high):
            if self.arr[j] <= pivot:
                i += 1
                self.arr[i], self.arr[j] = self.arr[j], self.arr[i]
        self.arr[i + 1], self.arr[high] = self.arr[high], self.arr[i + 1]
        return i + 1

    def sort(self, low, high):
        if low >= high:
            return 0
        pi = self.partition(low, high)
        self.sort(low, pi - 1)
        self.sort(pi + 1, high)
        return 0

    def checksum(self):
        chk = 0
        CHK_MOD = 1000000007
        for idx in range(self.n):
            chk = (chk + self.arr[idx] * (idx + 1)) % CHK_MOD
        return chk

qs = QuickSort(5000)
t0 = time.perf_counter()
qs.sort(0, 4999)
t1 = time.perf_counter()
print(f"Python3 | Quicksort N=5000 | Checksum: {qs.checksum()} | Time: {(t1 - t0) * 1000.0:.2f}ms")
