// Rust AI Benchmark: Transformer Scaled Dot-Product Attention
// Architecture: N = 64, D = 64, 100 passes

use std::time::Instant;

fn main() {
    println!("==========================================================");
    println!("🦀 Rust Transformer Attention Benchmark (opt-level=3)");
    println!("==========================================================");

    const N: usize = 64;
    const D: usize = 64;
    let scale = 1.0 / (D as f64).sqrt();
    let num_passes = 100;

    let mut seed: u64 = 987654321;
    let mut q = vec![vec![0.0f64; D]; N];
    let mut k = vec![vec![0.0f64; D]; N];
    let mut v = vec![vec![0.0f64; D]; N];

    for r in 0..N {
        for c in 0..D {
            seed = (seed.wrapping_mul(1103515245).wrapping_add(12345)) % 2147483648;
            q[r][c] = ((seed % 2000) as f64 - 1000.0) / 1000.0;
            seed = (seed.wrapping_mul(1103515245).wrapping_add(12345)) % 2147483648;
            k[r][c] = ((seed % 2000) as f64 - 1000.0) / 1000.0;
            seed = (seed.wrapping_mul(1103515245).wrapping_add(12345)) % 2147483648;
            v[r][c] = ((seed % 2000) as f64 - 1000.0) / 1000.0;
        }
    }

    let mut s = vec![vec![0.0f64; N]; N];
    let mut out = vec![vec![0.0f64; D]; N];

    let start = Instant::now();

    for _ in 0..num_passes {
        for i in 0..N {
            // Q * K^T
            for j in 0..N {
                let mut dot = 0.0;
                for idx in 0..D {
                    dot += q[i][idx] * k[j][idx];
                }
                s[i][j] = dot * scale;
            }

            // Softmax
            let mut max_val = s[i][0];
            for j in 1..N {
                if s[i][j] > max_val {
                    max_val = s[i][j];
                }
            }

            let mut sum_exp = 0.0;
            for j in 0..N {
                s[i][j] = (s[i][j] - max_val).exp();
                sum_exp += s[i][j];
            }

            for j in 0..N {
                s[i][j] /= sum_exp;
            }

            // S * V
            for d in 0..D {
                let mut out_val = 0.0;
                for v_idx in 0..N {
                    out_val += s[i][v_idx] * v[v_idx][d];
                }
                out[i][d] = out_val;
            }
        }
    }

    let elapsed = start.elapsed().as_secs_f64();

    let mut checksum = 0.0;
    for r in 0..N {
        for c in 0..D {
            checksum += out[r][c];
        }
    }

    let total_flops = (num_passes as f64) * (2.0 * (N as f64) * (N as f64) * (D as f64) + (N as f64) * (N as f64) * 3.0 + 2.0 * (N as f64) * (N as f64) * (D as f64));
    let gflops = (total_flops / elapsed) / 1e9;
    let pps = (num_passes as f64) / elapsed;

    println!("Attention Passes: {}", num_passes);
    println!("Sequence Length N: {}, Head Dimension D: {}", N, D);
    println!("Output Checksum: {:.4}", checksum);
    println!("Elapsed Time (seconds): {:.6}", elapsed);
    println!("Throughput (passes/sec): {:.2}", pps);
    println!("Performance (GFLOPs): {:.4}", gflops);
}
