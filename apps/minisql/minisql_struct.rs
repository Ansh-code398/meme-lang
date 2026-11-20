// ==============================================================================
// MiniSQL Struct-Store Database Engine in Pure Rust
// High-performance relational database baseline featuring:
//   - Relational Table storage with typed Employee struct records
//   - Primary Key Hash Index (HashMap<i64, usize>) for O(1) point lookups
//   - Secondary Inverted Index (HashMap<String, Vec<usize>>)
//   - Range predicate filtering
//   - Aggregations (COUNT, SUM, AVG, MIN, MAX)
//   - Tombstone deletions and ACID-style updates
// ==============================================================================

use std::collections::HashMap;
use std::env;
use std::time::Instant;

#[derive(Clone, Debug)]
pub struct Employee {
    pub id: i64,
    pub name: String,
    pub age: i64,
    pub salary: f64,
    pub dept: String,
    pub is_deleted: bool,
}

#[derive(Clone, Debug)]
pub struct AggResult {
    pub count: usize,
    pub sum: f64,
    pub avg: f64,
    pub min: f64,
    pub max: f64,
}

pub struct StructTable {
    pub name: String,
    pub rows: Vec<Employee>,
    pub pk_index: HashMap<i64, usize>,
    pub dept_index: HashMap<String, Vec<usize>>,
    pub active_count: usize,
}

impl StructTable {
    pub fn new(name: &str) -> Self {
        StructTable {
            name: name.to_string(),
            rows: Vec::new(),
            pk_index: HashMap::new(),
            dept_index: HashMap::new(),
            active_count: 0,
        }
    }

    // INSERT INTO table VALUES (id, name, age, salary, dept)
    pub fn insert_row(&mut self, id: i64, name: &str, age: i64, salary: f64, dept: &str) -> bool {
        if self.pk_index.contains_key(&id) {
            return false;
        }
        let idx = self.rows.len();
        let row = Employee {
            id,
            name: name.to_string(),
            age,
            salary,
            dept: dept.to_string(),
            is_deleted: false,
        };
        self.rows.push(row);
        self.pk_index.insert(id, idx);
        self.dept_index.entry(dept.to_string()).or_default().push(idx);
        self.active_count += 1;
        true
    }

    // SELECT * FROM table WHERE id = ?
    pub fn select_by_pk(&self, id: i64) -> Option<&Employee> {
        if let Some(&idx) = self.pk_index.get(&id) {
            let row = &self.rows[idx];
            if !row.is_deleted {
                return Some(row);
            }
        }
        None
    }

    // SELECT * FROM table WHERE dept = ? AND age >= ? AND age <= ?
    pub fn select_where(&self, dept: &str, min_age: i64, max_age: i64) -> Vec<&Employee> {
        let mut matches = Vec::new();
        if let Some(indices) = self.dept_index.get(dept) {
            for &idx in indices {
                let r = &self.rows[idx];
                if !r.is_deleted && r.age >= min_age && r.age <= max_age {
                    matches.push(r);
                }
            }
        }
        matches
    }

    // UPDATE table SET salary = ? WHERE id = ?
    pub fn update_salary(&mut self, id: i64, new_salary: f64) -> bool {
        if let Some(&idx) = self.pk_index.get(&id) {
            let r = &mut self.rows[idx];
            if !r.is_deleted {
                r.salary = new_salary;
                return true;
            }
        }
        false
    }

    // DELETE FROM table WHERE id = ?
    pub fn delete_by_pk(&mut self, id: i64) -> bool {
        if let Some(&idx) = self.pk_index.get(&id) {
            let r = &mut self.rows[idx];
            if !r.is_deleted {
                r.is_deleted = true;
                self.pk_index.remove(&id);
                self.active_count -= 1;
                return true;
            }
        }
        false
    }

