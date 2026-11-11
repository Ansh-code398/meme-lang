// Benchmark: Memory Allocation & Drop in Node.js
class TreeNode {
    constructor(val, left, right) {
        this.val = val;
        this.left = left;
        this.right = right;
    }
}

function buildTree(depth) {
    if (depth <= 0) return null;
    const left = buildTree(depth - 1);
    const right = buildTree(depth - 1);
    return new TreeNode(1, left, right);
}

function sumTree(node) {
    if (node === null) return 0;
    return node.val + sumTree(node.left) + sumTree(node.right);
}

function benchTreeDrop(depth) {
    let root = buildTree(depth);
    const total = sumTree(root);
    root = null;
    return total;
}

const t0 = performance.now();
const res = benchTreeDrop(14);
const elapsed = performance.now() - t0;
console.log(`Node.js | Tree Drop (depth 14) | Result: ${res} | Time: ${elapsed.toFixed(2)}ms`);
