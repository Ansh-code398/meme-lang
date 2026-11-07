// Benchmark: Modular Math Loop in Rust
use std::time::Instant;

fn bench_math_loop(n: i64) -> i64 {
    let mut a: i64 = 0;
    let mut b: i64 = 1;
    let mut i: i64 = 0;
    let m: i64 = 1000000007;
    while i < n {
        let c = (a * 3 + b / 2 + i) % m;
        a = b;
        b = c;
        i += 1;
    }
    a
}

fn main() {
    let start = Instant::now();
    let result = bench_math_loop(std::hint::black_box(200000));
    let elapsed = start.elapsed().as_secs_f64() * 1000.0;

    println!("Rust | Math Loop 200000 | Result: {} | Time: {:.5}ms", result, elapsed);
}
