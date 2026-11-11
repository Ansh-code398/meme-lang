// Benchmark: Sieve of Eratosthenes in Node.js
function sieve(n) {
    const isPrime = new Uint8Array(n + 1);
    isPrime.fill(1);
    isPrime[0] = 0;
    isPrime[1] = 0;
    let p = 2;
    while (p * p <= n) {
        if (isPrime[p]) {
            let m = p * p;
            while (m <= n) {
                isPrime[m] = 0;
                m += p;
            }
        }
        p++;
    }
    let count = 0;
    for (let i = 0; i <= n; i++) {
        if (isPrime[i]) count++;
    }
    return count;
}

const t0 = performance.now();
const total = sieve(100000);
const elapsed = performance.now() - t0;
console.log(`Node.js | Primes up to 100000: ${total} | Time: ${elapsed.toFixed(2)}ms`);
