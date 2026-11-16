#!/usr/bin/env python3
"""
Python AI Benchmark: Multi-Layer Perceptron (MLP) Neural Network
Architecture: 2 Inputs -> 4 Hidden Neurons (Tanh) -> 1 Output (Sigmoid)
Dataset: Non-linear XOR Problem (200 training samples)
Compares:
  1. Pure Python loop-by-loop implementation (matching MEMElang AST/loop semantics)
  2. NumPy vectorized implementation
"""

import time
import math
import numpy as np

def sigmoid(x):
    return 1.0 / (1.0 + math.exp(-x))

def run_pure_python():
    print("==========================================================")
    print("🐍 Pure Python Neural Network Training Benchmark")
    print("==========================================================")

    # Generate 200 XOR training samples with identical deterministic PRNG
    X = []
    Y = []
    seed = 1234567

    for s_idx in range(200):
        seed = (seed * 1103515245 + 12345) % 2147483648
        r1 = (seed % 1000) / 1000.0
        seed = (seed * 1103515245 + 12345) % 2147483648
        r2 = (seed % 1000) / 1000.0

        x1 = 0.0 if (s_idx % 2 == 0) else 1.0
        x2 = 0.0 if ((s_idx // 2) % 2 == 0) else 1.0
        y = 1.0 if (x1 != x2) else 0.0

        x1 += (r1 * 0.1 - 0.05)
        x2 += (r2 * 0.1 - 0.05)
        X.append([x1, x2])
        Y.append(y)

    W1 = [
        [0.15, 0.25, 0.35, 0.45],
        [0.55, 0.65, 0.75, 0.85]
    ]
    b1 = [0.1, 0.1, 0.1, 0.1]
    W2 = [0.2, 0.4, 0.6, 0.8]
    b2 = 0.5

    learning_rate = 0.25
    epochs = 1000
    num_samples = 200

    start_time = time.perf_counter()
    last_loss = 0.0

    for ep in range(epochs):
        total_loss = 0.0
        for i in range(num_samples):
            x1, x2 = X[i][0], X[i][1]
            y_true = Y[i]

            # Forward
            h0 = math.tanh(x1 * W1[0][0] + x2 * W1[1][0] + b1[0])
            h1 = math.tanh(x1 * W1[0][1] + x2 * W1[1][1] + b1[1])
            h2 = math.tanh(x1 * W1[0][2] + x2 * W1[1][2] + b1[2])
            h3 = math.tanh(x1 * W1[0][3] + x2 * W1[1][3] + b1[3])

            z2 = h0 * W2[0] + h1 * W2[1] + h2 * W2[2] + h3 * W2[3] + b2
            y_pred = sigmoid(z2)

            err = y_pred - y_true
            total_loss += err * err

            # Backward
            d_out = err * y_pred * (1.0 - y_pred)
            d_h0 = d_out * W2[0] * (1.0 - h0 * h0)
            d_h1 = d_out * W2[1] * (1.0 - h1 * h1)
            d_h2 = d_out * W2[2] * (1.0 - h2 * h2)
            d_h3 = d_out * W2[3] * (1.0 - h3 * h3)

            W2[0] -= learning_rate * d_out * h0
            W2[1] -= learning_rate * d_out * h1
            W2[2] -= learning_rate * d_out * h2
            W2[3] -= learning_rate * d_out * h3
            b2 -= learning_rate * d_out

            W1[0][0] -= learning_rate * d_h0 * x1
            W1[1][0] -= learning_rate * d_h0 * x2
            b1[0] -= learning_rate * d_h0

            W1[0][1] -= learning_rate * d_h1 * x1
            W1[1][1] -= learning_rate * d_h1 * x2
            b1[1] -= learning_rate * d_h1

            W1[0][2] -= learning_rate * d_h2 * x1
            W1[1][2] -= learning_rate * d_h2 * x2
            b1[2] -= learning_rate * d_h2

            W1[0][3] -= learning_rate * d_h3 * x1
            W1[1][3] -= learning_rate * d_h3 * x2
            b1[3] -= learning_rate * d_h3

        last_loss = total_loss / num_samples

    elapsed = time.perf_counter() - start_time

    correct = 0
    for i in range(num_samples):
        x1, x2 = X[i][0], X[i][1]
        h0 = math.tanh(x1 * W1[0][0] + x2 * W1[1][0] + b1[0])
        h1 = math.tanh(x1 * W1[0][1] + x2 * W1[1][1] + b1[1])
        h2 = math.tanh(x1 * W1[0][2] + x2 * W1[1][2] + b1[2])
        h3 = math.tanh(x1 * W1[0][3] + x2 * W1[1][3] + b1[3])
        y_pred = sigmoid(h0 * W2[0] + h1 * W2[1] + h2 * W2[2] + h3 * W2[3] + b2)
        label = 1.0 if y_pred >= 0.5 else 0.0
        if label == Y[i]:
            correct += 1

    acc = (correct * 100.0) / num_samples
    throughput = (epochs * num_samples) / elapsed

    print(f"Training Epochs: {epochs}")
    print(f"Total Samples: {num_samples}")
    print(f"Total Backprop Steps: {epochs * num_samples}")
    print(f"Final Mean Squared Loss: {last_loss:.6f}")
    print(f"Classification Accuracy (%): {acc:.1f}")
    print(f"Elapsed Time (seconds): {elapsed:.6f}")
    print(f"Throughput (samples/sec): {throughput:.2f}")
    return elapsed


def run_numpy():
    print("\n==========================================================")
    print("🔢 NumPy Vectorized Batch Neural Network Benchmark")
    print("==========================================================")

    # 200 samples
    seed = 1234567
    X_list = []
    Y_list = []
    for s_idx in range(200):
        seed = (seed * 1103515245 + 12345) % 2147483648
        r1 = (seed % 1000) / 1000.0
        seed = (seed * 1103515245 + 12345) % 2147483648
        r2 = (seed % 1000) / 1000.0
        x1 = 0.0 if (s_idx % 2 == 0) else 1.0
        x2 = 0.0 if ((s_idx // 2) % 2 == 0) else 1.0
        y = 1.0 if (x1 != x2) else 0.0
        x1 += (r1 * 0.1 - 0.05)
        x2 += (r2 * 0.1 - 0.05)
        X_list.append([x1, x2])
        Y_list.append([y])

    X = np.array(X_list, dtype=np.float64) # (200, 2)
    Y = np.array(Y_list, dtype=np.float64) # (200, 1)

    W1 = np.array([
        [0.15, 0.25, 0.35, 0.45],
        [0.55, 0.65, 0.75, 0.85]
    ], dtype=np.float64) # (2, 4)
    b1 = np.array([[0.1, 0.1, 0.1, 0.1]], dtype=np.float64) # (1, 4)

    W2 = np.array([[0.2], [0.4], [0.6], [0.8]], dtype=np.float64) # (4, 1)
    b2 = np.array([[0.5]], dtype=np.float64) # (1, 1)

    lr = 0.25
    epochs = 1000
    N = 200

    start_time = time.perf_counter()
    for _ in range(epochs):
        # Forward
        H = np.tanh(np.dot(X, W1) + b1) # (200, 4)
        Z2 = np.dot(H, W2) + b2 # (200, 1)
        Y_pred = 1.0 / (1.0 + np.exp(-Z2)) # (200, 1)

        err = Y_pred - Y # (200, 1)
        # Backward
        d_out = err * Y_pred * (1.0 - Y_pred) # (200, 1)
        d_H = np.dot(d_out, W2.T) * (1.0 - H * H) # (200, 4)

        W2 -= lr * np.dot(H.T, d_out)
        b2 -= lr * np.sum(d_out, axis=0, keepdims=True)
        W1 -= lr * np.dot(X.T, d_H)
        b1 -= lr * np.sum(d_H, axis=0, keepdims=True)

    elapsed = time.perf_counter() - start_time

    # Final eval
    H = np.tanh(np.dot(X, W1) + b1)
    Y_pred = 1.0 / (1.0 + np.exp(-(np.dot(H, W2) + b2)))
    loss = np.mean((Y_pred - Y) ** 2)
    acc = np.mean((Y_pred >= 0.5) == (Y >= 0.5)) * 100.0
    throughput = (epochs * N) / elapsed

    print(f"Training Epochs: {epochs}")
    print(f"Total Samples: {N}")
    print(f"Total Backprop Steps: {epochs * N}")
    print(f"Final Mean Squared Loss: {loss:.6f}")
    print(f"Classification Accuracy (%): {acc:.1f}")
    print(f"Elapsed Time (seconds): {elapsed:.6f}")
    print(f"Throughput (samples/sec): {throughput:.2f}")
    return elapsed


if __name__ == "__main__":
    t_py = run_pure_python()
    t_np = run_numpy()
