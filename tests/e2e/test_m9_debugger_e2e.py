#!/usr/bin/env python3
"""
MEMElang Milestone 9 E2E Test Suite: Source-Level Debugger Support (P2)
Verifies:
  1. #line directive emission in native compiler (DWARF .debug_line mappings)
  2. Native LLDB breakpoint hits on function body line in .meme source
  3. Native LLDB breakpoint hits on top-level statements in .meme source
  4. Native LLDB frame variable inspection
  5. DAP Server stdio protocol initialization & capabilities
  6. DAP Server breakpoint setting & verified response
  7. DAP Server stop event on execution & thread inspection
  8. DAP Server stackTrace, scopes, variables inspection & disconnect
"""

import os
import subprocess
import sys
import tempfile
import json
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"


def run_command(cmd, cwd=None, input_text=None, timeout=30):
    try:
        proc = subprocess.run(
            cmd,
            cwd=cwd,
            input=input_text,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return proc
    except Exception as e:
        return subprocess.CompletedProcess(cmd, returncode=-1, stdout="", stderr=str(e))


def encode_dap(msg):
    body = json.dumps(msg)
    return f"Content-Length: {len(body)}\r\n\r\n{body}".encode("utf-8")


def read_dap(proc):
    header = b""
    while b"\r\n\r\n" not in header:
        chunk = proc.stdout.read(1)
        if not chunk:
            return None
        header += chunk
    lines = header.decode("utf-8").split("\r\n")
    length = 0
    for l in lines:
        if l.startswith("Content-Length:"):
            length = int(l.split(":")[1].strip())
    body = proc.stdout.read(length)
    return json.loads(body.decode("utf-8"))


def test_m9_suite():
    print("=== Running E2E Test Suite: Milestone 9 Source-Level Debugger Support ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        sample_code = """kaam hisaab(a, b) {
    le ans = a * b + 5;
    wapas ans;
}

le num1 = 12;
le num2 = 3;
le final_ans = hisaab(num1, num2);
dikha final_ans;
"""
        src_file = tmp_path / "calc.meme"
        src_file.write_text(sample_code, encoding="utf-8")
        bin_file = tmp_path / "calc_bin"

        # Test 1: Native compilation with -g flag and #line generation
        total += 1
        comp_res = run_command([str(MEME_BIN), "compile", str(src_file), "-g", "-o", str(bin_file)])
        if comp_res.returncode == 0 and bin_file.exists():
            print("[PASS] T1_native_compile_with_debug_symbols")
            passed += 1
        else:
            print(f"[FAIL] T1_native_compile_with_debug_symbols: Code: {comp_res.returncode}, Stderr: {comp_res.stderr}")

        # Test 2: Native LLDB breakpoint hit on function body line 2 in .meme source
        total += 1
        lldb_res2 = run_command([
            "lldb", "--batch",
            "-o", "b calc.meme:2",
            "-o", "run",
            "-o", "quit",
            str(bin_file)
        ])
        if "calc.meme:2" in lldb_res2.stdout and "stop reason = breakpoint" in lldb_res2.stdout:
            print("[PASS] T1_lldb_breakpoint_hits_function_body_in_meme_source")
            passed += 1
        else:
            print(f"[FAIL] T1_lldb_breakpoint_hits_function_body_in_meme_source: Output:\n{lldb_res2.stdout}\nStderr:\n{lldb_res2.stderr}")

        # Test 3: Native LLDB breakpoint hit on top-level statement line 8 in .meme source
        total += 1
        lldb_res3 = run_command([
            "lldb", "--batch",
            "-o", "b calc.meme:8",
            "-o", "run",
            "-o", "quit",
            str(bin_file)
        ])
        if "calc.meme:8" in lldb_res3.stdout and "stop reason = breakpoint" in lldb_res3.stdout:
            print("[PASS] T1_lldb_breakpoint_hits_top_level_statement_in_meme_source")
            passed += 1
        else:
            print(f"[FAIL] T1_lldb_breakpoint_hits_top_level_statement_in_meme_source: Output:\n{lldb_res3.stdout}\nStderr:\n{lldb_res3.stderr}")

        # Test 4: Native LLDB frame variable inspection at breakpoint
        total += 1
        lldb_res4 = run_command([
            "lldb", "--batch",
            "-o", "b calc.meme:2",
            "-o", "run",
            "-o", "frame variable",
            "-o", "quit",
            str(bin_file)
        ])
        if "stop reason = breakpoint" in lldb_res4.stdout and ("a =" in lldb_res4.stdout or "b =" in lldb_res4.stdout):
            print("[PASS] T1_lldb_frame_variable_inspection")
            passed += 1
        else:
            print(f"[FAIL] T1_lldb_frame_variable_inspection: Output:\n{lldb_res4.stdout}")

        # Test 5: DAP Server handshake & initialization
        total += 1
        dap_proc = subprocess.Popen(
            [str(MEME_BIN), "dap"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        try:
            dap_proc.stdin.write(encode_dap({"seq": 1, "type": "request", "command": "initialize", "arguments": {}}))
            dap_proc.stdin.flush()
            init_resp = read_dap(dap_proc)
            init_ev = read_dap(dap_proc)
            if init_resp and init_resp.get("success") and init_ev and init_ev.get("event") == "initialized":
                print("[PASS] T1_dap_server_initialize_handshake")
                passed += 1
            else:
                print(f"[FAIL] T1_dap_server_initialize_handshake: Resp: {init_resp}, Event: {init_ev}")
        except Exception as e:
            print(f"[FAIL] T1_dap_server_initialize_handshake: Exception: {e}")

        # Test 6: DAP Server launch & setBreakpoints
        total += 1
        try:
            dap_proc.stdin.write(encode_dap({"seq": 2, "type": "request", "command": "launch", "arguments": {"program": str(src_file), "stopOnEntry": True}}))
            dap_proc.stdin.flush()
            launch_resp = read_dap(dap_proc)

            dap_proc.stdin.write(encode_dap({"seq": 3, "type": "request", "command": "setBreakpoints", "arguments": {"source": {"path": str(src_file)}, "lines": [2, 8]}}))
            dap_proc.stdin.flush()
            bp_resp = read_dap(dap_proc)

            if launch_resp and launch_resp.get("success") and bp_resp and bp_resp.get("success") and len(bp_resp["body"]["breakpoints"]) == 2:
                print("[PASS] T1_dap_server_launch_and_set_breakpoints")
                passed += 1
            else:
                print(f"[FAIL] T1_dap_server_launch_and_set_breakpoints: Launch: {launch_resp}, BP: {bp_resp}")
        except Exception as e:
            print(f"[FAIL] T1_dap_server_launch_and_set_breakpoints: Exception: {e}")

        # Test 7: DAP Server configurationDone & stopped event
        total += 1
        try:
            dap_proc.stdin.write(encode_dap({"seq": 4, "type": "request", "command": "configurationDone", "arguments": {}}))
            dap_proc.stdin.flush()
            cfg_resp = read_dap(dap_proc)
            stop_ev = read_dap(dap_proc)

            if cfg_resp and cfg_resp.get("success") and stop_ev and stop_ev.get("event") == "stopped":
                print("[PASS] T1_dap_server_configuration_done_and_stop_event")
                passed += 1
            else:
                print(f"[FAIL] T1_dap_server_configuration_done_and_stop_event: Cfg: {cfg_resp}, StopEv: {stop_ev}")
        except Exception as e:
            print(f"[FAIL] T1_dap_server_configuration_done_and_stop_event: Exception: {e}")

        # Test 8: DAP Server threads, stackTrace, scopes, variables & disconnect
        total += 1
        try:
            dap_proc.stdin.write(encode_dap({"seq": 5, "type": "request", "command": "threads", "arguments": {}}))
            dap_proc.stdin.flush()
            threads_resp = read_dap(dap_proc)

            dap_proc.stdin.write(encode_dap({"seq": 6, "type": "request", "command": "stackTrace", "arguments": {"threadId": 1}}))
            dap_proc.stdin.flush()
            stack_resp = read_dap(dap_proc)

            dap_proc.stdin.write(encode_dap({"seq": 7, "type": "request", "command": "scopes", "arguments": {"frameId": 1}}))
            dap_proc.stdin.flush()
            scopes_resp = read_dap(dap_proc)

            dap_proc.stdin.write(encode_dap({"seq": 8, "type": "request", "command": "variables", "arguments": {"variablesReference": 1}}))
            dap_proc.stdin.flush()
            var_resp = read_dap(dap_proc)

            dap_proc.stdin.write(encode_dap({"seq": 9, "type": "request", "command": "disconnect", "arguments": {}}))
            dap_proc.stdin.flush()
            disc_resp = read_dap(dap_proc)

            if (threads_resp and threads_resp.get("success") and
                stack_resp and stack_resp.get("success") and
                scopes_resp and scopes_resp.get("success") and
                var_resp and var_resp.get("success") and
                disc_resp and disc_resp.get("success")):
                print("[PASS] T1_dap_server_inspection_and_disconnect")
                passed += 1
            else:
                print(f"[FAIL] T1_dap_server_inspection_and_disconnect: Threads: {threads_resp}, Stack: {stack_resp}, Scopes: {scopes_resp}, Var: {var_resp}")
        except Exception as e:
            print(f"[FAIL] T1_dap_server_inspection_and_disconnect: Exception: {e}")
        finally:
            dap_proc.terminate()

    print(f"\nMilestone 9 Summary: {passed}/{total} tests passed.")
    if passed == total:
        print("ALL M9 DEBUGGER TESTS PASSED!")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(test_m9_suite())
