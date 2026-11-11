// Benchmark: JSON Processing in Pure Rust (No external dependencies)
use std::fs::File;
use std::io::Read;
use std::time::Instant;
use std::collections::HashMap;

fn main() {
    let start = Instant::now();
    let mut file = File::open("benchmarks/data/transactions.json").expect("JSON file not found");
    let mut content = String::new();
    file.read_to_string(&mut content).expect("read failed");

    let mut completed_count = 0i64;
    let mut total_revenue = 0.0f64;
    let mut category_totals: HashMap<String, f64> = HashMap::new();

    let bytes = content.as_bytes();
    let n = bytes.len();
    let mut i = 0;

    while i < n {
        if bytes[i] == b'{' {
            let mut status = "";
            let mut category = "";
            let mut amount = 0.0f64;

            i += 1;
            while i < n && bytes[i] != b'}' {
                if bytes[i] == b'"' {
                    let key_start = i + 1;
                    let mut key_end = key_start;
                    while key_end < n && bytes[key_end] != b'"' {
                        key_end += 1;
                    }
                    let key = &content[key_start..key_end];
                    i = key_end + 1;
                    while i < n && (bytes[i] == b':' || bytes[i] == b' ' || bytes[i] == b'\t' || bytes[i] == b'\n') {
                        i += 1;
                    }
                    if i < n && bytes[i] == b'"' {
                        let val_start = i + 1;
                        let mut val_end = val_start;
                        while val_end < n && bytes[val_end] != b'"' {
                            val_end += 1;
                        }
                        let val = &content[val_start..val_end];
                        if key == "status" {
                            status = val;
                        } else if key == "category" {
                            category = val;
                        }
                        i = val_end + 1;
                    } else {
                        let num_start = i;
                        while i < n && bytes[i] != b',' && bytes[i] != b'}' && bytes[i] != b' ' {
                            i += 1;
                        }
                        if key == "amount" {
                            let num_str = &content[num_start..i];
                            amount = num_str.parse().unwrap_or(0.0);
                        }
                    }
                } else {
                    i += 1;
                }
            }

            if status == "completed" {
                completed_count += 1;
                total_revenue += amount;
                *category_totals.entry(category.to_string()).or_insert(0.0) += amount;
            }
        }
        i += 1;
    }

    let elapsed = start.elapsed().as_secs_f64() * 1000.0;
    println!("Rust | Completed: {} | Revenue: {} | Categories: {} | Time: {:.2}ms",
             completed_count, total_revenue as i64, category_totals.len(), elapsed);
}
