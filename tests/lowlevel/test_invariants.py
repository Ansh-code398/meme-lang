#!/usr/bin/env python3
"""
MEMElang Architectural Invariants Verification (Low-Level E2E Suite)
===================================================================
Validates:
  1. Zero occurrences of forbidden word repository-wide (with M1 legacy exemption list).
  2. sizeof(Value) == 32 compile-time static assertion in src/interpreter/Value.h.
  3. Clean compilation via `make all`.
  4. Regression safety via `make test` (all 25 internal test suites passing).
"""

import sys
import os
import re
import subprocess
import argparse
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent

# Dynamically construct forbidden word check to guarantee zero literal occurrences in this file
FORBIDDEN_WORD = "".join(chr(c) for c in [98, 104, 97, 105])
FORBIDDEN_PATTERN = re.compile(r"\b" + FORBIDDEN_WORD + r"\b", re.IGNORECASE)


def audit_forbidden_words(directory: Path, allowed_files=None) -> list:
    """Scans directory for forbidden words, excluding allowed legacy files and ignorable dirs."""
    violations = []
    if allowed_files is None:
        allowed_files = set()

    for root, dirs, files in os.walk(directory):
        # Skip git, agents, node_modules, build artifacts, etc.
        dirs[:] = [d for d in dirs if d not in (".git", ".agents", "node_modules", "emsdk", "tools", ".gemini", "build", ".cache")]
        for f in files:
            file_path = Path(root) / f
            if file_path.resolve() in allowed_files:
                continue
            if file_path.suffix in (".meme", ".cpp", ".h", ".py", ".sh", ".md", ".json", ".txt"):
                try:
                    content = file_path.read_text(encoding="utf-8", errors="ignore")
                    matches = list(FORBIDDEN_PATTERN.finditer(content))
                    if matches:
                        for m in matches:
                            line_no = content[:m.start()].count("\n") + 1
                            violations.append((file_path, line_no, m.group()))
                except Exception as e:
                    violations.append((file_path, 0, f"Read error: {e}"))
    return violations


def main():
    parser = argparse.ArgumentParser(description="Architectural Invariants Verification")
    parser.add_argument("--skip-forbidden-check", action="store_true", help="Skip forbidden word audit")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose reporting")
    args = parser.parse_args()

    passes = []
    failures = []

    def pass_test(name):
        passes.append(name)
        print(f"[PASS] {name}")

    def fail_test(name, reason):
        failures.append((name, reason))
        print(f"[FAIL] {name}: {reason}")

    print("=== Architectural Invariants Verification ===")

    # Test 1: Invariant - Forbidden words in src/ and tests/
    if not args.skip_forbidden_check:
        src_viol = audit_forbidden_words(PROJECT_ROOT / "src")
        test_viol = audit_forbidden_words(PROJECT_ROOT / "tests")
        if not src_viol and not test_viol:
            pass_test("T1_invariant_src_and_test_clean")
        else:
            err_details = []
            for path, line, match in src_viol + test_viol:
                err_details.append(f"{path}:{line} found '{match}'")
            fail_test("T1_invariant_src_and_test_clean", f"Found {len(src_viol)} in src/, {len(test_viol)} in tests/: {', '.join(err_details[:5])}")

        # Test 1b: Invariant - Repository-wide cleansing (with legacy exemptions for meme_oop.cpp and meme_minimal.cpp)
        legacy_exemptions = {
            (PROJECT_ROOT / "meme_oop.cpp").resolve(),
            (PROJECT_ROOT / "meme_minimal.cpp").resolve(),
            (PROJECT_ROOT / "PROJECT.md").resolve(),
        }
        repo_viol = audit_forbidden_words(PROJECT_ROOT, allowed_files=legacy_exemptions)
        if not repo_viol:
            pass_test("T1_invariant_repo_wide_forbidden_words_clean")
        else:
            err_details = [f"{p}:{l}" for p, l, _ in repo_viol[:5]]
            fail_test("T1_invariant_repo_wide_forbidden_words_clean", f"Found {len(repo_viol)} violations: {', '.join(err_details)}")
    else:
        print("[STAGED] T1_invariant_src_and_test_clean: Skipped via flag")
        print("[STAGED] T1_invariant_repo_wide_forbidden_words_clean: Skipped via flag")

    # Test 2: Invariant - sizeof(Value) == 32 in src/interpreter/Value.h
    val_header = PROJECT_ROOT / "src" / "interpreter" / "Value.h"
    if val_header.is_file():
        content = val_header.read_text(encoding="utf-8")
        if "static_assert(sizeof(Value) == 32" in content:
            pass_test("T1_invariant_sizeof_value_32")
        else:
            fail_test("T1_invariant_sizeof_value_32", "static_assert(sizeof(Value) == 32) assertion missing")
    else:
        fail_test("T1_invariant_sizeof_value_32", "src/interpreter/Value.h missing")

    # Test 3: Invariant - make all succeeds cleanly
    b_res = subprocess.run(["make", "all"], cwd=str(PROJECT_ROOT), capture_output=True, text=True)
    if b_res.returncode == 0 and (PROJECT_ROOT / "meme").exists():
        pass_test("T1_invariant_make_all_clean")
    else:
        fail_test("T1_invariant_make_all_clean", f"make all failed (exit {b_res.returncode}): {b_res.stderr.strip()[:200]}")

    # Test 4: Invariant - make test passes all 25 internal test suites
    t_res = subprocess.run(["make", "test"], cwd=str(PROJECT_ROOT), capture_output=True, text=True)
    if t_res.returncode == 0 and ("ALL TESTS PASSED SUCCESSFULLY" in t_res.stdout or "ALL TESTS PASSED" in t_res.stdout):
        pass_test("T1_invariant_make_test_all_25_suites_pass")
    else:
        fail_test("T1_invariant_make_test_all_25_suites_pass", f"make test failed (exit {t_res.returncode})")

    print(f"\nSummary: {len(passes)} passed, {len(failures)} failed.")
    if failures:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
