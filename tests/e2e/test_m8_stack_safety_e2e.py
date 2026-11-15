#!/usr/bin/env python3
"""
MEMElang Milestone 8 E2E Test Suite: Stack Safety & Call Depth Tracking (P2)
Verifies:
  1. Tree-walking interpreter infinite recursion protection (limit = 1000)
  2. Tree-walking interpreter safe non-infinite recursion within limit
  3. Native AOT compiler infinite recursion protection (limit = 10000, no SIGSEGV)
  4. Native AOT safe deep recursion within limit
  5. Mutual recursion detection in interpreter and native compiler
  6. Class method recursion detection in interpreter and native compiler
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


def test_m8_suite():
    print("=== Running E2E Test Suite: Milestone 8 Stack Safety & Call Depth Tracking ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # Test 1: Interpreter infinite recursion depth protection
        total += 1
        code1 = """
kaam infinite_loop() {
    infinite_loop();
}
infinite_loop();
"""
        src1 = tmp_path / "test1.meme"
        src1.write_text(code1, encoding="utf-8")
        res1 = run_command([str(MEME_BIN), str(src1)])
        if res1.returncode != 0 and "Stack overflow ho gaya! Maximum recursion depth (1000) exceed ho gayi in 'infinite_loop'!" in (res1.stderr + res1.stdout):
            print("[PASS] T1_interpreter_infinite_recursion_caught")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_infinite_recursion_caught: Code: {res1.returncode}, Stdout: {res1.stdout}, Stderr: {res1.stderr}")

        # Test 2: Interpreter safe recursion within limit (50 depth)
        total += 1
        code2 = """
kaam countdown(n) {
    agar (n <= 0) {
        wapas 0;
    }
    wapas countdown(n - 1) + 1;
}
dikha countdown(50);
"""
        src2 = tmp_path / "test2.meme"
        src2.write_text(code2, encoding="utf-8")
        res2 = run_command([str(MEME_BIN), str(src2)])
        if res2.returncode == 0 and "50" in res2.stdout:
            print("[PASS] T1_interpreter_safe_recursion_within_limit")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_safe_recursion_within_limit: Code: {res2.returncode}, Stdout: {res2.stdout}, Stderr: {res2.stderr}")

        # Test 3: Native AOT infinite recursion depth protection (exits cleanly without SIGSEGV / 139)
        total += 1
        code3 = """
kaam native_inf() {
    native_inf();
}
native_inf();
"""
        src3 = tmp_path / "test3.meme"
        src3.write_text(code3, encoding="utf-8")
        bin3 = tmp_path / "bin3"
        compile_res = run_command([str(MEME_BIN), "compile", str(src3), "-o", str(bin3)])
        if compile_res.returncode == 0 and bin3.exists():
            exec_res = run_command([str(bin3)])
            if exec_res.returncode != 0 and exec_res.returncode != -11 and exec_res.returncode != 139 and "Stack overflow ho gaya! Maximum recursion depth (10000) exceed ho gayi in 'native_inf'!" in (exec_res.stderr + exec_res.stdout):
                print("[PASS] T1_native_aot_infinite_recursion_caught")
                passed += 1
            else:
                print(f"[FAIL] T1_native_aot_infinite_recursion_caught: Code: {exec_res.returncode}, Output: {exec_res.stdout}, Stderr: {exec_res.stderr}")
        else:
            print(f"[FAIL] T1_native_aot_infinite_recursion_caught: Native compilation failed. Code: {compile_res.returncode}, Stderr: {compile_res.stderr}")

        # Test 4: Native AOT safe deep recursion within limit (50 depth)
        total += 1
        code4 = """
kaam native_countdown(n) {
    agar (n <= 0) {
        wapas 0;
    }
    wapas native_countdown(n - 1) + 1;
}
dikha native_countdown(50);
"""
        src4 = tmp_path / "test4.meme"
        src4.write_text(code4, encoding="utf-8")
        bin4 = tmp_path / "bin4"
        compile_res4 = run_command([str(MEME_BIN), "compile", str(src4), "-o", str(bin4)])
        if compile_res4.returncode == 0 and bin4.exists():
            exec_res4 = run_command([str(bin4)])
            if exec_res4.returncode == 0 and "50" in exec_res4.stdout:
                print("[PASS] T1_native_aot_safe_recursion_within_limit")
                passed += 1
            else:
                print(f"[FAIL] T1_native_aot_safe_recursion_within_limit: Code: {exec_res4.returncode}, Output: {exec_res4.stdout}, Stderr: {exec_res4.stderr}")
        else:
            print(f"[FAIL] T1_native_aot_safe_recursion_within_limit: Native compilation failed. Code: {compile_res4.returncode}, Stderr: {compile_res4.stderr}")

        # Test 5: Mutual recursion detection in interpreter
        total += 1
        code5 = """
