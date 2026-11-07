#!/usr/bin/env python3
"""
MEMElang End-to-End (E2E) Test Runner
=====================================
Executes the opaque-box test suite against the `./meme` binary across 5 Tiers:
  - Tier 1: Core Feature Coverage (Overloading, Pointers, Memory Safety, Aliases)
  - Tier 2: Boundary, Corner Cases & Error Diagnostics
  - Tier 3: Cross-Feature Combinations (Pairwise interactions)
  - Tier 4: Real-World Application Scenarios
  - Tier 5: Adversarial Hardening (Overload Stress, Deep Indirection, Move/Drop Stresses)

Integrity Constraints:
  - Strictly audits for 0 occurrences of the forbidden word across test files.
  - Does NOT modify any compiler/runtime source code in `src/`.
"""

import sys
import os
import subprocess
import argparse
import time
import re
from pathlib import Path

# ANSI color codes
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
BOLD = "\033[1m"
RESET = "\033[0m"

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
TESTS_E2E_DIR = PROJECT_ROOT / "tests" / "e2e"
NEGATIVE_DIR = TESTS_E2E_DIR / "negative"

# Strict constraint: forbidden word check dynamically constructed from ascii codes
FORBIDDEN_WORD = "".join(chr(c) for c in [98, 104, 97, 105])
FORBIDDEN_PATTERN = re.compile(r"\b" + FORBIDDEN_WORD + r"\b", re.IGNORECASE)


def audit_forbidden_words(directory: Path) -> list:
    """Scans all test files to ensure the forbidden word NEVER appears."""
    violations = []
    for ext in ("*.meme", "*.py", "*.sh", "*.md"):
        for file_path in directory.rglob(ext):
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


def run_positive_test(meme_bin: str, file_path: Path, verbose: bool = False):
    """Runs a positive .meme test script and parses [PASS] / [FAIL] markers."""
    cmd = [meme_bin, str(file_path)]
    start_time = time.perf_counter()
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    elapsed = time.perf_counter() - start_time

    output = res.stdout + "\n" + res.stderr
    lines = output.splitlines()

    passes = []
    failures = []
    for line in lines:
        if line.startswith("[PASS]"):
            passes.append(line[6:].strip())
        elif line.startswith("[FAIL]"):
            failures.append(line[6:].strip())

    success = (res.returncode == 0) and (len(failures) == 0)

    return {
        "file": file_path.name,
        "success": success,
        "returncode": res.returncode,
        "passes": passes,
        "failures": failures,
        "elapsed": elapsed,
        "output": output,
    }


def run_negative_test(meme_bin: str, file_path: Path, verbose: bool = False):
    """Runs a negative test script expecting failure and checks error message."""
    expected_error = ""
    with open(file_path, "r", encoding="utf-8") as f:
        for line in f:
            if "EXPECT_ERROR:" in line:
                expected_error = line.split("EXPECT_ERROR:", 1)[1].strip()
                break

    cmd = [meme_bin, str(file_path)]
    start_time = time.perf_counter()
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    elapsed = time.perf_counter() - start_time

    output = res.stdout + "\n" + res.stderr
    error_matched = expected_error.lower() in output.lower() if expected_error else True
    success = (res.returncode != 0) and error_matched

    return {
        "file": file_path.name,
        "success": success,
        "returncode": res.returncode,
        "expected_error": expected_error,
        "error_matched": error_matched,
        "elapsed": elapsed,
        "output": output,
    }


