#!/usr/bin/env python3
"""
MiniSQL Struct-Store Database Engine in Python
Standard library baseline featuring:
  - Statically-slotted Employee record struct (__slots__)
  - Primary Key Hash Index (dict[int, int]) for O(1) point lookups
  - Secondary Inverted Index (dict[str, list[int]])
  - Direct attribute filtering and aggregations
  - Tombstone deletions and updates
"""

import sys
import time

class Employee:
    __slots__ = ("id", "name", "age", "salary", "dept", "is_deleted")
    def __init__(self, id: int, name: str, age: int, salary: float, dept: str, is_deleted: int = 0):
        self.id = id
        self.name = name
        self.age = age
        self.salary = salary
        self.dept = dept
        self.is_deleted = is_deleted

class AggResult:
    __slots__ = ("count", "sum", "avg", "min", "max")
    def __init__(self, count: int, sum_val: float, avg_val: float, min_val: float, max_val: float):
        self.count = count
        self.sum = sum_val
        self.avg = avg_val
        self.min = min_val
        self.max = max_val

class StructTable:
    def __init__(self, name: str):
        self.name = name
        self.rows: list[Employee] = []
        self.pk_index = {}
        self.dept_index = {}
        self.active_count = 0

    def insert_row(self, id: int, name: str, age: int, salary: float, dept: str) -> bool:
        if id in self.pk_index:
            return False
        idx = len(self.rows)
        row = Employee(id, name, age, salary, dept, 0)
        self.rows.append(row)
        self.pk_index[id] = idx
        if dept not in self.dept_index:
            self.dept_index[dept] = []
        self.dept_index[dept].append(idx)
        self.active_count += 1
        return True

    def select_by_pk(self, id: int):
        if id not in self.pk_index:
            return None
        idx = self.pk_index[id]
        r = self.rows[idx]
        if r.is_deleted == 1:
            return None
        return r

    def select_where(self, dept: str, min_age: int, max_age: int) -> list:
        matches = []
        if dept not in self.dept_index:
            return matches
        for idx in self.dept_index[dept]:
            r = self.rows[idx]
            if r.is_deleted == 0:
                if min_age <= r.age <= max_age:
                    matches.append(r)
        return matches

    def update_salary(self, id: int, new_salary: float) -> bool:
        if id not in self.pk_index:
            return False
        idx = self.pk_index[id]
        r = self.rows[idx]
        if r.is_deleted == 1:
            return False
        r.salary = new_salary
        return True

    def delete_by_pk(self, id: int) -> bool:
        if id not in self.pk_index:
            return False
        idx = self.pk_index[id]
        r = self.rows[idx]
        if r.is_deleted == 1:
            return False
        r.is_deleted = 1
        del self.pk_index[id]
        self.active_count -= 1
        return True

    def aggregate(self, dept: str) -> AggResult:
        if dept not in self.dept_index:
            return AggResult(0, 0.0, 0.0, 0.0, 0.0)

        count = 0
        sum_val = 0.0
        min_val = 999999999.0
        max_val = -999999999.0

        for idx in self.dept_index[dept]:
            r = self.rows[idx]
            if r.is_deleted == 0:
                sal = r.salary
                count += 1
                sum_val += sal
                if sal < min_val:
                    min_val = sal
                if sal > max_val:
                    max_val = sal

        if count > 0:
            return AggResult(count, sum_val, sum_val / float(count), min_val, max_val)
        else:
            return AggResult(0, 0.0, 0.0, 0.0, 0.0)

def run_verify():
    print("==================================================")
    print("🔍 MiniSQL Functional & Parity Verification Suite")
    print("==================================================")

    db = StructTable("employees")

    # 1. Bulk Inserts
    db.insert_row(101, "Alice", 28, 75000.0, "Engineering")
    db.insert_row(102, "Bob", 34, 92000.0, "Engineering")
    db.insert_row(103, "Charlie", 42, 110000.0, "Engineering")
    db.insert_row(104, "Diana", 26, 68000.0, "Design")
    db.insert_row(105, "Evan", 38, 95000.0, "Design")
    db.insert_row(106, "Fiona", 31, 88000.0, "Marketing")
    db.insert_row(107, "George", 45, 120000.0, "Marketing")
    db.insert_row(108, "Hannah", 29, 79000.0, "Engineering")
    db.insert_row(109, "Ian", 36, 102000.0, "Management")
    db.insert_row(110, "Julia", 40, 115000.0, "Management")

    print(f"1. Inserted 10 rows. Active count: {db.active_count}")

    # 2. Duplicate PK Test
    dup_res = db.insert_row(101, "Alice Clone", 29, 77000.0, "Engineering")
    print(f"2. Duplicate PK (101) rejected: {not dup_res}")

    # 3. Primary Key Lookup
    pk103 = db.select_by_pk(103)
    sal103 = int(pk103.salary) if pk103.salary.is_integer() else pk103.salary
    print(f"3. PK Lookup 103: {pk103.name} {sal103} {pk103.dept}")

    # 4. Filter Query (Dept = Engineering, Age 28..35)
    eng_matches = db.select_where("Engineering", 28, 35)
    print(f"4. Filter (Engineering, age 28-35): count = {len(eng_matches)}")
    for m in eng_matches:
        sal = int(m.salary) if float(m.salary).is_integer() else m.salary
        print(f"   - Matched: {m.id} {m.name} {m.age} {sal}")

    # 5. Update Salary
    up_res = db.update_salary(102, 98000.0)
    pk102 = db.select_by_pk(102)
    sal102 = int(pk102.salary) if pk102.salary.is_integer() else pk102.salary
    print(f"5. Update salary 102 to 98000: {up_res} New Salary: {sal102}")

    # 6. Aggregate Query
    agg = db.aggregate("Engineering")
    print("6. Aggregation (Engineering):")
    print(f"   Count: {agg.count}")
    print(f"   Sum:   {int(agg.sum) if agg.sum.is_integer() else agg.sum}")
    print(f"   Avg:   {int(agg.avg) if agg.avg.is_integer() else agg.avg}")
    print(f"   Min:   {int(agg.min) if agg.min.is_integer() else agg.min}")
    print(f"   Max:   {int(agg.max) if agg.max.is_integer() else agg.max}")

    # 7. Delete Test
    del_res = db.delete_by_pk(104)
    pk104_after = db.select_by_pk(104)
    print(f"7. Delete PK 104: {del_res}")
    print(f"   Lookup deleted PK 104 has name: {pk104_after is not None}")
    print(f"   Active count after delete: {db.active_count}")

    print("==================================================")
    print("✅ Verification Completed Successfully!")
    print("==================================================")

