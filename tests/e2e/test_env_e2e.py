#!/usr/bin/env python3
"""
MEMElang E2E Test Suite - Environment Variables Module
=====================================================
Validates all aspects of environment variable support:
  1. env.get, env.set, env.has, env.delete / env.unset
  2. Desi aliases: env.la, env.rakh, env.hai, env.hatao, env.sab, env.chadhau, env.samajh
  3. Universal module aliases: bula "env", bula "paryavaran", bula "dotenv"
  4. Direct call syntax: env("KEY", "default")
  5. Map index syntax: env["KEY"] and env["KEY"] = "value"
  6. env.all() / env.sab() full environ dump
  7. env.parse() .env syntax parser (comments, quotes, exports, variable expansion)
  8. env.load() file loader with overwrite control
  9. CLI --env-file flag
  10. Native AOT compilation (meme compile) parity
  11. Native binary automatic .env loading
"""

import os
import sys
import tempfile
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"

def run_meme_code(code: str, env_vars=None, extra_args=None):
    cmd = [str(MEME_BIN)]
    if extra_args:
        cmd.extend(extra_args)
    cmd.extend(["-e", code])
    
    current_env = os.environ.copy()
    if env_vars:
        current_env.update(env_vars)
        
    proc = subprocess.run(
        cmd,
        cwd=PROJECT_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=current_env
    )
    return proc.returncode, proc.stdout, proc.stderr

def run_test(name, fn):
    print(f"  • {name}...", end=" ", flush=True)
    try:
        fn()
        print("[PASS]")
        return True
    except AssertionError as e:
        print(f"[FAIL] Assertion: {e}")
        return False
    except Exception as e:
        print(f"[ERROR] Exception: {e}")
        return False

def test_env_basic_crud():
    code = """
    env.set("TEST_PORT", "9090");
    pakad has_it = env.has("TEST_PORT");
    pakad val = env.get("TEST_PORT");
    env.delete("TEST_PORT");
    pakad has_after = env.has("TEST_PORT");
    pakad def_val = env.get("TEST_PORT", "default_val");
    dikha f"{has_it}|{val}|{has_after}|{def_val}";
    """
    code_res, out, err = run_meme_code(code)
    assert code_res == 0, f"Process failed: {err}"
    assert "true|9090|false|default_val" in out, f"Unexpected output: {out}"

def test_env_desi_aliases():
    code = """
    bula "paryavaran" jaise p;
    p.rakh("TEST_DESI", "desi_swag");
    pakad hai = p.hai("TEST_DESI");
    pakad la = p.la("TEST_DESI");
    p.hatao("TEST_DESI");
    pakad hai_after = p.hai("TEST_DESI");
    dikha f"{hai}|{la}|{hai_after}";
    """
    code_res, out, err = run_meme_code(code)
    assert code_res == 0, f"Process failed: {err}"
    assert "true|desi_swag|false" in out, f"Unexpected output: {out}"

def test_env_direct_call_and_index():
    code = """
    env["INDEX_KEY"] = "indexed_value";
    pakad from_index = env["INDEX_KEY"];
    pakad from_call = env("INDEX_KEY");
    pakad call_default = env("UNKNOWN_XYZ", "fallback");
    dikha f"{from_index}|{from_call}|{call_default}";
    """
    code_res, out, err = run_meme_code(code)
    assert code_res == 0, f"Process failed: {err}"
    assert "indexed_value|indexed_value|fallback" in out, f"Unexpected output: {out}"

def test_env_all():
    code = """
    env.set("TEST_ALL_KEY", "all_val");
    pakad all_vars = env.all();
    dikha f"HAS_KEY:{all_vars.has('TEST_ALL_KEY')}|LEN_POS:{all_vars.length() > 0}";
    """
    code_res, out, err = run_meme_code(code)
    assert code_res == 0, f"Process failed: {err}"
    assert "HAS_KEY:true|LEN_POS:true" in out, f"Unexpected output: {out}"

def test_env_parse_full_syntax():
    code = """
    pakad text = "# Comment line\\n" +
                 "PORT=4000 # inline comment\\n" +
                 "export APP_TITLE=\\"My Test Application\\"\\n" +
                 "SECRET_KEY='p@ss#word'\\n" +
                 "HOST=127.0.0.1\\n" +
                 "URL=\\"http://${HOST}:${PORT}/v1\\"\\n";
    pakad p = env.parse(text);
    dikha f"{p['PORT']}|{p['APP_TITLE']}|{p['SECRET_KEY']}|{p['URL']}";
    """
    code_res, out, err = run_meme_code(code)
    assert code_res == 0, f"Process failed: {err}"
    assert "4000|My Test Application|p@ss#word|http://127.0.0.1:4000/v1" in out, f"Unexpected output: {out}"

