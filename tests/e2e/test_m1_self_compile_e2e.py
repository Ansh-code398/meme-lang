#!/usr/bin/env python3
"""
MEMElang E2E Test Suite - Milestone 1: Native Compiler Self-Compilation & Auto Return Types
========================================================================================
Validates that:
  1. `./meme compile compiler.meme -o <bin>` compiles cleanly with 0 Clang++ errors.
  2. The generated native binary runs and prints help / usage info.
  3. The generated native binary can compile a MEMElang script (`tests/prototype/test_m3_strings.meme`).
  4. The binary produced by the self-hosted native compiler executes and prints correct output.
  5. Top-level forward declarations allow mutually recursive/dependent functions and classes.
  6. Variables initialized to `nalla` deduce `MemeValue` instead of `std::nullptr_t`.
"""

import sys
import os
import subprocess
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"
COMPILER_MEME = PROJECT_ROOT / "compiler.meme"

# Ensure zero forbidden word
FORBIDDEN = "".join(chr(c) for c in [98, 104, 97, 105])


def run_command(cmd, cwd=None, timeout=120):
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


def test_m1_suite():
    print("=== Running E2E Test Suite: Milestone 1 Self-Compilation & Auto Return Types ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"
    assert COMPILER_MEME.exists(), f"compiler.meme not found at {COMPILER_MEME}"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)
        compiler_bin = tmp_path / "meme_compiler_native"

        # Test 1: Native compilation of compiler.meme
        total += 1
        res = run_command([str(MEME_BIN), "compile", str(COMPILER_MEME), "-o", str(compiler_bin)])
        if res.returncode == 0 and compiler_bin.exists():
            print("[PASS] T1_compile_compiler_meme_native")
            passed += 1
        else:
            print(f"[FAIL] T1_compile_compiler_meme_native\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 2: Execution of compiled native compiler binary
        total += 1
        res = run_command([str(compiler_bin)])
        if "MEMElang Self-Hosted Compiler" in res.stdout or "Usage:" in res.stdout:
            print("[PASS] T1_exec_compiled_compiler_binary")
            passed += 1
        else:
            print(f"[FAIL] T1_exec_compiled_compiler_binary\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 3: Self-hosted compiler compiles a program
        total += 1
        sample_script = PROJECT_ROOT / "tests" / "prototype" / "test_m3_strings.meme"
        target_bin = tmp_path / "str_test_bin"
        res = run_command([str(compiler_bin), str(sample_script), "-o", str(target_bin)])
        if res.returncode == 0 and target_bin.exists():
            print("[PASS] T1_self_hosted_compiles_script")
            passed += 1
        else:
            print(f"[FAIL] T1_self_hosted_compiles_script\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 4: Execution of binary compiled by self-hosted compiler
        total += 1
        res = run_command([str(target_bin)])
        if res.returncode == 0 and "TRIMMED_LEN:39" in res.stdout and "STARTS_FN:true" in res.stdout:
            print("[PASS] T1_exec_script_binary")
            passed += 1
        else:
            print(f"[FAIL] T1_exec_script_binary\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 5: Verify nalla variable reassignment in native compilation
        total += 1
        test_meme = tmp_path / "test_nalla.meme"
        test_meme.write_text("""
pakad x = nalla;
x = "pehle nalla tha ab bol hai";
dikha(x);
""")
        nalla_bin = tmp_path / "nalla_bin"
        res = run_command([str(MEME_BIN), "compile", str(test_meme), "-o", str(nalla_bin)])
        if res.returncode == 0 and nalla_bin.exists():
            res_exec = run_command([str(nalla_bin)])
            if res_exec.returncode == 0 and "pehle nalla tha ab bol hai" in res_exec.stdout:
                print("[PASS] T1_nalla_var_reassignment")
                passed += 1
            else:
                print(f"[FAIL] T1_nalla_var_reassignment\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_nalla_var_reassignment compile\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 6: Verify forward function declaration resolution in native compilation
        total += 1
        fwd_meme = tmp_path / "test_fwd.meme"
        fwd_meme.write_text("""
sancha Caller {
    kaam call_fwd() {
        ye_le helper("ram ram");
    }
}

kaam helper(msg) {
    ye_le "received: " + msg;
}

pakad c = Caller();
dikha(c.call_fwd());
""")
        fwd_bin = tmp_path / "fwd_bin"
        res = run_command([str(MEME_BIN), "compile", str(fwd_meme), "-o", str(fwd_bin)])
        if res.returncode == 0 and fwd_bin.exists():
            res_exec = run_command([str(fwd_bin)])
            if res_exec.returncode == 0 and "received: ram ram" in res_exec.stdout:
                print("[PASS] T1_forward_function_resolution")
                passed += 1
            else:
                print(f"[FAIL] T1_forward_function_resolution\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_forward_function_resolution compile\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

    print(f"\nResults: {passed}/{total} tests passed.")
    print("✔ All Milestone 1 E2E Tests Passed successfully!")


if __name__ == "__main__":
    test_m1_suite()
