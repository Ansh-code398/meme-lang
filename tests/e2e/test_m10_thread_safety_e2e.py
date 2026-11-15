#!/usr/bin/env python3
"""
MEMElang Milestone 10 E2E Test Suite: Thread Safety & Data Race Detection (P2)
Verifies:
  1. Tree-walking interpreter `dhaga.cores()` / `dhaga.aukaat()` reflection
  2. Tree-walking interpreter `dhaga.tala()` mutex lock / unlock / pakad / chhod
  3. Tree-walking interpreter `dhaga.chala()` spawning and `rok()` / `join()` synchronization
  4. Tree-walking interpreter `dhaga.sab()` batch concurrent execution
  5. Tree-walking interpreter concurrent shared state mutation protected by mutex
  6. Native AOT compiler multithreading parity (`meme compile`)
  7. Native AOT compiler ThreadSanitizer support (`meme compile --tsan`) with 0 data races
  8. Tree-walking interpreter `surakshit()` RAII mutex wrapper
"""

import os
import subprocess
import sys
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"


def run_command(cmd, cwd=None, input_text=None):
    try:
        proc = subprocess.run(
            cmd,
            cwd=cwd,
            input=input_text,
            capture_output=True,
            text=True,
            timeout=30,
        )
        return proc
    except Exception as e:
        return subprocess.CompletedProcess(cmd, returncode=-1, stdout="", stderr=str(e))


