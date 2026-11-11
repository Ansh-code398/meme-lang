// Benchmark: In-Place Quicksort in Rust
use std::time::Instant;

fn partition(arr: &mut [i64], low: usize, high: usize) -> usize {
    let pivot = arr[high];
    let mut i = low;
    for j in low..high {
        if arr[j] <= pivot {
            arr.swap(i, j);
            i += 1;
        }
    }
    arr.swap(i, high);
    i
}

fn sort(arr: &mut [i64], low: usize, high: usize) {
    if low < high {
        let pi = partition(arr, low, high);
        if pi > 0 {
            sort(arr, low, pi - 1);
        }
        sort(arr, pi + 1, high);
    }
}

fn main() {
    let n = 5000;
    let mut arr = Vec::with_capacity(n);
    let mut seed: i64 = 42;
    let m: i64 = 2147483648;
    for _ in 0..n {
        seed = (seed * 1103515245 + 12345) % m;
        arr.push(seed % 1000000);
    }

    let start = Instant::now();
    sort(std::hint::black_box(&mut arr), 0, n - 1);
    let elapsed = start.elapsed().as_secs_f64() * 1000.0;

    let mut chk: i64 = 0;
    let chk_mod: i64 = 1000000007;
    for (idx, &val) in arr.iter().enumerate() {
        chk = (chk + val * (idx as i64 + 1)) % chk_mod;
    }

    println!("Rust | Quicksort N={} | Checksum: {} | Time: {:.5}ms", n, chk, elapsed);
}