kaam ping() {
    pong();
}
kaam pong() {
    ping();
}
ping();
"""
        src5 = tmp_path / "test5.meme"
        src5.write_text(code5, encoding="utf-8")
        res5 = run_command([str(MEME_BIN), str(src5)])
        if res5.returncode != 0 and "Stack overflow ho gaya! Maximum recursion depth (1000) exceed ho gayi" in (res5.stderr + res5.stdout):
            print("[PASS] T1_interpreter_mutual_recursion_caught")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_mutual_recursion_caught: Code: {res5.returncode}, Stdout: {res5.stdout}, Stderr: {res5.stderr}")

        # Test 6: Mutual recursion detection in native AOT
        total += 1
        code6 = """
kaam native_ping() {
    native_pong();
}
kaam native_pong() {
    native_ping();
}
native_ping();
"""
        src6 = tmp_path / "test6.meme"
        src6.write_text(code6, encoding="utf-8")
        bin6 = tmp_path / "bin6"
        compile_res6 = run_command([str(MEME_BIN), "compile", str(src6), "-o", str(bin6)])
        if compile_res6.returncode == 0 and bin6.exists():
            exec_res6 = run_command([str(bin6)])
            if exec_res6.returncode != 0 and exec_res6.returncode != 139 and "Stack overflow ho gaya! Maximum recursion depth (10000) exceed ho gayi" in (exec_res6.stderr + exec_res6.stdout):
                print("[PASS] T1_native_aot_mutual_recursion_caught")
                passed += 1
            else:
                print(f"[FAIL] T1_native_aot_mutual_recursion_caught: Code: {exec_res6.returncode}, Output: {exec_res6.stdout}, Stderr: {exec_res6.stderr}")
        else:
            print(f"[FAIL] T1_native_aot_mutual_recursion_caught: Native compilation failed. Code: {compile_res6.returncode}, Stderr: {compile_res6.stderr}")

        # Test 7: Class method recursion in interpreter
        total += 1
        code7 = """
sancha LoopBox {
    kaam spin() {
        this.spin();
    }
}
le lb = LoopBox();
lb.spin();
"""
        src7 = tmp_path / "test7.meme"
        src7.write_text(code7, encoding="utf-8")
        res7 = run_command([str(MEME_BIN), str(src7)])
        if res7.returncode != 0 and "Stack overflow ho gaya! Maximum recursion depth (1000) exceed ho gayi in 'spin'!" in (res7.stderr + res7.stdout):
            print("[PASS] T1_interpreter_class_method_recursion_caught")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_class_method_recursion_caught: Code: {res7.returncode}, Stdout: {res7.stdout}, Stderr: {res7.stderr}")

        # Test 8: Class method recursion in native AOT
        total += 1
        code8 = """
sancha NativeLoopBox {
    kaam spin() {
        this.spin();
    }
}
le nlb = NativeLoopBox();
nlb.spin();
"""
        src8 = tmp_path / "test8.meme"
        src8.write_text(code8, encoding="utf-8")
        bin8 = tmp_path / "bin8"
        compile_res8 = run_command([str(MEME_BIN), "compile", str(src8), "-o", str(bin8)])
        if compile_res8.returncode == 0 and bin8.exists():
            exec_res8 = run_command([str(bin8)])
            if exec_res8.returncode != 0 and exec_res8.returncode != 139 and "Stack overflow ho gaya! Maximum recursion depth (10000) exceed ho gayi in 'spin'!" in (exec_res8.stderr + exec_res8.stdout):
                print("[PASS] T1_native_aot_class_method_recursion_caught")
                passed += 1
            else:
                print(f"[FAIL] T1_native_aot_class_method_recursion_caught: Code: {exec_res8.returncode}, Output: {exec_res8.stdout}, Stderr: {exec_res8.stderr}")
        else:
            print(f"[FAIL] T1_native_aot_class_method_recursion_caught: Native compilation failed. Code: {compile_res8.returncode}, Stderr: {compile_res8.stderr}")

    print(f"\nMilestone 8 Summary: {passed}/{total} tests passed.")
    if passed == total:
        print("ALL M8 STACK SAFETY TESTS PASSED!")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(test_m8_suite())