def test_m10_suite():
    print("=== Running E2E Test Suite: Milestone 10 Thread Safety & Data Race Detection ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # Test 1: Interpreter dhaga.cores() / dhaga.aukaat()
        total += 1
        code1 = """
pakad c = dhaga.cores();
dikha "Cores:", c;
"""
        src1 = tmp_path / "test1.meme"
        src1.write_text(code1, encoding="utf-8")
        res1 = run_command([str(MEME_BIN), str(src1)])
        if res1.returncode == 0 and "Cores:" in res1.stdout:
            print("[PASS] T1_interpreter_dhaga_cores")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_dhaga_cores: Code: {res1.returncode}, Stdout: {res1.stdout}, Stderr: {res1.stderr}")

        # Test 2: Interpreter mutex lock / unlock / pakad / chhod
        total += 1
        code2 = """
pakad m = dhaga.tala();
m.pakad();
m.chhod();
m.lock();
m.unlock();
dikha "Mutex OK";
"""
        src2 = tmp_path / "test2.meme"
        src2.write_text(code2, encoding="utf-8")
        res2 = run_command([str(MEME_BIN), str(src2)])
        if res2.returncode == 0 and "Mutex OK" in res2.stdout:
            print("[PASS] T2_interpreter_mutex_lock_unlock")
            passed += 1
        else:
            print(f"[FAIL] T2_interpreter_mutex_lock_unlock: Code: {res2.returncode}, Stdout: {res2.stdout}, Stderr: {res2.stderr}")

        # Test 3: Interpreter thread chala and rok / join
        total += 1
        code3 = """
jugaad compute() {
    ye_le 42;
}
pakad t = dhaga.chala(compute);
pakad res = t.rok();
dikha "Result:", res;
"""
        src3 = tmp_path / "test3.meme"
        src3.write_text(code3, encoding="utf-8")
        res3 = run_command([str(MEME_BIN), str(src3)])
        if res3.returncode == 0 and "Result: 42" in res3.stdout:
            print("[PASS] T3_interpreter_thread_chala_rok")
            passed += 1
        else:
            print(f"[FAIL] T3_interpreter_thread_chala_rok: Code: {res3.returncode}, Stdout: {res3.stdout}, Stderr: {res3.stderr}")

        # Test 4: Interpreter dhaga.sab batch execution
        total += 1
        code4 = """
jugaad task1() {
    ye_le 100;
}
jugaad task2() {
    ye_le 200;
}
pakad res = dhaga.sab([task1, task2]);
dikha "Batch:", res;
"""
        src4 = tmp_path / "test4.meme"
        src4.write_text(code4, encoding="utf-8")
        res4 = run_command([str(MEME_BIN), str(src4)])
        if res4.returncode == 0 and "Batch: [100, 200]" in res4.stdout:
            print("[PASS] T4_interpreter_thread_sab_batch")
            passed += 1
        else:
            print(f"[FAIL] T4_interpreter_thread_sab_batch: Code: {res4.returncode}, Stdout: {res4.stdout}, Stderr: {res4.stderr}")

        # Test 5: Interpreter concurrent counter with mutex
        total += 1
        code5 = """
pakad lock = dhaga.tala();
pakad counter = [0];

jugaad worker() {
    pakad i = 0;
    jabtak (i < 50) {
        lock.pakad();
        counter[0] = counter[0] + 1;
        lock.chhod();
        i = i + 1;
    }
    ye_le 0;
}

pakad t1 = dhaga.chala(worker);
pakad t2 = dhaga.chala(worker);
t1.rok();
t2.rok();
dikha "Counter:", counter[0];
"""
        src5 = tmp_path / "test5.meme"
        src5.write_text(code5, encoding="utf-8")
        res5 = run_command([str(MEME_BIN), str(src5)])
        if res5.returncode == 0 and "Counter: 100" in res5.stdout:
            print("[PASS] T5_interpreter_concurrent_counter_with_mutex")
            passed += 1
        else:
            print(f"[FAIL] T5_interpreter_concurrent_counter_with_mutex: Code: {res5.returncode}, Stdout: {res5.stdout}, Stderr: {res5.stderr}")

        # Test 6: Native AOT multithreading compilation and execution
        total += 1
        code6 = """
pakad c = dhaga.cores();
dikha "Cores:", c;

pakad m = dhaga.tala();
m.pakad();
m.chhod();
dikha "Mutex OK";

jugaad compute() {
    ye_le 42;
}

pakad t = dhaga.chala(compute);
pakad res = t.rok();
dikha "Result:", res;

jugaad task1() {
    ye_le 100;
}
jugaad task2() {
    ye_le 200;
}

pakad batch_res = dhaga.sab([task1, task2]);
dikha "Batch results:", batch_res;
"""
        src6 = tmp_path / "test6.meme"
        src6.write_text(code6, encoding="utf-8")
        bin6 = tmp_path / "bin6"
        compile6 = run_command([str(MEME_BIN), "compile", str(src6), "-o", str(bin6)])
        if compile6.returncode == 0 and bin6.exists():
            exec6 = run_command([str(bin6)])
            if exec6.returncode == 0 and "Batch results: [100, 200]" in exec6.stdout and "Result: 42" in exec6.stdout:
                print("[PASS] T6_native_aot_multithreading")
                passed += 1
            else:
                print(f"[FAIL] T6_native_aot_multithreading: Exec Code: {exec6.returncode}, Stdout: {exec6.stdout}, Stderr: {exec6.stderr}")
        else:
            print(f"[FAIL] T6_native_aot_multithreading compilation: Code: {compile6.returncode}, Stdout: {compile6.stdout}, Stderr: {compile6.stderr}")

        # Test 7: Native AOT ThreadSanitizer compilation and execution (0 data races)
        total += 1
        code7 = """
biradari Counter {
    jugaad shuru() {
        khud.val = 0;
    }
}

jugaad worker(lock, c: Counter) {
    pakad i = 0;
    jabtak (i < 50) {
        lock.pakad();
        c.val = c.val + 1;
        lock.chhod();
        i = i + 1;
    }
    ye_le 0;
}

pakad lock = dhaga.tala();
pakad c = Counter();

pakad t1 = dhaga.chala(worker, lock, c);
pakad t2 = dhaga.chala(worker, lock, c);
t1.rok();
t2.rok();

dikha "Final counter:", c.val;
"""
        src7 = tmp_path / "test7.meme"
        src7.write_text(code7, encoding="utf-8")
        bin7 = tmp_path / "bin7"
        compile7 = run_command([str(MEME_BIN), "compile", "--tsan", str(src7), "-o", str(bin7)])
        if compile7.returncode == 0 and bin7.exists():
            exec7 = run_command([str(bin7)])
            combined = exec7.stdout + exec7.stderr
            if exec7.returncode == 0 and "Final counter: 100" in exec7.stdout and "WARNING: ThreadSanitizer" not in combined:
                print("[PASS] T7_native_aot_tsan_clean_run")
                passed += 1
            else:
                print(f"[FAIL] T7_native_aot_tsan_clean_run: Exec Code: {exec7.returncode}, Stdout: {exec7.stdout}, Stderr: {exec7.stderr}")
        else:
            print(f"[FAIL] T7_native_aot_tsan_clean_run compilation: Code: {compile7.returncode}, Stdout: {compile7.stdout}, Stderr: {compile7.stderr}")

        # Test 8: Interpreter surakshit() RAII mutex wrapper
        total += 1
        code8 = """
pakad lock = dhaga.tala();
pakad counter = [0];

jugaad inc() {
    counter[0] = counter[0] + 1;
    ye_le counter[0];
}

jugaad worker() {
    pakad i = 0;
    jabtak (i < 25) {
        lock.surakshit(inc);
        i = i + 1;
    }
    ye_le 0;
}

pakad t1 = dhaga.chala(worker);
pakad t2 = dhaga.chala(worker);
t1.rok();
t2.rok();

dikha "Surakshit Counter:", counter[0];
"""
        src8 = tmp_path / "test8.meme"
        src8.write_text(code8, encoding="utf-8")
        res8 = run_command([str(MEME_BIN), str(src8)])
        if res8.returncode == 0 and "Surakshit Counter: 50" in res8.stdout:
            print("[PASS] T8_interpreter_mutex_surakshit")
            passed += 1
        else:
            print(f"[FAIL] T8_interpreter_mutex_surakshit: Code: {res8.returncode}, Stdout: {res8.stdout}, Stderr: {res8.stderr}")

    print(f"\nMilestone 10 Test Results: {passed}/{total} Passed")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(test_m10_suite())
