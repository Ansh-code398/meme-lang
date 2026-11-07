// Benchmark: Memory Allocation & Drop in Rust
use std::time::Instant;

struct TreeNode {
    val: i64,
    left: Option<Box<TreeNode>>,
    right: Option<Box<TreeNode>>,
}

impl TreeNode {
    fn new(val: i64, left: Option<Box<TreeNode>>, right: Option<Box<TreeNode>>) -> Self {
        TreeNode { val, left, right }
    }
}

fn build_tree(depth: i64) -> Option<Box<TreeNode>> {
    if depth <= 0 {
        return None;
    }
    let left = build_tree(depth - 1);
    let right = build_tree(depth - 1);
    Some(Box::new(TreeNode::new(1, left, right)))
}

fn sum_tree(node: &Option<Box<TreeNode>>) -> i64 {
    match node {
        None => 0,
        Some(n) => n.val + sum_tree(&n.left) + sum_tree(&n.right),
    }
}

fn bench_tree_drop(depth: i64) -> i64 {
    let total;
    {
        let root = build_tree(depth);
        total = sum_tree(&root);
    }
    total
}

fn main() {
    let start = Instant::now();
    let result = bench_tree_drop(std::hint::black_box(14));
    let elapsed = start.elapsed().as_secs_f64() * 1000.0;

    println!("Rust | Tree Drop (depth 14) | Result: {} | Time: {:.5}ms", result, elapsed);
}
