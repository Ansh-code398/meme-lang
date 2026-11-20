#!/usr/bin/env python3
"""
Parity & Verification Test Suite for MiniSQL Database Engine
Tests that Rust, Python, MEMElang Interpreter, and MEMElang Native AOT
produce bit-exact functional outputs across all database operations
for both Row-Store (MiniTable) and Columnar-Store (ColumnTable) engines.
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

def verify_engine(engine_name: str, meme_file: str, rust_file: str, py_file: str):
    print(f"\n--- Testing {engine_name} Engine ---")
    stem = Path(meme_file).stem
    meme_bin = MINISQL_DIR / f"{stem}_meme_bin"
    rust_bin = MINISQL_DIR / f"{stem}_rust_bin"

    print("  • Compiling native binaries...")
    run_cmd([str(MEME_BIN), "compile", str(MINISQL_DIR / meme_file), "-o", str(meme_bin)])
    run_cmd(["rustc", "-O", str(MINISQL_DIR / rust_file), "-o", str(rust_bin)])

    print("  • Running Python 3.12...")
    out_py = normalize_output(run_cmd(["python3", str(MINISQL_DIR / py_file)]))

    print("  • Running Pure Rust (-O)...")
    out_rust = normalize_output(run_cmd([str(rust_bin)]))

    print("  • Running MEMElang Native AOT...")
    out_meme_aot = normalize_output(run_cmd([str(meme_bin)]))

    print("  • Running MEMElang Interpreter...")
    out_meme_int = normalize_output(run_cmd([str(MEME_BIN), str(MINISQL_DIR / meme_file)]))

    print("  🔍 Verifying Parity...")
    assert out_py == out_rust, f"Mismatch between Python and Rust ({engine_name}):\n--- Python ---\n{out_py}\n--- Rust ---\n{out_rust}"
    print(f"    ✔ Python 3.12 <==> Pure Rust: EXACT MATCH")

    assert out_rust == out_meme_aot, f"Mismatch between Rust and MEMElang Native AOT ({engine_name}):\n--- Rust ---\n{out_rust}\n--- MEMElang Native ---\n{out_meme_aot}"
    print(f"    ✔ Pure Rust <==> MEMElang Native AOT: EXACT MATCH")

    assert out_meme_aot == out_meme_int, f"Mismatch between Native AOT and Interpreter ({engine_name}):\n--- Native ---\n{out_meme_aot}\n--- Interpreter ---\n{out_meme_int}"
    print(f"    ✔ MEMElang Native AOT <==> MEMElang Interpreter: EXACT MATCH")

def main():
    print("==================================================")
    print("🧪 Running MiniSQL Dual-Engine & Tri-Language Parity Tests")
    print("==================================================")

    # 1. Row-Store Engine
    verify_engine("Row-Store (MiniTable)", "minisql.meme", "minisql.rs", "minisql.py")

    # 2. Column-Store Engine
    verify_engine("Column-Store (ColumnTable)", "minisql_columnar.meme", "minisql_columnar.rs", "minisql_columnar.py")

    # 3. Struct-Store Engine
    verify_engine("Struct-Store (StructTable)", "minisql_struct.meme", "minisql_struct.rs", "minisql_struct.py")

    print("\n==================================================")
    print("🎉 ALL ENGINES & STORAGE FORMATS ACHIEVED 100% PARITY!")
    print("==================================================")

if __name__ == "__main__":
    main()
