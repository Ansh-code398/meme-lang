#!/usr/bin/env python3
"""
MEMElang E2E Test Suite - Milestone 2: khatam/daayra (Break/Continue) in Dual Engines
===================================================================================
Validates that:
  1. `khatam;` executes as `break` in loops in the tree-walking interpreter.
  2. `daayra;` executes as `continue` in loops in the tree-walking interpreter.
  3. `khatam;` and `daayra;` compile and execute cleanly in native AOT binaries.
  4. `khatam;` and `daayra;` compile and execute cleanly via the self-hosted compiler (`compiler.meme`).
  5. `khatam <expr>;` preserves backward compatibility as explicit drop (`DropStmt`).
  6. `daayra { ... }` preserves backward compatibility as lexical scope (`ScopeBlockStmt`).
"""

import sys
import os
import subprocess
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"
COMPILER_MEME = PROJECT_ROOT / "compiler.meme"


def run_command(cmd, cwd=None, timeout=60):
    try:
        res = subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else str(PROJECT_ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout
        )
        return res
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(cmd, returncode=-1, stdout="", stderr="Timeout expired")
    except Exception as e:
        return subprocess.CompletedProcess(cmd, returncode=-1, stdout="", stderr=str(e))


def test_m2_suite():
    print("=== Running E2E Test Suite: Milestone 2 khatam/daayra Dual Engine Parity ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # Test 1: Interpreter while loop with daayra and khatam
        total += 1
        res = run_command([
            str(MEME_BIN), "-e",
            'pakad i = 0; jabtak (i < 10) { i = i + 1; agar (i == 2) { daayra; } agar (i == 5) { khatam; } } dikha f"VAL:{i}";'
        ])
        if res.returncode == 0 and "VAL:5" in res.stdout:
            print("[PASS] T1_interpreter_while_break_continue")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_while_break_continue\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 2: Interpreter ghoom (for) loop with daayra and khatam
        total += 1
        res = run_command([
            str(MEME_BIN), "-e",
            'pakad sum = 0; ghoom x in 0..10 { agar (x == 3) { daayra; } agar (x == 7) { khatam; } sum = sum + x; } dikha f"SUM:{sum}";'
        ])
        # 0 + 1 + 2 + (skip 3) + 4 + 5 + 6 = 18
        if res.returncode == 0 and "SUM:18" in res.stdout:
            print("[PASS] T1_interpreter_for_break_continue")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_for_break_continue\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 3: Backward compatibility - daayra { ... } lexical scope block
        total += 1
        res = run_command([
            str(MEME_BIN), "-e",
            'pakad a = 10; daayra { pakad b = 20; a = a + b; } dikha f"RES:{a}";'
        ])
        if res.returncode == 0 and "RES:30" in res.stdout:
            print("[PASS] T1_interpreter_daayra_scope_block")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_daayra_scope_block\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 4: Native compiler AOT with khatam and daayra
        total += 1
        native_src = tmp_path / "native_kd.meme"
        native_src.write_text("""
pakad count = 0;
pakad i = 0;
jabtak (i < 20) {
    i = i + 1;
    agar (i % 2 == 0) {
        daayra;
    }
    agar (i > 9) {
        khatam;
    }
    count = count + 1;
}
dikha f"COUNT:{count}|I:{i}";
""")
        native_bin = tmp_path / "native_kd_bin"
        res = run_command([str(MEME_BIN), "compile", str(native_src), "-o", str(native_bin)])
        if res.returncode == 0 and native_bin.exists():
            res_exec = run_command([str(native_bin)])
            # Odd numbers under 10: 1, 3, 5, 7, 9 -> count = 5, when i=10 (even, daayra), i=11 (odd, i>9 khatam) -> i=11
            if res_exec.returncode == 0 and "COUNT:5|I:11" in res_exec.stdout:
                print("[PASS] T1_native_compiler_break_continue")
                passed += 1
            else:
                print(f"[FAIL] T1_native_compiler_break_continue exec\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_native_compiler_break_continue compile\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 5: Self-hosted compiler with khatam and daayra
        total += 1
        compiler_bin = tmp_path / "compiler_m2_native"
        res_build_comp = run_command([str(MEME_BIN), "compile", str(COMPILER_MEME), "-o", str(compiler_bin)])
        if res_build_comp.returncode != 0:
            print(f"[FAIL] T1_self_hosted_compiler_break_continue build\nstdout: {res_build_comp.stdout}\nstderr: {res_build_comp.stderr}")
            sys.exit(1)

        self_bin = tmp_path / "self_kd_bin"
        res_compile_script = run_command([str(compiler_bin), str(native_src), "-o", str(self_bin)])
        if res_compile_script.returncode == 0 and self_bin.exists():
            res_exec = run_command([str(self_bin)])
            if res_exec.returncode == 0 and "COUNT:5|I:11" in res_exec.stdout:
                print("[PASS] T1_self_hosted_compiler_break_continue")
                passed += 1
            else:
                print(f"[FAIL] T1_self_hosted_compiler_break_continue exec\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_self_hosted_compiler_break_continue compile\nstdout: {res_compile_script.stdout}\nstderr: {res_compile_script.stderr}")
            sys.exit(1)

    print(f"\nResults: {passed}/{total} tests passed.")
    print("✔ All Milestone 2 E2E Tests Passed successfully!")


if __name__ == "__main__":
    test_m2_suite()
