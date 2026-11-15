#!/usr/bin/env python3
"""
MEMElang E2E Test Suite - Milestone 6: Tapri Package Registry & Remote Git URLs
==============================================================================
Validates that:
  1. `meme tapri init` scaffolds tapri.json.
  2. `meme tapri install <name> <git-url>` clones git packages to `.tapri_modules/<name>`.
  3. `tapri.lock` is generated with pinned Git commit SHAs and paths.
  4. `tapri.json` dependency declarations are properly updated and persisted.
  5. Tree-walking interpreter resolves and executes modules imported from `.tapri_modules/`.
  6. Native AOT compiler resolves, transpiles, and links `.tapri_modules/` packages.
  7. Headless `meme tapri install` batch-restores all packages declared in `tapri.json`.
"""

import sys
import os
import subprocess
import tempfile
import json
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"


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


def test_m6_suite():
    print("=== Running E2E Test Suite: Milestone 6 Tapri Git Packages & Lockfile ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # Create a mock remote git repository with a MEMElang module
        git_remote = tmp_path / "mock_git_remote"
        git_remote.mkdir()
        run_command(["git", "init"], cwd=git_remote)
        run_command(["git", "config", "user.name", "TestUser"], cwd=git_remote)
        run_command(["git", "config", "user.email", "test@meme.dev"], cwd=git_remote)

        module_meme = git_remote / "colors.meme"
        module_meme.write_text("""
jugaad get_primary_color() {
    ye_le "safed";
}

jugaad multiply_cost(price, qty) {
    ye_le price * qty;
}
""")
        run_command(["git", "add", "."], cwd=git_remote)
        run_command(["git", "commit", "-m", "Initial commit"], cwd=git_remote)

        rev_res = run_command(["git", "rev-parse", "HEAD"], cwd=git_remote)
        expected_commit = rev_res.stdout.strip()
        assert len(expected_commit) == 40, f"Invalid git commit: {expected_commit}"

        # Setup consumer project
        app_dir = tmp_path / "app_project"
        app_dir.mkdir()

        # Test 1: tapri init
        total += 1
        res_init = run_command([str(MEME_BIN), "tapri", "init"], cwd=app_dir)
        manifest_file = app_dir / "tapri.json"
        if res_init.returncode == 0 and manifest_file.exists():
            print("[PASS] T1_tapri_init_creates_manifest")
            passed += 1
        else:
            print(f"[FAIL] T1_tapri_init_creates_manifest\nstdout: {res_init.stdout}\nstderr: {res_init.stderr}")
            sys.exit(1)

        # Test 2: tapri install from git url
        total += 1
        git_url = f"file://{git_remote.resolve()}"
        res_inst = run_command([str(MEME_BIN), "tapri", "install", "colors", git_url], cwd=app_dir)
        module_dest = app_dir / ".tapri_modules" / "colors"
        if res_inst.returncode == 0 and module_dest.exists() and (module_dest / "colors.meme").exists():
            print("[PASS] T1_tapri_install_git_clone")
            passed += 1
        else:
            print(f"[FAIL] T1_tapri_install_git_clone\nstdout: {res_inst.stdout}\nstderr: {res_inst.stderr}")
            sys.exit(1)

        # Test 3: tapri.lock generation with commit SHA
        total += 1
        lock_file = app_dir / "tapri.lock"
        if lock_file.exists():
            try:
                lock_data = json.loads(lock_file.read_text())
                pkg_info = lock_data.get("packages", {}).get("colors", {})
                if pkg_info.get("commit") == expected_commit and pkg_info.get("path") == ".tapri_modules/colors":
                    print("[PASS] T1_tapri_lockfile_pinned_commit")
                    passed += 1
                else:
                    print(f"[FAIL] T1_tapri_lockfile_pinned_commit incorrect contents: {pkg_info}")
                    sys.exit(1)
            except Exception as e:
                print(f"[FAIL] T1_tapri_lockfile_pinned_commit json parse: {e}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_tapri_lockfile_pinned_commit lockfile missing")
            sys.exit(1)

        # Test 4: tapri.json updated with dependency
        total += 1
        manifest_data = json.loads(manifest_file.read_text())
        if "colors" in manifest_data.get("dependencies", {}):
            print("[PASS] T1_tapri_manifest_dependencies_updated")
            passed += 1
        else:
            print(f"[FAIL] T1_tapri_manifest_dependencies_updated: {manifest_data}")
            sys.exit(1)

        # Test 5: Tree-walking interpreter import from .tapri_modules/
        total += 1
        script_file = app_dir / "main.meme"
        script_file.write_text("""
bula "colors";

pakad col = colors.get_primary_color();
pakad total = colors.multiply_cost(10, 5);
dikha f"INTERP_OUT:{col}|TOTAL:{total}";
""")
        res_interp = run_command([str(MEME_BIN), str(script_file)], cwd=app_dir)
        if res_interp.returncode == 0 and "INTERP_OUT:safed|TOTAL:50" in res_interp.stdout:
            print("[PASS] T1_tapri_interpreter_import")
            passed += 1
        else:
            print(f"[FAIL] T1_tapri_interpreter_import\nstdout: {res_interp.stdout}\nstderr: {res_interp.stderr}")
            sys.exit(1)

        # Test 6: Native AOT compiler resolves module from .tapri_modules/
        total += 1
        out_bin = app_dir / "app_native_bin"
        res_compile = run_command([str(MEME_BIN), "compile", str(script_file), "-o", str(out_bin)], cwd=app_dir)
        if res_compile.returncode == 0 and out_bin.exists():
            res_exec = run_command([str(out_bin)], cwd=app_dir)
            if res_exec.returncode == 0 and "INTERP_OUT:safed|TOTAL:50" in res_exec.stdout:
                print("[PASS] T1_tapri_native_aot_import")
                passed += 1
            else:
                print(f"[FAIL] T1_tapri_native_aot_import exec: {res_exec.stdout}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_tapri_native_aot_import compile\nstdout: {res_compile.stdout}\nstderr: {res_compile.stderr}")
            sys.exit(1)

        # Test 7: Headless tapri install restores packages from tapri.json
        total += 1
        # Delete .tapri_modules directory
        run_command(["rm", "-rf", ".tapri_modules"], cwd=app_dir)
        res_restore = run_command([str(MEME_BIN), "tapri", "install"], cwd=app_dir)
        if (
            res_restore.returncode == 0
            and module_dest.exists()
            and (module_dest / "colors.meme").exists()
        ):
            print("[PASS] T1_tapri_headless_install_restore")
            passed += 1
        else:
            print(f"[FAIL] T1_tapri_headless_install_restore\nstdout: {res_restore.stdout}\nstderr: {res_restore.stderr}")
            sys.exit(1)

    print(f"\nResults: {passed}/{total} tests passed.")
    print("✔ All Milestone 6 E2E Tests Passed successfully!")


if __name__ == "__main__":
    test_m6_suite()
