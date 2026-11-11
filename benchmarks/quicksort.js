// Benchmark: In-Place Quicksort in Node.js
class QuickSort {
    constructor(n) {
        this.arr = [];
        this.n = n;
        let seed = 42n;
        const MOD = 2147483648n;
        for (let k = 0; k < n; k++) {
            seed = (seed * 1103515245n + 12345n) % MOD;
            this.arr.push(Number(seed % 1000000n));
        }
    }
    partition(low, high) {
        const pivot = this.arr[high];
        let i = low - 1;
        for (let j = low; j < high; j++) {
            if (this.arr[j] <= pivot) {
                i++;
                const tmp = this.arr[i];
                this.arr[i] = this.arr[j];
                this.arr[j] = tmp;
            }
        }
        const tmp2 = this.arr[i + 1];
        this.arr[i + 1] = this.arr[high];
        this.arr[high] = tmp2;
        return i + 1;
    }
    sort(low, high) {
        if (low >= high) return 0;
        const pi = this.partition(low, high);
        this.sort(low, pi - 1);
        this.sort(pi + 1, high);
        return 0;
    }
    checksum() {
        let chk = 0n;
        const CHK_MOD = 1000000007n;
        for (let idx = 0; idx < this.n; idx++) {
            chk = (chk + BigInt(this.arr[idx]) * BigInt(idx + 1)) % CHK_MOD;
        }
        return Number(chk);
    }
}

const qs = new QuickSort(5000);
const t0 = performance.now();
qs.sort(0, 4999);
const t1 = performance.now();
console.log(`Node.js | Quicksort N=5000 | Checksum: ${qs.checksum()} | Time: ${(t1 - t0).toFixed(2)}ms`);
