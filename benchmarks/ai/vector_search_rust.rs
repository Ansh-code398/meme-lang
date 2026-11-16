// Rust AI Benchmark: Vector Embeddings & Semantic Search (Cosine Similarity)
// Dataset: 5,000 Document Vectors (Dim = 64), 20 Queries

use std::time::Instant;

fn cosine_similarity(v1: &[f64], v2: &[f64]) -> f64 {
    let mut dot = 0.0;
    let mut norm1 = 0.0;
    let mut norm2 = 0.0;
    for i in 0..v1.len() {
        dot += v1[i] * v2[i];
        norm1 += v1[i] * v1[i];
        norm2 += v2[i] * v2[i];
    }
    let denom = (norm1 * norm2).sqrt();
    if denom == 0.0 { 0.0 } else { dot / denom }
}

fn main() {
    println!("==========================================================");
    println!("🦀 Rust Vector Search Benchmark (opt-level=3)");
    println!("==========================================================");

    let num_docs = 5000;
    let d = 64;
    let num_queries = 20;

    let mut seed: u64 = 543216789;
    let mut docs = Vec::with_capacity(num_docs);
    for _ in 0..num_docs {
        let mut vec = Vec::with_capacity(d);
        for _ in 0..d {
            seed = (seed.wrapping_mul(1103515245).wrapping_add(12345)) % 2147483648;
            vec.push(((seed % 2000) as f64 - 1000.0) / 1000.0);
        }
        docs.push(vec);
    }

    let mut queries = Vec::with_capacity(num_queries);
    for _ in 0..num_queries {
        let mut q_vec = Vec::with_capacity(d);
        for _ in 0..d {
            seed = (seed.wrapping_mul(1103515245).wrapping_add(12345)) % 2147483648;
            q_vec.push(((seed % 2000) as f64 - 1000.0) / 1000.0);
        }
        queries.push(q_vec);
    }

    let start = Instant::now();

    let mut best_score = -1.0;
    let mut best_match_idx = 0;

    for query in &queries {
        best_score = -1.0;
        best_match_idx = 0;
        for (doc_idx, doc) in docs.iter().enumerate() {
            let sim = cosine_similarity(query, doc);
            if sim > best_score {
                best_score = sim;
                best_match_idx = doc_idx;
            }
        }
    }

    let elapsed = start.elapsed().as_secs_f64();
    let total_evals = num_queries * num_docs;
    let qps = (num_queries as f64) / elapsed;
    let evals_per_sec = (total_evals as f64) / elapsed;

    println!("Database Size (vectors): {}, Dim: {}", num_docs, d);
    println!("Queries Executed: {}, Total Evaluations: {}", num_queries, total_evals);
    println!("Best Score in Last Query: {:.5}, Best Doc Index: {}", best_score, best_match_idx);
    println!("Elapsed Time (seconds): {:.6}", elapsed);
    println!("Query Throughput (QPS): {:.2}", qps);
    println!("Vector Throughput (evals/sec): {:.2}", evals_per_sec);
}