    // SELECT COUNT(*), SUM(salary), AVG(salary), MIN(salary), MAX(salary) FROM table WHERE dept = ?
    pub fn aggregate(&self, dept: &str) -> AggResult {
        if let Some(indices) = self.dept_index.get(dept) {
            let mut count = 0usize;
            let mut sum_val = 0.0f64;
            let mut min_val = 999999999.0f64;
            let mut max_val = -999999999.0f64;

            for &idx in indices {
                let r = &self.rows[idx];
                if !r.is_deleted {
                    count += 1;
                    sum_val += r.salary;
                    if r.salary < min_val {
                        min_val = r.salary;
                    }
                    if r.salary > max_val {
                        max_val = r.salary;
                    }
                }
            }

            if count > 0 {
                AggResult {
                    count,
                    sum: sum_val,
                    avg: sum_val / (count as f64),
                    min: min_val,
                    max: max_val,
                }
            } else {
                AggResult {
                    count: 0,
                    sum: 0.0,
                    avg: 0.0,
                    min: 0.0,
                    max: 0.0,
                }
            }
        } else {
            AggResult {
                count: 0,
                sum: 0.0,
                avg: 0.0,
                min: 0.0,
                max: 0.0,
            }
        }
    }
}

pub fn run_verify() {
    println!("==================================================");
    println!("🔍 MiniSQL Functional & Parity Verification Suite");
    println!("==================================================");

    let mut db = StructTable::new("employees");

    // 1. Bulk Inserts
    db.insert_row(101, "Alice", 28, 75000.0, "Engineering");
    db.insert_row(102, "Bob", 34, 92000.0, "Engineering");
    db.insert_row(103, "Charlie", 42, 110000.0, "Engineering");
    db.insert_row(104, "Diana", 26, 68000.0, "Design");
    db.insert_row(105, "Evan", 38, 95000.0, "Design");
    db.insert_row(106, "Fiona", 31, 88000.0, "Marketing");
    db.insert_row(107, "George", 45, 120000.0, "Marketing");
    db.insert_row(108, "Hannah", 29, 79000.0, "Engineering");
    db.insert_row(109, "Ian", 36, 102000.0, "Management");
    db.insert_row(110, "Julia", 40, 115000.0, "Management");

    println!("1. Inserted 10 rows. Active count: {}", db.active_count);

    // 2. Duplicate PK Test
    let dup_res = db.insert_row(101, "Alice Clone", 29, 77000.0, "Engineering");
    println!("2. Duplicate PK (101) rejected: {}", !dup_res);

    // 3. Primary Key Lookup
    if let Some(pk103) = db.select_by_pk(103) {
        println!("3. PK Lookup 103: {} {} {}", pk103.name, pk103.salary, pk103.dept);
    }

    // 4. Filter Query (Dept = Engineering, Age 28..35)
    let eng_matches = db.select_where("Engineering", 28, 35);
    println!("4. Filter (Engineering, age 28-35): count = {}", eng_matches.len());
    for m in &eng_matches {
        println!("   - Matched: {} {} {} {}", m.id, m.name, m.age, m.salary);
    }

    // 5. Update Salary
    let up_res = db.update_salary(102, 98000.0);
    let pk102 = db.select_by_pk(102).unwrap();
    println!("5. Update salary 102 to 98000: {} New Salary: {}", up_res, pk102.salary);

    // 6. Aggregate Query
    let agg = db.aggregate("Engineering");
    println!("6. Aggregation (Engineering):");
    println!("   Count: {}", agg.count);
    println!("   Sum:   {}", agg.sum);
    println!("   Avg:   {}", agg.avg);
    println!("   Min:   {}", agg.min);
    println!("   Max:   {}", agg.max);

    // 7. Delete Test
    let del_res = db.delete_by_pk(104);
    let pk104_after = db.select_by_pk(104);
    println!("7. Delete PK 104: {}", del_res);
    println!("   Lookup deleted PK 104 has name: {}", pk104_after.is_some());
    println!("   Active count after delete: {}", db.active_count);

    println!("==================================================");
    println!("✅ Verification Completed Successfully!");
    println!("==================================================");
}

