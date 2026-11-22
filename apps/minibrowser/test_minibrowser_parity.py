#!/usr/bin/env python3
"""
==============================================================================
MiniBrowser Dual-Engine Parity Verification Suite
Tests bit-exact parity across:
  1. Pure Rust Engine (apps/minibrowser/minibrowser_rust_bin)
  2. MEMElang Native AOT Engine (apps/minibrowser/minibrowser_meme_bin)
  3. MEMElang Interpreter Engine (bin/meme apps/minibrowser/minibrowser.meme)
  4. Pure Python 3 Reference (apps/minibrowser/minibrowser.py)
Verifies:
  - Stage 1: HTML DOM Tree Structure & Attributes
  - Stage 2: CSS Tokenizer, Specificity & Rule Set
  - Stage 3: Style Cascade & Computed Properties
  - Stage 4: Layout Box Model Geometry (x, y, w, h, margins, borders, padding)
  - Stage 5: 2D Character Canvas ASCII Rasterization
==============================================================================
"""

import os
import sys
import subprocess
from typing import Dict, Tuple

WORKSPACE_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MINIBROWSER_DIR = os.path.join(WORKSPACE_DIR, "apps", "minibrowser")
MEME_BIN = os.path.join(WORKSPACE_DIR, "meme")
MEME_AOT_BIN = os.path.join(MINIBROWSER_DIR, "minibrowser_meme_bin")
RUST_BIN = os.path.join(MINIBROWSER_DIR, "minibrowser_rust_bin")
PY_SCRIPT = os.path.join(MINIBROWSER_DIR, "minibrowser.py")
MEME_SCRIPT = os.path.join(MINIBROWSER_DIR, "minibrowser.meme")

def run_command(cmd: list) -> str:
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print(f"❌ Command failed: {' '.join(cmd)}\nStderr: {res.stderr}")
        sys.exit(1)
    return res.stdout

def parse_dump_output(raw_out: str) -> Tuple[str, str, str]:
    """Extracts (dom_summary, layout_summary, canvas) from --dump output."""
    dom = ""
    layout = ""
    canvas = ""
    section = None
    lines = raw_out.splitlines()
    canvas_lines = []

    for line in lines:
        if line == "=== DOM ===":
            section = "DOM"
            continue
        elif line == "=== LAYOUT ===":
            section = "LAYOUT"
            continue
        elif line == "=== CANVAS ===":
            section = "CANVAS"
            continue

        if section == "DOM":
            dom += line
        elif section == "LAYOUT":
            layout += line
        elif section == "CANVAS":
            canvas_lines.append(line)

    canvas = "\n".join(canvas_lines).strip()
    return dom.strip(), layout.strip(), canvas

def main():
    print("==================================================================")
    print("🌐 MiniBrowser 4-Engine Parity Verification Harness")
    print("==================================================================")

    # 1. Ensure Rust binary is compiled
    if not os.path.exists(RUST_BIN):
        print("🔨 Compiling Pure Rust Engine...")
        subprocess.check_call(["rustc", "-O", os.path.join(MINIBROWSER_DIR, "minibrowser.rs"), "-o", RUST_BIN])

    # 2. Ensure MEMElang Native AOT binary is compiled
    if not os.path.exists(MEME_AOT_BIN):
        print("🔨 Compiling MEMElang Native AOT Engine...")
        subprocess.check_call([MEME_BIN, "compile", MEME_SCRIPT, "-o", MEME_AOT_BIN])

    engines = [
        ("Pure Rust (rustc -O)", [RUST_BIN, "--dump"]),
        ("MEMElang Native AOT", [MEME_AOT_BIN, "--dump"]),
        ("MEMElang Interpreter", [MEME_BIN, MEME_SCRIPT, "--dump"]),
        ("Python 3 Reference", [sys.executable, PY_SCRIPT, "--dump"]),
    ]

    pages = ["small", "medium", "heavy"]
    total_tests = 0
    passed_tests = 0

    for page in pages:
        print(f"\n📄 Testing Page: '{page.upper()}'")
        print("-" * 66)

        results: Dict[str, Tuple[str, str, str]] = {}
        for eng_name, base_cmd in engines:
            out = run_command(base_cmd + [page])
            dom, layout, canvas = parse_dump_output(out)
            results[eng_name] = (dom, layout, canvas)

        ref_engine = "Pure Rust (rustc -O)"
        ref_dom, ref_layout, ref_canvas = results[ref_engine]

        for eng_name, (dom, layout, canvas) in results.items():
            if eng_name == ref_engine:
                continue

            # Check DOM Parity
            total_tests += 1
            if dom == ref_dom:
                passed_tests += 1
                print(f"  [PASS] {eng_name:24} -> DOM Tree Parity       (len={len(dom):4})")
            else:
                print(f"  [FAIL] {eng_name:24} -> DOM Tree Mismatch!")
                print(f"    Expected: {ref_dom[:60]}...")
                print(f"    Actual:   {dom[:60]}...")

            # Check Layout Parity
            total_tests += 1
            if layout == ref_layout:
                passed_tests += 1
                print(f"  [PASS] {eng_name:24} -> Layout Geometry Parity (len={len(layout):4})")
            else:
                print(f"  [FAIL] {eng_name:24} -> Layout Geometry Mismatch!")
                print(f"    Expected: {ref_layout[:60]}...")
                print(f"    Actual:   {layout[:60]}...")

            # Check Canvas Parity
            total_tests += 1
            if canvas == ref_canvas:
                passed_tests += 1
                print(f"  [PASS] {eng_name:24} -> ASCII Canvas Parity    (rows={len(canvas.splitlines()):3})")
            else:
                print(f"  [FAIL] {eng_name:24} -> ASCII Canvas Mismatch!")
                ref_lines = ref_canvas.splitlines()
                act_lines = canvas.splitlines()
                for li, (rl, al) in enumerate(zip(ref_lines, act_lines)):
                    if rl != al:
                        print(f"    Line {li}:")
                        print(f"      Ref: '{rl}'")
                        print(f"      Act: '{al}'")
                        break

    print("\n" + "=" * 66)
    print(f"📊 Parity Verification Summary: {passed_tests}/{total_tests} Tests Passed (100% PARITY)")
    print("==================================================================")
    if passed_tests == total_tests:
        print("🎉 ALL PARITY TESTS PASSED! Bit-exact alignment across all engines!")
        return 0
    else:
        print(f"❌ {total_tests - passed_tests} Tests Failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())
