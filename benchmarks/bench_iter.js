// Benchmark 4: Numerical Iterative Computation in JavaScript (Node/Bun)
function benchMathLoop(n) {
    let a = 0;
    let b = 1;
    let i = 0;
    const MOD = 1000000007;
    while (i < n) {
        const c = (a + b) % MOD;
        a = b;
        b = c;
        i++;
    }
    return a;
}

const start = performance.now();
const res = benchMathLoop(200000);
const elapsed = performance.now() - start;

const runtime = typeof Bun !== 'undefined' ? 'Bun      ' : 'Node.js  ';
console.log(`${runtime}| Steps: 200000 | Result: ${res} | Time: ${elapsed.toFixed(2)}ms`);
