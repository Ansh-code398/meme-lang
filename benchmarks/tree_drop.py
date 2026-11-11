# Benchmark: Memory Allocation & Drop in Python
import time

class TreeNode:
    def __init__(self, val, left, right):
        self.val = val
        self.left = left
        self.right = right

def build_tree(depth):
    if depth <= 0:
        return None
    left = build_tree(depth - 1)
    right = build_tree(depth - 1)
    return TreeNode(1, left, right)

def sum_tree(node):
    if node is None:
        return 0
    return node.val + sum_tree(node.left) + sum_tree(node.right)

def bench_tree_drop(depth):
    root = build_tree(depth)
    total = sum_tree(root)
    del root
    return total

t0 = time.perf_counter()
res = bench_tree_drop(14)
elapsed = (time.perf_counter() - t0) * 1000.0
print(f"Python3 | Tree Drop (depth 14) | Result: {res} | Time: {elapsed:.2f}ms")
