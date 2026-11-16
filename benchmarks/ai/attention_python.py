#!/usr/bin/env python3
"""
Python AI Benchmark: Transformer Scaled Dot-Product Attention
Architecture: N = 64, D = 64, 100 passes
Compares:
  1. Pure Python nested loops (matching MEMElang baseline)
  2. NumPy vectorized tensor operations
"""

import time
import math
import numpy as np

def run_pure_python():
    print("==========================================================")
    print("🐍 Pure Python Transformer Attention Benchmark")
    print("==========================================================")

    N = 64
    D = 64
    scale = 1.0 / math.sqrt(D)
    num_passes = 100

    seed = 987654321
    Q = []
    K = []
    V = []
    for _ in range(N):
        q_row, k_row, v_row = [], [], []
        for _ in range(D):
            seed = (seed * 1103515245 + 12345) % 2147483648
            q_row.append(((seed % 2000) - 1000) / 1000.0)
            seed = (seed * 1103515245 + 12345) % 2147483648
            k_row.append(((seed % 2000) - 1000) / 1000.0)
            seed = (seed * 1103515245 + 12345) % 2147483648
            v_row.append(((seed % 2000) - 1000) / 1000.0)
        Q.append(q_row)
        K.append(k_row)
        V.append(v_row)

    S = [[0.0] * N for _ in range(N)]
    Out = [[0.0] * D for _ in range(N)]

    start_time = time.perf_counter()

    for _ in range(num_passes):
        for i in range(N):
            # Q * K^T
            for j in range(N):
                dot = sum(Q[i][k] * K[j][k] for k in range(D))
                S[i][j] = dot * scale

            # Softmax
            max_val = max(S[i])
            exp_row = [math.exp(v - max_val) for v in S[i]]
            sum_exp = sum(exp_row)
            S[i] = [v / sum_exp for v in exp_row]

            # S * V
            for d in range(D):
                Out[i][d] = sum(S[i][v_idx] * V[v_idx][d] for v_idx in range(N))

    elapsed = time.perf_counter() - start_time
    checksum = sum(sum(row) for row in Out)

    total_flops = num_passes * (2.0 * N * N * D + N * N * 3.0 + 2.0 * N * N * D)
    gflops = (total_flops / elapsed) / 1e9
    pps = num_passes / elapsed

    print(f"Attention Passes: {num_passes}")
    print(f"Sequence Length N: {N}, Head Dimension D: {D}")
    print(f"Output Checksum: {checksum:.4f}")
    print(f"Elapsed Time (seconds): {elapsed:.6f}")
    print(f"Throughput (passes/sec): {pps:.2f}")
    print(f"Performance (GFLOPs): {gflops:.4f}")
    return elapsed


def run_numpy():
    print("\n==========================================================")
    print("🔢 NumPy Vectorized Transformer Attention Benchmark")
    print("==========================================================")

    N = 64
    D = 64
    scale = 1.0 / math.sqrt(D)
    num_passes = 100

    np.random.seed(42)
    Q = np.random.randn(N, D).astype(np.float64)
    K = np.random.randn(N, D).astype(np.float64)
    V = np.random.randn(N, D).astype(np.float64)

    start_time = time.perf_counter()

    for _ in range(num_passes):
        # Attention = softmax(Q @ K.T * scale) @ V
        scores = np.dot(Q, K.T) * scale
        scores -= np.max(scores, axis=-1, keepdims=True)
        weights = np.exp(scores)
        weights /= np.sum(weights, axis=-1, keepdims=True)
        out = np.dot(weights, V)

    elapsed = time.perf_counter() - start_time
    checksum = np.sum(out)

    total_flops = num_passes * (2.0 * N * N * D + N * N * 3.0 + 2.0 * N * N * D)
    gflops = (total_flops / elapsed) / 1e9
    pps = num_passes / elapsed

    print(f"Attention Passes: {num_passes}")
    print(f"Sequence Length N: {N}, Head Dimension D: {D}")
    print(f"Output Checksum: {checksum:.4f}")
    print(f"Elapsed Time (seconds): {elapsed:.6f}")
    print(f"Throughput (passes/sec): {pps:.2f}")
    print(f"Performance (GFLOPs): {gflops:.4f}")
    return elapsed


if __name__ == "__main__":
    t_py = run_pure_python()
    t_np = run_numpy()