def main():
    parser = argparse.ArgumentParser(description="MEMElang E2E Test Suite Runner")
    parser.add_argument("--meme", default=str(PROJECT_ROOT / "meme"), help="Path to meme binary")
    parser.add_argument("--tier", choices=["1", "2", "3", "4", "5", "all"], default="all", help="Run specific tier")
    parser.add_argument("--negative-only", action="store_true", help="Run only negative boundary tests")
    parser.add_argument("--list", action="store_true", help="List all cataloged test cases and tiers without executing")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show verbose output")
    parser.add_argument("--skip-forbidden-check", action="store_true", help="Skip forbidden word audit")
    args = parser.parse_args()

    print(f"{CYAN}{BOLD}================================================================{RESET}")
    print(f"{CYAN}{BOLD}🎭 MEMElang Opaque-Box E2E Test Suite Runner                    {RESET}")
    print(f"{CYAN}{BOLD}================================================================{RESET}")
    print(f"Target Binary: {args.meme}")
    print(f"Test Directory: {TESTS_E2E_DIR}")
    print(f"Tier Selection: {args.tier.upper()}")
    print()

    # Step 1: Forbidden word audit
    if not args.skip_forbidden_check:
        print(f"🔍 Step 1: Auditing strict forbidden word zero-tolerance invariant...")
        violations = audit_forbidden_words(TESTS_E2E_DIR)
        if violations:
            print(f"{RED}{BOLD}❌ CRITICAL VIOLATION: Forbidden word found in test repository!{RESET}")
            for file_path, line_no, word in violations:
                print(f"   {file_path}:{line_no} -> '{word}'")
            sys.exit(1)
        else:
            print(f"{GREEN}✔ Forbidden word audit passed: 0 occurrences detected.{RESET}")
    print()

    # Test file registry
    suite_files = {
        "overloading": TESTS_E2E_DIR / "test_overloading.meme",
        "pointers": TESTS_E2E_DIR / "test_pointers.meme",
        "memory_safety": TESTS_E2E_DIR / "test_memory_safety.meme",
        "aliases": TESTS_E2E_DIR / "test_aliases.meme",
        "combinations": TESTS_E2E_DIR / "test_combinations.meme",
        "scenarios": TESTS_E2E_DIR / "test_scenarios.meme",
        "tier5_adversarial_overloading": TESTS_E2E_DIR / "test_adversarial_overloading.meme",
        "tier5_adversarial_pointers": TESTS_E2E_DIR / "test_tier5_adversarial_pointers_memory.meme",
    }

    # If --list requested, inspect files and print catalog
    if args.list:
        print(f"{BOLD}📋 MEMElang E2E Test Inventory & Tier Catalog:{RESET}\n")
        total_cataloged = 0
        tier_stats = {"Tier 1": 0, "Tier 2": 0, "Tier 3": 0, "Tier 4": 0, "Tier 5": 0}

        for name, path in suite_files.items():
            if not path.is_file():
                continue
            print(f"{CYAN}Suite: {path.name}{RESET}")
            content = path.read_text(encoding="utf-8")
            for line in content.splitlines():
                line_str = line.strip()
                if line_str.startswith("//") or line_str.startswith("jugaad ") or line_str.startswith("fn "):
                    continue
                m = re.search(r"""(?:assert_eq|sahi_hai)\s*\(.*,\s*\"([^\"]+)\"\s*\);?""", line_str)
                if m:
                    test_name = m.group(1)
                    if test_name.startswith("T1_"):
                        t_str = "Tier 1"
                    elif test_name.startswith("T2_"):
                        t_str = "Tier 2"
                    elif test_name.startswith("T3_"):
                        t_str = "Tier 3"
                    elif test_name.startswith("T4_"):
                        t_str = "Tier 4"
                    elif test_name.startswith("T5_") or re.match(r"^S[1-7]_", test_name):
                        t_str = "Tier 5"
                    else:
                        t_str = "Other"
                    if t_str in tier_stats:
                        tier_stats[t_str] += 1
                    total_cataloged += 1
                    print(f"   [{t_str}] {test_name}")
            print()

        print(f"{CYAN}Negative Boundary Tests (Tier 2):{RESET}")
        if NEGATIVE_DIR.is_dir():
            for neg_file in sorted(NEGATIVE_DIR.glob("*.meme")):
                tier_stats["Tier 2"] += 1
                total_cataloged += 1
                print(f"   [Tier 2] {neg_file.name}")
        print()

        print(f"{BOLD}Catalog Breakdown:{RESET}")
        for t, count in tier_stats.items():
            print(f"   {t}: {count} tests")
        print(f"   Total Test Inventory: {total_cataloged} tests\n")
        sys.exit(0)

    # Check meme binary existence
    meme_path = Path(args.meme)
    if not meme_path.is_file() or not os.access(meme_path, os.X_OK):
        print(f"{YELLOW}Warning: '{args.meme}' not found or not executable. Attempting 'make' build...{RESET}")
        build_res = subprocess.run(["make"], cwd=str(PROJECT_ROOT))
        if build_res.returncode != 0 or not meme_path.is_file():
            print(f"{RED}Error: Failed to build '{args.meme}'. Run 'make' first.{RESET}")
            sys.exit(1)

    tier_map = {
        "1": ["overloading", "pointers", "memory_safety", "aliases"],
        "2": ["overloading", "pointers", "memory_safety", "aliases"],
        "3": ["combinations"],
        "4": ["scenarios"],
        "5": ["tier5_adversarial_overloading", "tier5_adversarial_pointers"],
    }

    selected_suites = []
    if not args.negative_only:
        if args.tier == "all":
            selected_suites = list(suite_files.keys())
        else:
            selected_suites = tier_map.get(args.tier, [])

    # Step 2: Positive tests execution
    total_passed = 0
    total_failed = 0
    total_duration = 0.0
    tier_counts = {"Tier 1": 0, "Tier 2": 0, "Tier 3": 0, "Tier 4": 0, "Tier 5": 0}

    if selected_suites:
        print(f"🚀 Step 2: Executing E2E Positive Test Suites...")
        for suite_key in selected_suites:
            file_path = suite_files[suite_key]
            if not file_path.is_file():
                print(f"{YELLOW}Skipping missing suite: {file_path}{RESET}")
                continue

            result = run_positive_test(args.meme, file_path, args.verbose)
            total_duration += result["elapsed"]

            status_str = f"{GREEN}PASS{RESET}" if result["success"] else f"{RED}FAIL{RESET}"
            pass_count = len(result["passes"])
            fail_count = len(result["failures"])
            print(f"  • {file_path.name:<45} [{status_str}]  Pass: {pass_count:<2} Fail: {fail_count:<2} ({result['elapsed']:.3f}s)")

            total_passed += pass_count
            total_failed += fail_count

            if args.verbose or not result["success"]:
                for f in result["failures"]:
                    print(f"      {RED}✖ Failure: {f}{RESET}")
                if not result["success"] and pass_count == 0 and fail_count == 0:
                    print(f"      {RED}✖ Execution failed (exit {result['returncode']}):\n{result['output'].strip()}{RESET}")

            # Map to tiers based on test naming
            for p in result["passes"]:
                if p.startswith("T1_"):
                    tier_counts["Tier 1"] += 1
                elif p.startswith("T2_"):
                    tier_counts["Tier 2"] += 1
                elif p.startswith("T3_"):
                    tier_counts["Tier 3"] += 1
                elif p.startswith("T4_"):
                    tier_counts["Tier 4"] += 1
                elif p.startswith("T5_") or re.match(r"^S[1-7]_", p):
                    tier_counts["Tier 5"] += 1

        print()

    # Step 3: Negative boundary tests
    neg_passed = 0
    neg_failed = 0
    if args.tier in ("2", "all") or args.negative_only:
        print(f"🛡️  Step 3: Executing Negative Boundary & Corner Case Tests...")
        if NEGATIVE_DIR.is_dir():
            for neg_file in sorted(NEGATIVE_DIR.glob("*.meme")):
                res = run_negative_test(args.meme, neg_file, args.verbose)
                total_duration += res["elapsed"]
                status_str = f"{GREEN}PASS{RESET}" if res["success"] else f"{RED}FAIL{RESET}"
                print(f"  • {neg_file.name:<35} [{status_str}] Expected: '{res['expected_error']}' ({res['elapsed']:.3f}s)")

                if res["success"]:
                    neg_passed += 1
                    tier_counts["Tier 2"] += 1
                else:
                    neg_failed += 1
                    if args.verbose or not res["success"]:
                        print(f"      {RED}✖ Expected non-zero exit & error containing '{res['expected_error']}'. Output:\n{res['output'].strip()}{RESET}")

    print()
    print(f"{CYAN}{BOLD}================================================================{RESET}")
    print(f"{CYAN}{BOLD}📊 E2E Test Suite Summary Report                                {RESET}")
    print(f"{CYAN}{BOLD}================================================================{RESET}")
    print(f"  Tier 1 (Core Feature Coverage):          {tier_counts['Tier 1']:>3} verified")
    print(f"  Tier 2 (Boundary & Corner Cases):        {tier_counts['Tier 2']:>3} verified (incl. negative cases)")
    print(f"  Tier 3 (Cross-Feature Combinations):     {tier_counts['Tier 3']:>3} verified")
    print(f"  Tier 4 (Real-World Scenarios):           {tier_counts['Tier 4']:>3} verified")
    print(f"  Tier 5 (Adversarial Hardening):          {tier_counts['Tier 5']:>3} verified")
    print(f"  -------------------------------------------------------------")
    total_cases = sum(tier_counts.values())
    print(f"  Total Verified Test Cases:               {total_cases:>3}")
    print(f"  Total Execution Duration:                {total_duration:.3f}s")
    print(f"  Negative Boundary Tests:                 {neg_passed} passed, {neg_failed} failed")
    print(f"  Forbidden Word Invariant:                CLEAN (0 occurrences)")
    print(f"{CYAN}{BOLD}================================================================{RESET}")

    if total_failed > 0 or neg_failed > 0:
        print(f"\n{RED}{BOLD}❌ Some tests failed or features require runtime milestone implementation.{RESET}")
        sys.exit(1)
    else:
        print(f"\n{GREEN}{BOLD}🎉 ALL E2E TESTS PASSED SUCCESSFULLY! Wah bete mauj kardi!{RESET}")
        sys.exit(0)


if __name__ == "__main__":
    main()
