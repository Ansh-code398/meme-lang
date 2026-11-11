// Benchmark: Recursive Fibonacci in Node.js
function fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

const t0 = performance.now();
const result = fib(20);
const elapsed = performance.now() - t0;
console.log(`Node.js | fib(20) = ${result} | Time: ${elapsed.toFixed(2)}ms`);
