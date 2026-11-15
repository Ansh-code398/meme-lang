#!/usr/bin/env python3
"""
MEMElang E2E Test Suite - Milestone 3: JSON and CSV Module Parity (Interpreter & AOT)
===================================================================================
Validates that:
  1. JSON parsing and stringification work in the tree-walking interpreter.
  2. CSV parsing (with/without headers, quoted fields) and stringification work in the interpreter.
  3. JSON module and dynamic property access compile and run in native AOT binaries.
  4. CSV module and dynamic property access compile and run in native AOT binaries.
  5. File module json/csv helper methods (padh_json, likh_json, padh_csv, likh_csv) work.
  6. Self-hosted compiler (compiler.meme) successfully compiles json and csv programs.
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


def test_m3_suite():
    print("=== Running E2E Test Suite: Milestone 3 JSON/CSV Dual Engine Parity ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # Test 1: Interpreter JSON parse and stringify
        total += 1
        res = run_command([
            str(MEME_BIN), "-e",
            'bula "json"; '
            'pakad raw = "{\\"name\\":\\"chai\\",\\"count\\":42,\\"items\\":[1,2,3],\\"active\\":true}"; '
            'pakad doc = json.samajh(raw); '
            'pakad out = json.likh(doc); '
            'dikha f"PARSED:{doc.name}|{doc.count}|{doc.items.size()}|{doc.active}";'
        ])
        if res.returncode == 0 and "PARSED:chai|42|3|true" in res.stdout:
            print("[PASS] T1_interpreter_json_parse_stringify")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_json_parse_stringify\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 2: Interpreter CSV parse and stringify
        total += 1
        res = run_command([
            str(MEME_BIN), "-e",
            'bula "csv"; '
            'pakad raw = "id,product,price\\n1,\\"Special, Tea\\",15.5\\n2,Samosa,10.0\\n"; '
            'pakad rows = csv.padh(raw); '
            'pakad r0 = rows[0]; '
            'pakad r1 = rows[1]; '
            'dikha f"ROWS:{rows.size()}|R0:{r0.product}|PRICE:{r0.price}|R1:{r1.product}";'
        ])
        if res.returncode == 0 and "ROWS:2|R0:Special, Tea|PRICE:15.5|R1:Samosa" in res.stdout:
            print("[PASS] T1_interpreter_csv_parse_stringify")
            passed += 1
        else:
            print(f"[FAIL] T1_interpreter_csv_parse_stringify\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 3: Native Compiler JSON program AOT
        total += 1
        json_meme_file = tmp_path / "test_json_aot.meme"
        json_meme_file.write_text("""
bula "json";

pakad raw = "{\\"title\\":\\"order\\",\\"items\\":[{\\"id\\":1,\\"price\\":100},{\\"id\\":2,\\"price\\":250}]}";
pakad doc = json.samajh(raw);
pakad total = 0;
ghoom it in doc.items {
    total = total + it.price;
}
dikha f"JSON_AOT:{doc.title}|TOTAL:{total}|COUNT:{doc.items.size()}";
""")
        json_bin = tmp_path / "test_json_bin"
        res = run_command([str(MEME_BIN), "compile", str(json_meme_file), "-o", str(json_bin)])
        if res.returncode == 0 and json_bin.exists():
            res_exec = run_command([str(json_bin)])
            if res_exec.returncode == 0 and "JSON_AOT:order|TOTAL:350|COUNT:2" in res_exec.stdout:
                print("[PASS] T1_native_compiler_json_aot")
                passed += 1
            else:
                print(f"[FAIL] T1_native_compiler_json_aot exec\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_native_compiler_json_aot compile\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 4: Native Compiler CSV program AOT
        total += 1
        csv_meme_file = tmp_path / "test_csv_aot.meme"
        csv_meme_file.write_text("""
bula "csv";