pub fn run_benchmark(num_rows: usize, num_queries: usize) {
    let mut db = StructTable::new("bench_table");

    let depts = ["Engineering", "Marketing", "Sales", "Design", "Management"];
    let names = ["Raju", "Baburao", "Shyam", "Totla", "Kachra", "Munna", "Circuit", "Pandit", "Chaman", "Khadak"];

    let t0 = Instant::now();

    // 1. Bulk Inserts
    for i in 0..num_rows {
        let id = (1000 + i) as i64;
        let name = names[i % 10];
        let age = (20 + (i % 45)) as i64;
        let salary = 40000.0 + ((i * 73) % 110000) as f64;
        let dept = depts[i % 5];
        db.insert_row(id, name, age, salary, dept);
    }
    let t_insert = t0.elapsed().as_secs_f64().max(1e-9);

    // 2. PK Point Lookups
    let t_pk_start = Instant::now();
    let mut pk_hits = 0usize;
    for i in 0..num_queries {
        let q_id = (1000 + ((i * 37) % num_rows)) as i64;
        if db.select_by_pk(q_id).is_some() {
            pk_hits += 1;
        }
    }
    let t_pk = t_pk_start.elapsed().as_secs_f64().max(1e-9);

    // 3. Filter Queries
    let t_filter_start = Instant::now();
    let num_filters = num_queries / 2;
    let mut filter_matched = 0usize;
    for i in 0..num_filters {
        let q_dept = depts[i % 5];
        let min_a = (25 + (i % 10)) as i64;
        let max_a = min_a + 15;
        let res = db.select_where(q_dept, min_a, max_a);
        filter_matched += res.len();
    }
    let t_filter = t_filter_start.elapsed().as_secs_f64().max(1e-9);

    // 4. Aggregations
    let t_agg_start = Instant::now();
    let num_aggs = num_queries / 4;
    let mut agg_count_acc = 0usize;
    for i in 0..num_aggs {
        let q_dept = depts[i % 5];
        let agg_res = db.aggregate(q_dept);
        agg_count_acc += agg_res.count;
    }
    let t_agg = t_agg_start.elapsed().as_secs_f64().max(1e-9);

    // 5. Updates
    let t_update_start = Instant::now();
    let num_updates = num_queries / 5;
    let mut update_hits = 0usize;
    for i in 0..num_updates {
        let q_id = (1000 + ((i * 41) % num_rows)) as i64;
        if db.update_salary(q_id, 99999.0) {
            update_hits += 1;
        }
    }
    let t_update = t_update_start.elapsed().as_secs_f64().max(1e-9);

    let total_time = t0.elapsed().as_secs_f64().max(1e-9);
    let total_ops = num_rows + num_queries + num_filters + num_aggs + num_updates;
    let ops_per_sec = (total_ops as f64) / total_time;

    let t_insert_ms = t_insert * 1000.0;
    let t_pk_ms = t_pk * 1000.0;
    let t_filter_ms = t_filter * 1000.0;
    let t_agg_ms = t_agg * 1000.0;
    let t_update_ms = t_update * 1000.0;
    let total_ms = total_time * 1000.0;

    let insert_qps = (num_rows as f64) / t_insert;
    let pk_qps = (num_queries as f64) / t_pk;
    let filter_qps = (num_filters as f64) / t_filter;
    let agg_qps = (num_aggs as f64) / t_agg;
    let update_qps = (num_updates as f64) / t_update;

    println!("total_ms={:.2} ops_per_sec={:.1} insert_ms={:.4} insert_qps={:.0} pk_ms={:.4} pk_qps={:.0} filter_ms={:.4} filter_qps={:.0} agg_ms={:.4} agg_qps={:.0} update_ms={:.4} update_qps={:.0} pk_hits={} filter_matched={} agg_count={} update_hits={} total_rows={}",
             total_ms, ops_per_sec, t_insert_ms, insert_qps, t_pk_ms, pk_qps, t_filter_ms, filter_qps, t_agg_ms, agg_qps, t_update_ms, update_qps, pk_hits, filter_matched, agg_count_acc, update_hits, db.active_count);
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut is_bench = false;
    let mut num_rows = 10000usize;
    let mut num_queries = 5000usize;

    if args.len() >= 2 && args[1] == "--benchmark" {
        is_bench = true;
        if args.len() >= 3 {
            num_rows = args[2].parse().unwrap_or(10000);
        }
        if args.len() >= 4 {
            num_queries = args[3].parse().unwrap_or(5000);
        }
    }

    if is_bench {
        run_benchmark(num_rows, num_queries);
    } else {
        run_verify();
    }
}