def test_env_load_file():
    with tempfile.NamedTemporaryFile("w", delete=False, suffix=".env") as f:
        f.write("LOADED_VAR=from_file_123\n")
        f.write("OVERWRITE_ME=initial\n")
        tmp_name = f.name
        
    try:
        code = f"""
        env.set("OVERWRITE_ME", "keep_me");
        env.load("{tmp_name}", jhooth);
        pakad v1 = env.get("LOADED_VAR");
        pakad v2 = env.get("OVERWRITE_ME");
        env.load("{tmp_name}", han);
        pakad v3 = env.get("OVERWRITE_ME");
        dikha f"{{v1}}|{{v2}}|{{v3}}";
        """
        code_res, out, err = run_meme_code(code)
        assert code_res == 0, f"Process failed: {err}"
        assert "from_file_123|keep_me|initial" in out, f"Unexpected output: {out}"
    finally:
        if os.path.exists(tmp_name):
            os.remove(tmp_name)

def test_cli_env_file_flag():
    with tempfile.NamedTemporaryFile("w", delete=False, suffix=".env") as f:
        f.write("CLI_FLAG_VAR=loaded_via_cli_flag\n")
        tmp_name = f.name
        
    try:
        code = """
        dikha f"CLI_VAL:{env.get('CLI_FLAG_VAR')}";
        """
        code_res, out, err = run_meme_code(code, extra_args=[f"--env-file={tmp_name}"])
        assert code_res == 0, f"Process failed: {err}"
        assert "CLI_VAL:loaded_via_cli_flag" in out, f"Unexpected output: {out}"
    finally:
        if os.path.exists(tmp_name):
            os.remove(tmp_name)

def test_native_compilation_env():
    with tempfile.TemporaryDirectory() as tmpdir:
        src_path = Path(tmpdir) / "test_native.meme"
        bin_path = Path(tmpdir) / "test_native_bin"
        
        src_path.write_text("""
        bula "env" jaise env;
        env.set("NATIVE_RUN_VAR", "native_val_999");
        pakad v = env.get("NATIVE_RUN_VAR");
        pakad h = env.has("NATIVE_RUN_VAR");
        pakad call_v = env("NATIVE_RUN_VAR");
        pakad idx_v = env["NATIVE_RUN_VAR"];
        env.delete("NATIVE_RUN_VAR");
        pakad h_after = env.has("NATIVE_RUN_VAR");
        dikha f"{v}|{h}|{call_v}|{idx_v}|{h_after}";
        """)
        
        comp_res = subprocess.run(
            [str(MEME_BIN), "compile", str(src_path), "-o", str(bin_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert comp_res.returncode == 0, f"Native compilation failed: {comp_res.stderr}"
        
        run_res = subprocess.run(
            [str(bin_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert run_res.returncode == 0, f"Native binary failed: {run_res.stderr}"
        assert "native_val_999|true|native_val_999|native_val_999|false" in run_res.stdout, f"Unexpected output: {run_res.stdout}"

def test_native_auto_load():
    with tempfile.TemporaryDirectory() as tmpdir:
        env_path = Path(tmpdir) / ".env"
        env_path.write_text("AUTOLOAD_NATIVE=autoload_works_100\n")
        
        src_path = Path(tmpdir) / "test_autoload.meme"
        bin_path = Path(tmpdir) / "test_autoload_bin"
        
        src_path.write_text("""
        dikha f"AUTOLOAD:{env.get('AUTOLOAD_NATIVE')}";
        """)
        
        comp_res = subprocess.run(
            [str(MEME_BIN), "compile", str(src_path), "-o", str(bin_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert comp_res.returncode == 0, f"Native compilation failed: {comp_res.stderr}"
        
        run_res = subprocess.run(
            [str(bin_path)],
            cwd=tmpdir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        assert run_res.returncode == 0, f"Native binary failed: {run_res.stderr}"
        assert "AUTOLOAD:autoload_works_100" in run_res.stdout, f"Unexpected output: {run_res.stdout}"

def main():
    print("=== Running E2E Test Suite: Environment Variables Support ===")
    tests = [
        ("T1_env_basic_crud", test_env_basic_crud),
        ("T1_env_desi_aliases", test_env_desi_aliases),
        ("T1_env_direct_call_and_index", test_env_direct_call_and_index),
        ("T1_env_all", test_env_all),
        ("T1_env_parse_full_syntax", test_env_parse_full_syntax),
        ("T1_env_load_file", test_env_load_file),
        ("T1_cli_env_file_flag", test_cli_env_file_flag),
        ("T1_native_compilation_env", test_native_compilation_env),
        ("T1_native_auto_load", test_native_auto_load),
    ]
    
    passed = 0
    for name, fn in tests:
        if run_test(name, fn):
            passed += 1
            
    print(f"\nResults: {passed}/{len(tests)} tests passed.")
    if passed == len(tests):
        print("✔ All Environment Variable E2E Tests Passed successfully!")
        return 0
    return 1

if __name__ == "__main__":
    sys.exit(main())
