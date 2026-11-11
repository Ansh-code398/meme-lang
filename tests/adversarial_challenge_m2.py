#!/usr/bin/env python3
"""
MEMElang Adversarial Stress Harness - Milestone 2 Subheading 2
==============================================================
Empirical adversarial tests for:
  - JaanchExpr list destructuring & rest patterns (...tail, *tail, anonymous rest, nested)
  - Pattern guards (falsy, exceptions, isolation, shadowing)
  - Deep namespaces (A::B::C::D, mutation, cross-namespace, reopening)
  - tapri.json and meme build edge cases (malformed JSON, spaces in paths, nested dirs, escaped quotes)

Zero occurrences of forbidden word enforced.
"""

import os
import sys
import json
import tempfile
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
MEME_BIN = PROJECT_ROOT / "meme"

passes = []
failures = []

def record_pass(name):
    passes.append(name)
    print(f"  [PASS] {name}")

def record_fail(name, reason):
    failures.append((name, reason))
    print(f"  [FAIL] {name}: {reason}")

def run_meme_code(code, timeout=15):
    res = subprocess.run(
        [str(MEME_BIN), "-e", code],
        cwd=str(PROJECT_ROOT),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=timeout
    )
    return res

def run_command(cmd, cwd=None, timeout=30):
    res = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else str(PROJECT_ROOT),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=timeout
    )
    return res


print("=================================================================")
print("🧪 ADVERSARIAL STRESS SUITE: M2 CHALLENGER 2")
print("=================================================================")

# ----------------------------------------------------------------------
# 1. JaanchExpr with Complex List Destructuring & Rest Patterns
# ----------------------------------------------------------------------
print("\n--- 1. List Destructuring & Rest Patterns ---")