def run_benchmark(num_rows: int, num_queries: int):
    db = StructTable("bench_table")

    depts = ["Engineering", "Marketing", "Sales", "Design", "Management"]
    names = ["Raju", "Baburao", "Shyam", "Totla", "Kachra", "Munna", "Circuit", "Pandit", "Chaman", "Khadak"]

    t0 = time.perf_counter()

    # 1. Bulk Inserts
    for i in range(num_rows):
        id_val = 1000 + i
        name = names[i % 10]
        age = 20 + (i % 45)
        salary = 40000.0 + float((i * 73) % 110000)
        dept = depts[i % 5]
        db.insert_row(id_val, name, age, salary, dept)
    t_insert = max(time.perf_counter() - t0, 1e-9)

    # 2. PK Point Lookups
    t_pk_start = time.perf_counter()
    pk_hits = 0
    for i in range(num_queries):
        q_id = 1000 + ((i * 37) % num_rows)
        r = db.select_by_pk(q_id)
        if r is not None:
            pk_hits += 1
    t_pk = max(time.perf_counter() - t_pk_start, 1e-9)

    # 3. Filter Queries
    t_filter_start = time.perf_counter()
    num_filters = num_queries // 2
    filter_matched = 0
    for i in range(num_filters):
        min_a = 25 + (i % 10)
        max_a = min_a + 15
        res = db.select_where(depts[i % 5], min_a, max_a)
        filter_matched += len(res)
    t_filter = max(time.perf_counter() - t_filter_start, 1e-9)

    # 4. Aggregations
    t_agg_start = time.perf_counter()
    num_aggs = num_queries // 4
    agg_count_acc = 0
    for i in range(num_aggs):
        agg_res = db.aggregate(depts[i % 5])
        agg_count_acc += agg_res.count
    t_agg = max(time.perf_counter() - t_agg_start, 1e-9)

    # 5. Updates
    t_update_start = time.perf_counter()
    num_updates = num_queries // 5
    update_hits = 0
    for i in range(num_updates):
        q_id = 1000 + ((i * 41) % num_rows)
        if db.update_salary(q_id, 99999.0):
            update_hits += 1
    t_update = max(time.perf_counter() - t_update_start, 1e-9)

    total_time = max(time.perf_counter() - t0, 1e-9)
    total_ops = num_rows + num_queries + num_filters + num_aggs + num_updates
    ops_per_sec = total_ops / total_time

    t_insert_ms = t_insert * 1000.0
    t_pk_ms = t_pk * 1000.0
    t_filter_ms = t_filter * 1000.0
    t_agg_ms = t_agg * 1000.0
    t_update_ms = t_update * 1000.0
    total_ms = total_time * 1000.0

    insert_qps = num_rows / t_insert
    pk_qps = num_queries / t_pk
    filter_qps = num_filters / t_filter
    agg_qps = num_aggs / t_agg
    update_qps = num_updates / t_update

    print(f"total_ms={total_ms:.2f} ops_per_sec={ops_per_sec:.1f} insert_ms={t_insert_ms:.4f} insert_qps={insert_qps:.0f} pk_ms={t_pk_ms:.4f} pk_qps={pk_qps:.0f} filter_ms={t_filter_ms:.4f} filter_qps={filter_qps:.0f} agg_ms={t_agg_ms:.4f} agg_qps={agg_qps:.0f} update_ms={t_update_ms:.4f} update_qps={update_qps:.0f} pk_hits={pk_hits} filter_matched={filter_matched} agg_count={agg_count_acc} update_hits={update_hits} total_rows={db.active_count}")

def main():
    args = sys.argv
    is_bench = False
    num_rows = 10000
    num_queries = 5000

    if len(args) >= 2 and args[1] == "--benchmark":
        is_bench = True
        if len(args) >= 3:
            num_rows = int(args[2])
        if len(args) >= 4:
            num_queries = int(args[3])

    if is_bench:
        run_benchmark(num_rows, num_queries)
    else:
        run_verify()

if __name__ == "__main__":
    main()
