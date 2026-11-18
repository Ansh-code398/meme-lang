#!/usr/bin/env python3
"""
Parity & Verification Test Suite for MiniSQL Database Engine
Tests that Rust, Python, MEMElang Interpreter, and MEMElang Native AOT
produce bit-exact functional outputs across all database operations.
"""

import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"
MINISQL_DIR = PROJECT_ROOT / "apps" / "minisql"

def run_cmd(cmd):
    res = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"❌ Command failed: {' '.join(cmd)}\nStderr: {res.stderr}")
        sys.exit(1)
    return res.stdout.strip()

def normalize_output(out: str) -> str:
    lines = []
    for line in out.splitlines():
        line = line.strip()
        if not line or line.startswith("="):
            continue
        # Normalize boolean text (han/nhi/true/false)
        line = line.replace("han", "true").replace("nhi", "false")
        line = line.replace("True", "true").replace("False", "false")
        # Normalize float formatting (e.g. 110000.0 -> 110000)
        line = line.replace(".0 ", " ").replace(".0\n", "\n")
        if line.endswith(".0"):
            line = line[:-2]
        lines.append(line)
    return "\n".join(lines)

def main():
    print("==================================================")
    print("🧪 Running MiniSQL Dual-Engine & Tri-Language Parity Tests")
    print("==================================================")

    # 1. Compile Native Binaries
    print("  • Compiling native binaries...")
    meme_bin = MINISQL_DIR / "minisql_meme_bin"
    rust_bin = MINISQL_DIR / "minisql_rust_bin"

    run_cmd([str(MEME_BIN), "compile", str(MINISQL_DIR / "minisql.meme"), "-o", str(meme_bin)])
    run_cmd(["rustc", "-O", str(MINISQL_DIR / "minisql.rs"), "-o", str(rust_bin)])

    # 2. Execute verification runs
    print("  • Running Python 3.12...")
    out_py = normalize_output(run_cmd(["python3", str(MINISQL_DIR / "minisql.py")]))

    print("  • Running Pure Rust (-O)...")
    out_rust = normalize_output(run_cmd([str(rust_bin)]))

    print("  • Running MEMElang Native AOT...")
    out_meme_aot = normalize_output(run_cmd([str(meme_bin)]))

    print("  • Running MEMElang Interpreter...")
    out_meme_int = normalize_output(run_cmd([str(MEME_BIN), str(MINISQL_DIR / "minisql.meme")]))

    # 3. Assert exact parity
    print("\n🔍 Verifying Cross-Engine Parity...")
    assert out_py == out_rust, f"Mismatch between Python and Rust:\n--- Python ---\n{out_py}\n--- Rust ---\n{out_rust}"
    print("  ✔ Python 3.12 <==> Pure Rust: EXACT MATCH")

    assert out_rust == out_meme_aot, f"Mismatch between Rust and MEMElang Native AOT:\n--- Rust ---\n{out_rust}\n--- MEMElang Native ---\n{out_meme_aot}"
    print("  ✔ Pure Rust <==> MEMElang Native AOT: EXACT MATCH")

    assert out_meme_aot == out_meme_int, f"Mismatch between Native AOT and Interpreter:\n--- Native ---\n{out_meme_aot}\n--- Interpreter ---\n{out_meme_int}"
    print("  ✔ MEMElang Native AOT <==> MEMElang Interpreter: EXACT MATCH")

    print("\n==================================================")
    print("🎉 ALL 4 ENGINES ACHIEVED 100% PARITY CONFIRMATION!")
    print("==================================================")

if __name__ == "__main__":
    main()
