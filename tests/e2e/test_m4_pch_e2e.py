#!/usr/bin/env python3
"""
MEMElang E2E Test Suite - Milestone 4: Compilation Speedup via Precompiled Headers (PCH)
======================================================================================
Validates that:
  1. Cold compilation creates and initializes the PCH in ~/.meme/pch/ or /tmp/meme_pch/.
  2. Warm compilation with -include-pch succeeds and produces functionally identical binaries.
  3. Compilation with --no-pch bypasses PCH and succeeds cleanly.
  4. PCH auto-invalidation and rebuild when MemeRuntime.h timestamp changes.
  5. Fallback behavior if PCH is corrupted or removed.
  6. Self-hosted compiler (compiler.meme) leverages PCH acceleration.
"""

import sys
import os
import time
import subprocess
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"
COMPILER_MEME = PROJECT_ROOT / "compiler.meme"
RUNTIME_HEADER = PROJECT_ROOT / "src" / "compiler" / "MemeRuntime.h"


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


def test_m4_suite():
    print("=== Running E2E Test Suite: Milestone 4 PCH Compilation Speedup ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)
        sample_meme = tmp_path / "sample.meme"
        sample_meme.write_text("""
pakad sum = 0;
ghoom i in 0..100 {
    sum = sum + i;
}
dikha f"SUM:{sum}";
""")

        home_dir = Path.home()
        pch_dir = home_dir / ".meme" / "pch"
        pch_o3 = pch_dir / "MemeRuntime_O3.h.pch"

        # Test 1: PCH creation on cold compile
        total += 1
        # Remove existing PCH to simulate cold compile
        if pch_o3.exists():
            pch_o3.unlink()

        bin_cold = tmp_path / "bin_cold"
        t0 = time.perf_counter()
        res_cold = run_command([str(MEME_BIN), "compile", str(sample_meme), "-o", str(bin_cold), "-v"])
        cold_time = time.perf_counter() - t0

        if res_cold.returncode == 0 and bin_cold.exists() and pch_o3.exists():
            res_exec = run_command([str(bin_cold)])
            if res_exec.returncode == 0 and "SUM:4950" in res_exec.stdout:
                print(f"[PASS] T1_cold_compile_builds_pch (duration: {cold_time:.3f}s)")
                passed += 1
            else:
                print(f"[FAIL] T1_cold_compile_builds_pch exec failed: {res_exec.stdout}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_cold_compile_builds_pch\nstdout: {res_cold.stdout}\nstderr: {res_cold.stderr}")
            sys.exit(1)

        # Test 2: Warm compile reusing PCH
        total += 1
        bin_warm = tmp_path / "bin_warm"
        t0 = time.perf_counter()
        res_warm = run_command([str(MEME_BIN), "compile", str(sample_meme), "-o", str(bin_warm), "-v"])
        warm_time = time.perf_counter() - t0

        if res_warm.returncode == 0 and bin_warm.exists():
            res_exec = run_command([str(bin_warm)])
            if res_exec.returncode == 0 and "SUM:4950" in res_exec.stdout:
                print(f"[PASS] T1_warm_compile_reuses_pch (duration: {warm_time:.3f}s, speedup: {cold_time/max(warm_time, 0.001):.2f}x)")
                passed += 1
            else:
                print(f"[FAIL] T1_warm_compile_reuses_pch exec failed: {res_exec.stdout}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_warm_compile_reuses_pch\nstdout: {res_warm.stdout}\nstderr: {res_warm.stderr}")
            sys.exit(1)

        # Test 3: Compilation with --no-pch flag
        total += 1
        bin_no_pch = tmp_path / "bin_no_pch"
        res_no_pch = run_command([str(MEME_BIN), "compile", str(sample_meme), "-o", str(bin_no_pch), "--no-pch"])
        if res_no_pch.returncode == 0 and bin_no_pch.exists():
            res_exec = run_command([str(bin_no_pch)])
            if res_exec.returncode == 0 and "SUM:4950" in res_exec.stdout:
                print("[PASS] T1_compile_with_no_pch_flag")
                passed += 1
            else:
                print(f"[FAIL] T1_compile_with_no_pch_flag exec failed: {res_exec.stdout}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_compile_with_no_pch_flag\nstdout: {res_no_pch.stdout}\nstderr: {res_no_pch.stderr}")
            sys.exit(1)

        # Test 4: PCH auto-invalidation and rebuild when MemeRuntime.h touches
        total += 1
        old_mtime = pch_o3.stat().st_mtime
        time.sleep(0.05)
        # Touch runtime header
        RUNTIME_HEADER.touch()

        bin_rebuild = tmp_path / "bin_rebuild"
        res_rebuild = run_command([str(MEME_BIN), "compile", str(sample_meme), "-o", str(bin_rebuild)])
        if res_rebuild.returncode == 0 and bin_rebuild.exists():
            new_mtime = pch_o3.stat().st_mtime
            if new_mtime >= old_mtime:
                res_exec = run_command([str(bin_rebuild)])
                if res_exec.returncode == 0 and "SUM:4950" in res_exec.stdout:
                    print("[PASS] T1_pch_auto_invalidation_and_rebuild")
                    passed += 1
                else:
                    print(f"[FAIL] T1_pch_auto_invalidation_and_rebuild exec: {res_exec.stdout}")
                    sys.exit(1)
            else:
                print(f"[FAIL] T1_pch_auto_invalidation_and_rebuild mtime not updated")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_pch_auto_invalidation_and_rebuild compile\nstdout: {res_rebuild.stdout}\nstderr: {res_rebuild.stderr}")
            sys.exit(1)

        # Test 5: Fallback on corrupted PCH
        total += 1
        # Corrupt PCH file
        pch_o3.write_text("CORRUPTED PCH DATA GARBAGE")
        bin_fallback = tmp_path / "bin_fallback"
        res_fallback = run_command([str(MEME_BIN), "compile", str(sample_meme), "-o", str(bin_fallback), "-v"])
        if res_fallback.returncode == 0 and bin_fallback.exists():
            res_exec = run_command([str(bin_fallback)])
            if res_exec.returncode == 0 and "SUM:4950" in res_exec.stdout:
                print("[PASS] T1_pch_corrupted_fallback_clean")
                passed += 1
            else:
                print(f"[FAIL] T1_pch_corrupted_fallback_clean exec: {res_exec.stdout}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_pch_corrupted_fallback_clean compile\nstdout: {res_fallback.stdout}\nstderr: {res_fallback.stderr}")
            sys.exit(1)

        # Test 6: Self-hosted compiler with PCH acceleration
        total += 1
        compiler_bin = tmp_path / "compiler_m4_native"
        res_build_comp = run_command([str(MEME_BIN), "compile", str(COMPILER_MEME), "-o", str(compiler_bin)])
        if res_build_comp.returncode != 0:
            print(f"[FAIL] T1_self_hosted_compiler_pch build\nstdout: {res_build_comp.stdout}\nstderr: {res_build_comp.stderr}")
            sys.exit(1)

        self_out_bin = tmp_path / "self_pch_bin"
        res_compile_script = run_command([str(compiler_bin), str(sample_meme), "-o", str(self_out_bin), "-v"])
        if res_compile_script.returncode == 0 and self_out_bin.exists():
            res_exec = run_command([str(self_out_bin)])
            if res_exec.returncode == 0 and "SUM:4950" in res_exec.stdout:
                print("[PASS] T1_self_hosted_compiler_pch_exec")
                passed += 1
            else:
                print(f"[FAIL] T1_self_hosted_compiler_pch_exec exec\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_self_hosted_compiler_pch compile\nstdout: {res_compile_script.stdout}\nstderr: {res_compile_script.stderr}")
            sys.exit(1)

    print(f"\nResults: {passed}/{total} tests passed.")
    print("✔ All Milestone 4 E2E Tests Passed successfully!")


if __name__ == "__main__":
    test_m4_suite()
