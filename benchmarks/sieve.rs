// Benchmark: Sieve of Eratosthenes in Rust
use std::time::Instant;

fn sieve(n: usize) -> usize {
    let mut is_prime = vec![true; n + 1];
    is_prime[0] = false;
    is_prime[1] = false;

    let mut p = 2;
    while p * p <= n {
        if is_prime[p] {
            let mut m = p * p;
            while m <= n {
                is_prime[m] = false;
                m += p;
            }
        }
        p += 1;
    }

    let mut count = 0;
    for &prime in &is_prime {
        if prime {
            count += 1;
        }
    }
    count
}

fn main() {
    let start = Instant::now();
    let total = sieve(std::hint::black_box(100000));
    let elapsed = start.elapsed().as_secs_f64() * 1000.0;

    println!("Rust | Primes up to 100000: {} | Time: {:.5}ms", total, elapsed);
}
