#!/usr/bin/env python3
"""
Python AI Benchmark: Vector Embeddings & Semantic Search (Cosine Similarity)
Dataset: 5,000 Document Vectors (Dim = 64), 20 Queries
Compares:
  1. Pure Python loops
  2. NumPy matrix multiplication
"""

import time
import math
import numpy as np

def run_pure_python():
    print("==========================================================")
    print("🐍 Pure Python Vector Search Benchmark")
    print("==========================================================")

    num_docs = 5000
    D = 64
    num_queries = 20

    docs = []
    seed = 543216789
    for _ in range(num_docs):
        vec = []
        for _ in range(D):
            seed = (seed * 1103515245 + 12345) % 2147483648
            vec.append(((seed % 2000) - 1000) / 1000.0)
        docs.append(vec)

    queries = []
    for _ in range(num_queries):
        q_vec = []
        for _ in range(D):
            seed = (seed * 1103515245 + 12345) % 2147483648
            q_vec.append(((seed % 2000) - 1000) / 1000.0)
        queries.append(q_vec)

    start_time = time.perf_counter()

    best_score = -1.0
    best_match_idx = 0

    for query in queries:
        best_score = -1.0
        best_match_idx = 0
        for doc_idx, doc in enumerate(docs):
            dot = sum(query[i] * doc[i] for i in range(D))
            norm1 = sum(query[i] * query[i] for i in range(D))
            norm2 = sum(doc[i] * doc[i] for i in range(D))
            denom = math.sqrt(norm1) * math.sqrt(norm2)
            sim = dot / denom if denom > 0 else 0.0
            if sim > best_score:
                best_score = sim
                best_match_idx = doc_idx

    elapsed = time.perf_counter() - start_time
    total_evals = num_queries * num_docs
    qps = num_queries / elapsed
    evals_per_sec = total_evals / elapsed

    print(f"Database Size (vectors): {num_docs}, Dim: {D}")
    print(f"Queries Executed: {num_queries}, Total Evaluations: {total_evals}")
    print(f"Best Score in Last Query: {best_score:.5f}, Best Doc Index: {best_match_idx}")
    print(f"Elapsed Time (seconds): {elapsed:.6f}")
    print(f"Query Throughput (QPS): {qps:.2f}")
    print(f"Vector Throughput (evals/sec): {evals_per_sec:.2f}")
    return elapsed


def run_numpy():
    print("\n==========================================================")
    print("🔢 NumPy Matrix Vector Search Benchmark")
    print("==========================================================")

    num_docs = 5000
    D = 64
    num_queries = 20

    seed = 543216789
    docs_raw = []
    for _ in range(num_docs):
        vec = []
        for _ in range(D):
            seed = (seed * 1103515245 + 12345) % 2147483648
            vec.append(((seed % 2000) - 1000) / 1000.0)
        docs_raw.append(vec)

    queries_raw = []
    for _ in range(num_queries):
        q_vec = []
        for _ in range(D):
            seed = (seed * 1103515245 + 12345) % 2147483648
            q_vec.append(((seed % 2000) - 1000) / 1000.0)
        queries_raw.append(q_vec)

    docs = np.array(docs_raw, dtype=np.float64)
    queries = np.array(queries_raw, dtype=np.float64)

    start_time = time.perf_counter()

    # Pre-normalize docs
    docs_norm = docs / np.linalg.norm(docs, axis=1, keepdims=True)
    queries_norm = queries / np.linalg.norm(queries, axis=1, keepdims=True)

    # All-to-all similarity matrix (20, 5000)
    sim_matrix = np.dot(queries_norm, docs_norm.T)
    best_indices = np.argmax(sim_matrix, axis=1)
    best_scores = np.max(sim_matrix, axis=1)

    elapsed = time.perf_counter() - start_time
    total_evals = num_queries * num_docs
    qps = num_queries / elapsed
    evals_per_sec = total_evals / elapsed

    print(f"Database Size (vectors): {num_docs}, Dim: {D}")
    print(f"Queries Executed: {num_queries}, Total Evaluations: {total_evals}")
    print(f"Best Score in Last Query: {best_scores[-1]:.5f}, Best Doc Index: {best_indices[-1]}")
    print(f"Elapsed Time (seconds): {elapsed:.6f}")
    print(f"Query Throughput (QPS): {qps:.2f}")
    print(f"Vector Throughput (evals/sec): {evals_per_sec:.2f}")
    return elapsed


if __name__ == "__main__":
    t_py = run_pure_python()
    t_np = run_numpy()
