// Benchmark: CSV Processing in Pure Rust (No external dependencies)
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::time::Instant;
use std::collections::HashMap;

fn main() {
    let start = Instant::now();
    let file = File::open("benchmarks/data/sales_records.csv").expect("CSV file not found");
    let reader = BufReader::new(file);

    let mut high_value_orders = 0i64;
    let mut total_net_revenue = 0.0f64;
    let mut regional_revenue: HashMap<String, f64> = HashMap::new();
    let mut row_count = 0i64;

    let mut lines = reader.lines();
    let _header = lines.next();

    for line_res in lines {
        let line = match line_res {
            Ok(l) => l,
            Err(_) => continue,
        };
        let line = line.trim();
        if line.is_empty() {
            continue;
        }
        row_count += 1;
        let parts: Vec<&str> = line.split(',').collect();
        if parts.len() < 7 {
            continue;
        }
        let reg = parts[2].to_string();
        let units: f64 = parts[4].parse().unwrap_or(0.0);
        let price: f64 = parts[5].parse().unwrap_or(0.0);
        let disc: f64 = parts[6].parse().unwrap_or(0.0);
        let net = units * price * (1.0 - disc);

        total_net_revenue += net;

        if net > 500.0 {
            high_value_orders += 1;
            *regional_revenue.entry(reg).or_insert(0.0) += net;
        }
    }

    let elapsed = start.elapsed().as_secs_f64() * 1000.0;
    println!("Rust | Rows: {} | HighValue: {} | NetRev: {} | Regions: {} | Time: {:.2}ms",
             row_count, high_value_orders, total_net_revenue as i64, regional_revenue.len(), elapsed);
}