# 1.1 Empty list matches []
res = run_meme_code('let r = jaanch [] { [] => "empty", _ => "other" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "empty":
    record_pass("T1_list_empty_matches_empty")
else:
    record_fail("T1_list_empty_matches_empty", f"Got: {res.stdout.strip()} (err: {res.stderr.strip()})")

# 1.2 Empty list does not match [x]
res = run_meme_code('let r = jaanch [] { [x] => "single", [] => "empty" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "empty":
    record_pass("T1_list_empty_rejects_single_element_pattern")
else:
    record_fail("T1_list_empty_rejects_single_element_pattern", f"Got: {res.stdout.strip()}")

# 1.3 Single element list matches [x]
res = run_meme_code('let r = jaanch [42] { [x] => x * 2, _ => 0 }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "84":
    record_pass("T1_list_single_element_matches")
else:
    record_fail("T1_list_single_element_matches", f"Got: {res.stdout.strip()}")

# 1.4 Single element list rejects [a, b]
res = run_meme_code('let r = jaanch [42] { [a, b] => "two", [x] => "one" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "one":
    record_pass("T1_list_single_rejects_multi_pattern")
else:
    record_fail("T1_list_single_rejects_multi_pattern", f"Got: {res.stdout.strip()}")

# 1.5 Multi-element list matches [a, b]
res = run_meme_code('let r = jaanch [10, 20] { [a, b] => a + b, _ => 0 }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "30":
    record_pass("T1_list_multi_element_matches")
else:
    record_fail("T1_list_multi_element_matches", f"Got: {res.stdout.strip()}")

# 1.6 Multi-element length mismatch rejects [a, b] on length 3
res = run_meme_code('let r = jaanch [10, 20, 30] { [a, b] => "two", [a, b, c] => "three" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "three":
    record_pass("T1_list_multi_element_rejects_length_mismatch")
else:
    record_fail("T1_list_multi_element_rejects_length_mismatch", f"Got: {res.stdout.strip()}")

# 1.7 Rest pattern [head, ...tail] on multi-element list
res = run_meme_code('let r = jaanch [1, 2, 3, 4] { [head, ...tail] => [head, tail], _ => "none" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "[1, [2, 3, 4]]":
    record_pass("T1_list_rest_pattern_multi_tail")
else:
    record_fail("T1_list_rest_pattern_multi_tail", f"Got: {res.stdout.strip()}")

# 1.8 Empty rest: [a, ...tail] on 1-item list produces tail == []
res = run_meme_code('let r = jaanch [99] { [a, ...tail] => [a, tail], _ => "none" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "[99, []]":
    record_pass("T1_list_rest_pattern_empty_tail")
else:
    record_fail("T1_list_rest_pattern_empty_tail", f"Got: {res.stdout.strip()}")

# 1.9 Rest pattern rejects when target has fewer elements than prefix
res = run_meme_code('let r = jaanch [1, 2] { [a, b, c, ...tail] => "match", _ => "fallback" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "fallback":
    record_pass("T1_list_rest_pattern_insufficient_prefix_rejects")
else:
    record_fail("T1_list_rest_pattern_insufficient_prefix_rejects", f"Got: {res.stdout.strip()}")

# 1.10 Bare rest pattern [...all] on [] and [1, 2]
res = run_meme_code('let r1 = jaanch [] { [...all] => all }; let r2 = jaanch [1, 2] { [...all] => all }; dikha([r1, r2]);')
if res.returncode == 0 and res.stdout.strip() == "[[], [1, 2]]":
    record_pass("T1_list_bare_rest_pattern")
else:
    record_fail("T1_list_bare_rest_pattern", f"Got: {res.stdout.strip()}")

# 1.11 Rest pattern with wildcard [..._]
res = run_meme_code('let r = jaanch [1, 2, 3] { [a, ..._] => a }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "1":
    record_pass("T1_list_rest_pattern_wildcard")
else:
    record_fail("T1_list_rest_pattern_wildcard", f"Got: {res.stdout.strip()}")

# 1.12 Nested list destructuring [[a, b], ...rest]
res = run_meme_code('let r = jaanch [[10, 20], 30, 40] { [[a, b], ...rest] => [a, b, rest], _ => "none" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "[10, 20, [30, 40]]":
    record_pass("T1_list_nested_list_destructuring")
else:
    record_fail("T1_list_nested_list_destructuring", f"Got: {res.stdout.strip()}")

# 1.13 Deeply nested list [a, [b, [c, [d]]]]
res = run_meme_code('let val = [1, [2, [3, [4]]]]; let r = jaanch val { [a, [b, [c, [d]]]] => a + b + c + d, _ => 0 }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "10":
    record_pass("T1_list_deeply_nested_destructuring")
else:
    record_fail("T1_list_deeply_nested_destructuring", f"Got: {res.stdout.strip()}")

# 1.14 Anonymous rest syntax [head, ...]
res = run_meme_code('let r = jaanch [10, 20, 30] { [head, ...] => head, _ => 0 }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "10":
    record_pass("T1_list_anonymous_rest_syntax")
else:
    record_fail("T1_list_anonymous_rest_syntax", f"Got: {res.stdout.strip()}")

# 1.15 Asterisk rest syntax [head, *tail]
res = run_meme_code('let r = jaanch [10, 20, 30] { [head, *tail] => tail, _ => [] }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "[20, 30]":
    record_pass("T1_list_asterisk_rest_syntax")
else:
    record_fail("T1_list_asterisk_rest_syntax", f"Got: {res.stdout.strip()}")

# 1.16 Variant wrapping list destructuring: Some([head, ...tail])
res = run_meme_code('let r = jaanch Some([10, 20, 30]) { Some([head, ...tail]) => head + tail[0], _ => 0 }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "30":
    record_pass("T1_list_variant_wrapped_list_destructuring")
else:
    record_fail("T1_list_variant_wrapped_list_destructuring", f"Got: {res.stdout.strip()}")

# 1.17 Non-list matching against list patterns
res = run_meme_code('''
let r1 = jaanch 42 { [] => "empty", [a, ...tail] => "list", _ => "non-list" };
let r2 = jaanch "hello" { [x] => "list", _ => "non-list" };
let r3 = jaanch true { [x] => "list", _ => "non-list" };
let r4 = jaanch None { [x] => "list", _ => "non-list" };
dikha([r1, r2, r3, r4]);
''')
if res.returncode == 0 and res.stdout.strip() == '["non-list", "non-list", "non-list", "non-list"]':
    record_pass("T1_list_non_list_matching_rejection")
else:
    record_fail("T1_list_non_list_matching_rejection", f"Got: {res.stdout.strip()}")

# 1.18 Non-exhaustive list match throws "Adhoora"
res = run_meme_code('try { jaanch [1, 2, 3] { [] => "empty", [x] => "single" }; } catch (e) { dikha("ERR:" + e); }')
if res.returncode == 0 and "Adhoora" in res.stdout:
    record_pass("T2_list_non_exhaustive_throws_adhoora")
else:
    record_fail("T2_list_non_exhaustive_throws_adhoora", f"Got: {res.stdout.strip()}")


# ----------------------------------------------------------------------
# 2. Pattern Guards
# ----------------------------------------------------------------------
print("\n--- 2. Pattern Guards ---")

# 2.1 Guard returning falsy falls through
res = run_meme_code('let r = jaanch 10 { x agar x > 20 => "high", x agar x > 5 => "mid", _ => "low" }; dikha(r);')
if res.returncode == 0 and res.stdout.strip() == "mid":
    record_pass("T1_guard_falsy_fallthrough")
else:
    record_fail("T1_guard_falsy_fallthrough", f"Got: {res.stdout.strip()}")

# 2.2 Guard throwing division-by-zero exception propagates
res = run_meme_code('try { jaanch 0 { x agar (100 / x > 0) => "div", _ => "fallback" }; } catch (e) { dikha("CAUGHT:" + e); }')
if res.returncode == 0 and "divide" in res.stdout.lower() and "zero" in res.stdout.lower():
    record_pass("T1_guard_div_zero_exception_propagates")
else:
    record_fail("T1_guard_div_zero_exception_propagates", f"Got: {res.stdout.strip()}")

# 2.3 Guard calling function that throws user error propagates
res = run_meme_code('fn blowup() { throw "GuardCustomError"; }; try { jaanch 5 { x agar blowup() => "yes", _ => "no" }; } catch (e) { dikha("CAUGHT:" + e); }')
if res.returncode == 0 and "GuardCustomError" in res.stdout:
    record_pass("T1_guard_user_throw_propagates")
else:
    record_fail("T1_guard_user_throw_propagates", f"Got: {res.stdout.strip()}")

# 2.4 Rejected arm bindings do not leak into subsequent arms
res = run_meme_code('''
fn testArmIsolation() {
    fn reject() { return false; };
    let r = jaanch [42] {
        [x] agar reject() => "arm1",
        [y] => {
            try {
                let z = x;
                return "leaked";
            } catch (e) {
                return "isolated";
            }
        }
    };
    return r;
};
dikha(testArmIsolation());
''')
if res.returncode == 0 and res.stdout.strip() == "isolated":
    record_pass("T1_guard_arm_bindings_isolated_from_subsequent_arms")
else:
    record_fail("T1_guard_arm_bindings_isolated_from_subsequent_arms", f"Got: {res.stdout.strip()}")

# 2.5 Matched arm bindings do not leak outside jaanch
res = run_meme_code('''
fn testOuterIsolation() {
    let r = jaanch [42] { [y] => y };
    try {
        let z = y;
        return "leaked";
    } catch (e) {
        return "isolated";
    }
};
dikha(testOuterIsolation());
''')
if res.returncode == 0 and res.stdout.strip() == "isolated":
    record_pass("T1_guard_matched_bindings_isolated_from_outer_scope")
else:
    record_fail("T1_guard_matched_bindings_isolated_from_outer_scope", f"Got: {res.stdout.strip()}")

# 2.6 Arm binding shadows outer variable without modifying it
res = run_meme_code('''
let x = 100;
let r = jaanch 200 {
    x agar x == 200 => x * 2,
    _ => 0
};
dikha(x);
dikha(r);
''')
lines = [l.strip() for l in res.stdout.strip().splitlines() if l.strip()]
if res.returncode == 0 and lines == ["100", "400"]:
    record_pass("T1_guard_arm_shadowing_preserves_outer_variable")
else:
    record_fail("T1_guard_arm_shadowing_preserves_outer_variable", f"Got: {lines}")


# ----------------------------------------------------------------------
# 3. Deep Namespaces & Scoped Access
# ----------------------------------------------------------------------
print("\n--- 3. Deep Namespaces & Scoped Access ---")

# 3.1 Deep 4-level namespace access: A::B::C::D::val
res = run_meme_code('''
namespace A {
    namespace B {
        namespace C {
            namespace D {
                let val = 42;
                fn add(x, y) { return x + y; };
            }
        }
    }
};
dikha(A::B::C::D::val);
dikha(A::B::C::D::add(10, 32));
''')
lines = [l.strip() for l in res.stdout.strip().splitlines() if l.strip()]
if res.returncode == 0 and lines == ["42", "42"]:
    record_pass("T1_ns_deep_4_level_access_and_call")
else:
    record_fail("T1_ns_deep_4_level_access_and_call", f"Got: {lines}")

# 3.2 Deep namespace scoped assignment: A::B::C::D::val = 123
res = run_meme_code('''
namespace A {
    namespace B {
        namespace C {
            namespace D {
                let val = 42;
            }
        }
    }
};
A::B::C::D::val = 123;
dikha(A::B::C::D::val);
''')
if res.returncode == 0 and res.stdout.strip() == "123":
    record_pass("T1_ns_deep_scoped_assignment")
else:
    record_fail("T1_ns_deep_scoped_assignment", f"Got: {res.stdout.strip()}")

# 3.3 Cross-namespace invocations
res = run_meme_code('''
namespace Alpha {
    let factor = 10;
    fn scale(x) { return x * factor; };
};
namespace Beta {
    fn compute(x) { return Alpha::scale(x) + 5; };
};
dikha(Beta::compute(4));
''')
if res.returncode == 0 and res.stdout.strip() == "45":
    record_pass("T1_ns_cross_namespace_invocation")
else:
    record_fail("T1_ns_cross_namespace_invocation", f"Got: {res.stdout.strip()}")

# 3.4 Namespace reopening preserves earlier declarations
res = run_meme_code('''
namespace MathBox {
    let base = 100;
};
namespace MathBox {
    let offset = 25;
    fn getTotal() { return base + offset; };
};
dikha(MathBox::getTotal());
''')
if res.returncode == 0 and res.stdout.strip() == "125":
    record_pass("T1_ns_reopening_preserves_prior_declarations")
else:
    record_fail("T1_ns_reopening_preserves_prior_declarations", f"Got: {res.stdout.strip()}")

# 3.5 Missing member in deep namespace throws "Mohalla"
res = run_meme_code('''
namespace A {
    namespace B {
        namespace C {
            namespace D {
                let x = 1;
            }
        }
    }
};
try {
    dikha(A::B::C::D::ghost);
} catch (e) {
    dikha("CAUGHT:" + e);
}
''')
if res.returncode == 0 and "Mohalla" in res.stdout:
    record_pass("T2_ns_missing_deep_member_throws_mohalla")
else:
    record_fail("T2_ns_missing_deep_member_throws_mohalla", f"Got: {res.stdout.strip()}")

# 3.6 Missing intermediate namespace throws "Mohalla"
res = run_meme_code('''
namespace A {
    namespace B {
        let x = 1;
    }
};
try {
    dikha(A::ghost::C);
} catch (e) {
    dikha("CAUGHT:" + e);
}
''')
if res.returncode == 0 and "Mohalla" in res.stdout:
    record_pass("T2_ns_missing_intermediate_namespace_throws_mohalla")
else:
    record_fail("T2_ns_missing_intermediate_namespace_throws_mohalla", f"Got: {res.stdout.strip()}")


# ----------------------------------------------------------------------
# 4. meme build & tapri.json Edge Cases
# ----------------------------------------------------------------------
print("\n--- 4. Build Tooling & tapri.json Edge Cases ---")

# 4.1 Output path with spaces: "bin/my app with spaces"
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    manifest = proj_dir / "tapri.json"
    manifest.write_text(json.dumps({
        "name": "spaced_output_project",
        "entry": "src/main.meme",
        "output": "bin/my app with spaces"
    }), encoding="utf-8")
    src_dir = proj_dir / "src"
    src_dir.mkdir()
    (src_dir / "main.meme").write_text('dikha "Spaced binary output ran!";\n', encoding="utf-8")

    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    out_bin = proj_dir / "bin" / "my app with spaces"
    if res.returncode == 0 and out_bin.is_file():
        exec_res = run_command([str(out_bin)], cwd=proj_dir)
        if exec_res.returncode == 0 and "Spaced binary output ran!" in exec_res.stdout:
            record_pass("T1_build_output_path_with_spaces")
        else:
            record_fail("T1_build_output_path_with_spaces", f"Execution failed: {exec_res.stdout} {exec_res.stderr}")
    else:
        record_fail("T1_build_output_path_with_spaces", f"Build failed: {res.stdout} {res.stderr}")

# 4.2 Deep nested output directory: "bin/nested/deep/app"
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    manifest = proj_dir / "tapri.json"
    manifest.write_text(json.dumps({
        "name": "nested_out_project",
        "entry": "src/main.meme",
        "output": "bin/nested/deep/app"
    }), encoding="utf-8")
    src_dir = proj_dir / "src"
    src_dir.mkdir()
    (src_dir / "main.meme").write_text('dikha "Nested dir binary output ran!";\n', encoding="utf-8")

    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    out_bin = proj_dir / "bin" / "nested" / "deep" / "app"
    if res.returncode == 0 and out_bin.is_file():
        exec_res = run_command([str(out_bin)], cwd=proj_dir)
        if exec_res.returncode == 0 and "Nested dir binary output ran!" in exec_res.stdout:
            record_pass("T1_build_deep_nested_output_dir")
        else:
            record_fail("T1_build_deep_nested_output_dir", f"Execution failed: {exec_res.stdout}")
    else:
        record_fail("T1_build_deep_nested_output_dir", f"Build failed: {res.stdout} {res.stderr}")

# 4.3 Nested source entrypoint: "src/deep/nested/main.meme"
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    manifest = proj_dir / "tapri.json"
    manifest.write_text(json.dumps({
        "name": "nested_src_project",
        "entry": "src/deep/nested/main.meme",
        "output": "bin/app"
    }), encoding="utf-8")
    nested_src = proj_dir / "src" / "deep" / "nested"
    nested_src.mkdir(parents=True)
    (nested_src / "main.meme").write_text('dikha "Nested source compiled and ran!";\n', encoding="utf-8")

    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    out_bin = proj_dir / "bin" / "app"
    if res.returncode == 0 and out_bin.is_file():
        exec_res = run_command([str(out_bin)], cwd=proj_dir)
        if exec_res.returncode == 0 and "Nested source compiled and ran!" in exec_res.stdout:
            record_pass("T1_build_nested_source_entrypoint")
        else:
            record_fail("T1_build_nested_source_entrypoint", f"Execution failed: {exec_res.stdout}")
    else:
        record_fail("T1_build_nested_source_entrypoint", f"Build failed: {res.stdout} {res.stderr}")

# 4.4 Output path with symbols: "bin/app-v1.0.0_release"
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    manifest = proj_dir / "tapri.json"
    manifest.write_text(json.dumps({
        "name": "symbol_project",
        "entry": "src/main.meme",
        "output": "bin/app-v1.0.0_release"
    }), encoding="utf-8")
    src_dir = proj_dir / "src"
    src_dir.mkdir()
    (src_dir / "main.meme").write_text('dikha "Symbol binary output ran!";\n', encoding="utf-8")

    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    out_bin = proj_dir / "bin" / "app-v1.0.0_release"
    if res.returncode == 0 and out_bin.is_file():
        exec_res = run_command([str(out_bin)], cwd=proj_dir)
        if exec_res.returncode == 0 and "Symbol binary output ran!" in exec_res.stdout:
            record_pass("T1_build_output_with_symbols")
        else:
            record_fail("T1_build_output_with_symbols", f"Execution failed: {exec_res.stdout}")
    else:
        record_fail("T1_build_output_with_symbols", f"Build failed: {res.stdout} {res.stderr}")

# 4.5 Complex manifest with nested dependencies and scripts
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    manifest = {
        "name": "complex_manifest",
        "version": "2.1.0",
        "dependencies": {
            "py:numpy": "^1.24",
            "nested": {"active": True}
        },
        "scripts": {
            "start": "meme run src/main.meme"
        },
        "entry": "src/main.meme",
        "output": "bin/complex_app"
    }
    (proj_dir / "tapri.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    src_dir = proj_dir / "src"
    src_dir.mkdir()
    (src_dir / "main.meme").write_text('dikha "Complex manifest compiled and ran!";\n', encoding="utf-8")

    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    out_bin = proj_dir / "bin" / "complex_app"
    if res.returncode == 0 and out_bin.is_file():
        exec_res = run_command([str(out_bin)], cwd=proj_dir)
        if exec_res.returncode == 0 and "Complex manifest compiled and ran!" in exec_res.stdout:
            record_pass("T1_build_complex_manifest_nested_keys")
        else:
            record_fail("T1_build_complex_manifest_nested_keys", f"Execution failed: {exec_res.stdout}")
    else:
        record_fail("T1_build_complex_manifest_nested_keys", f"Build failed: {res.stdout} {res.stderr}")

# 4.6 Escaped quotes in tapri.json string
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    raw_json = '''{
      "name": "escaped_app",
      "description": "A \\"very cool\\" project",
      "entry": "src/main.meme",
      "output": "bin/escaped_app"
    }'''
    (proj_dir / "tapri.json").write_text(raw_json, encoding="utf-8")
    src_dir = proj_dir / "src"
    src_dir.mkdir()
    (src_dir / "main.meme").write_text('dikha "Escaped quotes project ran!";\n', encoding="utf-8")

    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    out_bin = proj_dir / "bin" / "escaped_app"
    if res.returncode == 0 and out_bin.is_file():
        exec_res = run_command([str(out_bin)], cwd=proj_dir)
        if exec_res.returncode == 0 and "Escaped quotes project ran!" in exec_res.stdout:
            record_pass("T1_build_escaped_quotes_in_manifest")
        else:
            record_fail("T1_build_escaped_quotes_in_manifest", f"Execution failed: {exec_res.stdout}")
    else:
        record_fail("T1_build_escaped_quotes_in_manifest", f"Build failed: {res.stdout} {res.stderr}")

# 4.7 Malformed JSON: unclosed brace
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    (proj_dir / "tapri.json").write_text('{"name": "broken_brace"', encoding="utf-8")
    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    if res.returncode != 0 and "Manifest error" in (res.stdout + res.stderr):
        record_pass("T2_build_malformed_json_unclosed_brace")
    else:
        record_fail("T2_build_malformed_json_unclosed_brace", f"Expected failure, got exit {res.returncode}")

# 4.8 Malformed JSON: unclosed quote
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    (proj_dir / "tapri.json").write_text('{"name": "broken_quote', encoding="utf-8")
    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    if res.returncode != 0 and "Manifest error" in (res.stdout + res.stderr):
        record_pass("T2_build_malformed_json_unclosed_quote")
    else:
        record_fail("T2_build_malformed_json_unclosed_quote", f"Expected failure, got exit {res.returncode}")

# 4.9 Malformed JSON: pure garbage text
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    (proj_dir / "tapri.json").write_text('this is definitely not json at all', encoding="utf-8")
    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    if res.returncode != 0:
        record_pass("T2_build_malformed_json_garbage_text")
    else:
        record_fail("T2_build_malformed_json_garbage_text", f"Expected failure, got exit {res.returncode}")

# 4.10 Missing tapri.json in directory
with tempfile.TemporaryDirectory() as tmp_dir:
    res = run_command([str(MEME_BIN), "build"], cwd=Path(tmp_dir))
    if res.returncode != 0 and "tapri.json" in (res.stdout + res.stderr).lower():
        record_pass("T2_build_missing_manifest_error")
    else:
        record_fail("T2_build_missing_manifest_error", f"Expected failure, got exit {res.returncode}")

# 4.11 Missing entrypoint file specified in tapri.json
with tempfile.TemporaryDirectory() as tmp_dir:
    proj_dir = Path(tmp_dir)
    manifest = proj_dir / "tapri.json"
    manifest.write_text(json.dumps({
        "name": "missing_entry_project",
        "entry": "nonexistent_file.meme",
        "output": "bin/app"
    }), encoding="utf-8")
    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir)
    if res.returncode != 0 and "Entry file" in (res.stdout + res.stderr):
        record_pass("T2_build_missing_entrypoint_error")
    else:
        record_fail("T2_build_missing_entrypoint_error", f"Expected failure, got exit {res.returncode}")

# 4.12 Project directory itself contains spaces
base_tmp = Path(tempfile.gettempdir())
proj_dir_with_spaces = base_tmp / "meme project with spaces"
try:
    if proj_dir_with_spaces.exists():
        import shutil
        shutil.rmtree(proj_dir_with_spaces)
    proj_dir_with_spaces.mkdir(parents=True)

    manifest = proj_dir_with_spaces / "tapri.json"
    manifest.write_text(json.dumps({
        "name": "spaces_project",
        "entry": "src/main.meme",
        "output": "bin/app"
    }), encoding="utf-8")
    src_dir = proj_dir_with_spaces / "src"
    src_dir.mkdir()
    (src_dir / "main.meme").write_text('dikha "Project in dir with spaces ran!";\n', encoding="utf-8")

    res = run_command([str(MEME_BIN), "build"], cwd=proj_dir_with_spaces)
    out_bin = proj_dir_with_spaces / "bin" / "app"
    if res.returncode == 0 and out_bin.is_file():
        exec_res = run_command([str(out_bin)], cwd=proj_dir_with_spaces)
        if exec_res.returncode == 0 and "Project in dir with spaces ran!" in exec_res.stdout:
            record_pass("T1_build_project_directory_with_spaces")
        else:
            record_fail("T1_build_project_directory_with_spaces", f"Execution failed: {exec_res.stdout}")
    else:
        record_fail("T1_build_project_directory_with_spaces", f"Build failed: {res.stdout} {res.stderr}")
finally:
    if proj_dir_with_spaces.exists():
        import shutil
        shutil.rmtree(proj_dir_with_spaces, ignore_errors=True)


print("\n=================================================================")
print(f"📊 Summary: {len(passes)} PASSED, {len(failures)} FAILED")
print("=================================================================")

if failures:
    sys.exit(1)
sys.exit(0)
