// Benchmark: Modular Math Loop in Node.js
function bench_math_loop(n) {
    let a = 0n;
    let b = 1n;
    let i = 0n;
    const MOD = 1000000007n;
    const limit = BigInt(n);
    while (i < limit) {
        const c = (a * 3n + (b / 2n) + i) % MOD;
        a = b;
        b = c;
        i += 1n;
    }
    return Number(a);
}

const t0 = performance.now();
const result = bench_math_loop(200000);
const elapsed = performance.now() - t0;
console.log(`Node.js | Math Loop 200000 | Result: ${result} | Time: ${elapsed.toFixed(2)}ms`);
