#!/usr/bin/env python3
"""
MEMElang Milestone 7 E2E Test Suite: Generics / Type Parameters (P2)
Verifies:
  1. Generic struct declarations: sancha Dabba[T] { le saaman: T; ... }
  2. Generic function declarations: jugaad swap_karo[T](a: T, b: T): soochi[T]
  3. Generic function auto-deduction and explicit specialization call
  4. Multi-parameter generic structs and functions: Jodi[K, V]
  5. TypeChecker enforcement: rejection of invalid type assignment to generic instance
  6. Native AOT compilation parity across all generic features
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


def test_m7_suite():
    print("=== Running E2E Test Suite: Milestone 7 Generics & Type Parameters ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # Test 1: Generic struct in tree-walking interpreter
        total += 1
        code1 = """
sancha Dabba[T] {
    le saaman: T;
    jugaad naya(item: T) {
        khud.saaman = item;
    }
    jugaad dekho(): T {
        ye_le khud.saaman;
    }
}

pakad intBox = Dabba[ank](42);
pakad strBox = Dabba[bol]("namaste");
dikha f"INT_BOX:{intBox.dekho()}|STR_BOX:{strBox.dekho()}";
"""
        f1 = tmp_path / "t1.meme"
        f1.write_text(code1)
        res1 = run_command([str(MEME_BIN), str(f1)])
        if res1.returncode == 0 and "INT_BOX:42|STR_BOX:namaste" in res1.stdout:
            print("[PASS] T1_generic_struct_interpreter")
            passed += 1
        else:
            print(f"[FAIL] T1_generic_struct_interpreter: stdout: {res1.stdout}\nstderr: {res1.stderr}")
            sys.exit(1)

        # Test 2: Generic function with argument type deduction
        total += 1
        code2 = """
jugaad swap_karo[T](a: T, b: T): soochi[T] {
    ye_le [b, a];
}

pakad sw1 = swap_karo(10, 20);
pakad sw2 = swap_karo("pehla", "dusra");
dikha f"SW1:{sw1[0]},{sw1[1]}|SW2:{sw2[0]},{sw2[1]}";
"""
        f2 = tmp_path / "t2.meme"
        f2.write_text(code2)
        res2 = run_command([str(MEME_BIN), str(f2)])
        if res2.returncode == 0 and "SW1:20,10|SW2:dusra,pehla" in res2.stdout:
            print("[PASS] T1_generic_function_interpreter_deduction")
            passed += 1
        else:
            print(f"[FAIL] T1_generic_function_interpreter_deduction: stdout: {res2.stdout}\nstderr: {res2.stderr}")
            sys.exit(1)

        # Test 3: Generic function with explicit type parameter call
        total += 1
        code3 = """
jugaad identity[T](val: T): T {
    ye_le val;
}

pakad num = identity[ank](99);
pakad txt = identity[bol]("alvida");
dikha f"ID_NUM:{num}|ID_TXT:{txt}";
"""
        f3 = tmp_path / "t3.meme"
        f3.write_text(code3)
        res3 = run_command([str(MEME_BIN), str(f3)])
        if res3.returncode == 0 and "ID_NUM:99|ID_TXT:alvida" in res3.stdout:
            print("[PASS] T1_generic_function_explicit_type")
            passed += 1
        else:
            print(f"[FAIL] T1_generic_function_explicit_type: stdout: {res3.stdout}\nstderr: {res3.stderr}")
            sys.exit(1)

        # Test 4: Multi-parameter generic struct (Jodi[K, V])
        total += 1
        code4 = """
sancha Jodi[K, V] {
    le pehla: K;
    le dusra: V;
    jugaad naya(k: K, v: V) {
        khud.pehla = k;
        khud.dusra = v;
    }
    jugaad dikhao() {
        dikha f"PAIR:{khud.pehla}={khud.dusra}";
    }
}

