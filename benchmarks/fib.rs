// Benchmark: Recursive Fibonacci in Rust
use std::time::Instant;

fn fib(n: i64) -> i64 {
    if n <= 1 {
        return n;
    }
    fib(n - 1) + fib(n - 2)
}

fn main() {
    let start = Instant::now();
    let result = fib(std::hint::black_box(20));
    let elapsed = start.elapsed().as_secs_f64() * 1000.0;

    println!("Rust | fib(20) = {} | Time: {:.5}ms", result, elapsed);
}