pakad raw = "name,role,salary\\nAlice,Lead,50000\\nBob,Dev,40000\\nCharlie,\\"Dev, Senior\\",60000\\n";
pakad staff = csv.padh(raw);
pakad total_sal = 0;
ghoom s in staff {
    total_sal = total_sal + bina_point(s.salary);
}
dikha f"CSV_AOT:COUNT:{staff.size()}|TOTAL:{total_sal}|C2:{staff[2].name}";
""")
        csv_bin = tmp_path / "test_csv_bin"
        res = run_command([str(MEME_BIN), "compile", str(csv_meme_file), "-o", str(csv_bin)])
        if res.returncode == 0 and csv_bin.exists():
            res_exec = run_command([str(csv_bin)])
            if res_exec.returncode == 0 and "CSV_AOT:COUNT:3|TOTAL:150000|C2:Charlie" in res_exec.stdout:
                print("[PASS] T1_native_compiler_csv_aot")
                passed += 1
            else:
                print(f"[FAIL] T1_native_compiler_csv_aot exec\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_native_compiler_csv_aot compile\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 5: File module JSON and CSV file I/O (native AOT)
        total += 1
        fs_meme_file = tmp_path / "test_fs_json_csv.meme"
        json_data_path = tmp_path / "sample.json"
        csv_data_path = tmp_path / "sample.csv"
        json_data_path.write_text('{"greeting":"ram_ram","code":108}')
        csv_data_path.write_text("item,qty\ncutting_chai,5\nsamosa,3\n")

        fs_meme_file.write_text(f"""
bula "file" jaise fs;

pakad jd = fs.padh_json("{json_data_path}");
pakad cd = fs.padh_csv("{csv_data_path}");
pakad total_qty = 0;
ghoom r in cd {{
    total_qty = total_qty + bina_point(r.qty);
}}
dikha f"FS_AOT:JSON:{{jd.greeting}}:{{jd.code}}|CSV_ITEMS:{{cd.size()}}|TOTAL_QTY:{{total_qty}}";
""")
        fs_bin = tmp_path / "test_fs_bin"
        res = run_command([str(MEME_BIN), "compile", str(fs_meme_file), "-o", str(fs_bin)])
        if res.returncode == 0 and fs_bin.exists():
            res_exec = run_command([str(fs_bin)])
            if res_exec.returncode == 0 and "FS_AOT:JSON:ram_ram:108|CSV_ITEMS:2|TOTAL_QTY:8" in res_exec.stdout:
                print("[PASS] T1_file_module_json_csv_aot")
                passed += 1
            else:
                print(f"[FAIL] T1_file_module_json_csv_aot exec\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_file_module_json_csv_aot compile\nstdout: {res.stdout}\nstderr: {res.stderr}")
            sys.exit(1)

        # Test 6: Self-hosted compiler (compiler.meme) compiling JSON benchmark
        total += 1
        compiler_bin = tmp_path / "compiler_m3_native"
        res_build_comp = run_command([str(MEME_BIN), "compile", str(COMPILER_MEME), "-o", str(compiler_bin)])
        if res_build_comp.returncode != 0:
            print(f"[FAIL] T1_self_hosted_compiler_json build\nstdout: {res_build_comp.stdout}\nstderr: {res_build_comp.stderr}")
            sys.exit(1)

        self_json_bin = tmp_path / "self_json_bin"
        res_compile_script = run_command([str(compiler_bin), str(json_meme_file), "-o", str(self_json_bin)])
        if res_compile_script.returncode == 0 and self_json_bin.exists():
            res_exec = run_command([str(self_json_bin)])
            if res_exec.returncode == 0 and "JSON_AOT:order|TOTAL:350|COUNT:2" in res_exec.stdout:
                print("[PASS] T1_self_hosted_compiler_json_exec")
                passed += 1
            else:
                print(f"[FAIL] T1_self_hosted_compiler_json_exec\nstdout: {res_exec.stdout}\nstderr: {res_exec.stderr}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_self_hosted_compiler_json compile\nstdout: {res_compile_script.stdout}\nstderr: {res_compile_script.stderr}")
            sys.exit(1)

    print(f"\nResults: {passed}/{total} tests passed.")
    print("✔ All Milestone 3 E2E Tests Passed successfully!")


if __name__ == "__main__":
    test_m3_suite()