pakad pair = Jodi[bol, ank]("chai_rate", 20);
pair.dikhao();
"""
        f4 = tmp_path / "t4.meme"
        f4.write_text(code4)
        res4 = run_command([str(MEME_BIN), str(f4)])
        if res4.returncode == 0 and "PAIR:chai_rate=20" in res4.stdout:
            print("[PASS] T1_multi_type_param_struct_interpreter")
            passed += 1
        else:
            print(f"[FAIL] T1_multi_type_param_struct_interpreter: stdout: {res4.stdout}\nstderr: {res4.stderr}")
            sys.exit(1)

        # Test 5: TypeChecker negative test: assigning invalid type to generic field
        total += 1
        code5 = """
sancha Dabba[T] {
    le saaman: T;
    jugaad naya(item: T) {
        khud.saaman = item;
    }
}

pakad box = Dabba[ank](42);
box.saaman = "string_invalid";
"""
        f5 = tmp_path / "t5.meme"
        f5.write_text(code5)
        res5 = run_command([str(MEME_BIN), str(f5)])
        if res5.returncode != 0 and ("Doglapan" in res5.stderr or "Field 'saaman'" in res5.stderr):
            print("[PASS] T2_typechecker_generic_field_assignment_error")
            passed += 1
        else:
            print(f"[FAIL] T2_typechecker_generic_field_assignment_error: ret={res5.returncode}\nstderr={res5.stderr}")
            sys.exit(1)

        # Test 6: Native AOT compilation of generic struct
        total += 1
        out6 = tmp_path / "bin_t6"
        res6_c = run_command([str(MEME_BIN), "compile", str(f1), "-o", str(out6)])
        if res6_c.returncode == 0 and out6.exists():
            res6_e = run_command([str(out6)])
            if res6_e.returncode == 0 and "INT_BOX:42|STR_BOX:namaste" in res6_e.stdout:
                print("[PASS] T1_generic_struct_native_aot")
                passed += 1
            else:
                print(f"[FAIL] T1_generic_struct_native_aot exec: stdout={res6_e.stdout}\nstderr={res6_e.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_generic_struct_native_aot compile: stderr={res6_c.stderr}")
            sys.exit(1)

        # Test 7: Native AOT compilation of generic function with deduction & explicit type
        total += 1
        code7 = """
jugaad swap_karo[T](a: T, b: T): soochi[T] {
    ye_le [b, a];
}

jugaad identity[T](val: T): T {
    ye_le val;
}

pakad s = swap_karo(100, 200);
pakad idVal = identity[ank](777);
dikha f"AOT_SWAP:{s[0]},{s[1]}|ID:{idVal}";
"""
        f7 = tmp_path / "t7.meme"
        f7.write_text(code7)
        out7 = tmp_path / "bin_t7"
        res7_c = run_command([str(MEME_BIN), "compile", str(f7), "-o", str(out7)])
        if res7_c.returncode == 0 and out7.exists():
            res7_e = run_command([str(out7)])
            if res7_e.returncode == 0 and "AOT_SWAP:200,100|ID:777" in res7_e.stdout:
                print("[PASS] T1_generic_function_native_aot")
                passed += 1
            else:
                print(f"[FAIL] T1_generic_function_native_aot exec: stdout={res7_e.stdout}\nstderr={res7_e.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_generic_function_native_aot compile: stderr={res7_c.stderr}")
            sys.exit(1)

        # Test 8: Native AOT compilation of multi-parameter generic struct
        total += 1
        out8 = tmp_path / "bin_t8"
        res8_c = run_command([str(MEME_BIN), "compile", str(f4), "-o", str(out8)])
        if res8_c.returncode == 0 and out8.exists():
            res8_e = run_command([str(out8)])
            if res8_e.returncode == 0 and "PAIR:chai_rate=20" in res8_e.stdout:
                print("[PASS] T1_multi_type_param_struct_native_aot")
                passed += 1
            else:
                print(f"[FAIL] T1_multi_type_param_struct_native_aot exec: stdout={res8_e.stdout}\nstderr={res8_e.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_multi_type_param_struct_native_aot compile: stderr={res8_c.stderr}")
            sys.exit(1)

    print(f"\nResults: {passed}/{total} tests passed.")
    print("✔ All Milestone 7 E2E Tests Passed successfully!")


if __name__ == "__main__":
    test_m7_suite()
