#!/usr/bin/env python3
"""
MEMElang E2E Test Suite - Milestone 5: Language Server Protocol (LSP) Engine
==========================================================================
Validates that:
  1. `./meme --lsp` launches over stdio and responds to JSON-RPC 2.0 handshake (`initialize`).
  2. `textDocument/didOpen` on valid code emits 0 diagnostics.
  3. `textDocument/didOpen` on syntax error emits diagnostics with line/character ranges.
  4. `textDocument/didChange` dynamically re-parses and updates diagnostics.
  5. `textDocument/hover` returns documentation markdown for keywords and modules.
  6. `textDocument/completion` returns available keywords and builtins.
  7. `textDocument/definition` resolves symbol definitions in the document.
  8. `shutdown` and `exit` cleanly terminate the server process with exit code 0.
"""

import sys
import os
import json
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = PROJECT_ROOT / "meme"


class LspClient:
    def __init__(self, meme_bin):
        self.proc = subprocess.Popen(
            [str(meme_bin), "--lsp"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

    def send(self, msg):
        raw = json.dumps(msg).encode("utf-8")
        header = f"Content-Length: {len(raw)}\r\n\r\n".encode("utf-8")
        self.proc.stdin.write(header + raw)
        self.proc.stdin.flush()

    def read_message(self, timeout=5):
        headers = {}
        while True:
            line = self.proc.stdout.readline().decode("utf-8")
            if line == "\r\n" or line == "\n" or line == "":
                break
            if ": " in line:
                k, v = line.strip().split(": ", 1)
                headers[k] = v

        content_len = int(headers.get("Content-Length", 0))
        if content_len == 0:
            return None

        body = self.proc.stdout.read(content_len).decode("utf-8")
        return json.loads(body)

    def close(self):
        if self.proc.poll() is not None:
            return
        try:
            self.send({"jsonrpc": "2.0", "id": 999, "method": "shutdown"})
            self.read_message()
            self.send({"jsonrpc": "2.0", "method": "exit"})
            self.proc.wait(timeout=2)
        except Exception:
            self.proc.terminate()


def test_m5_suite():
    print("=== Running E2E Test Suite: Milestone 5 Language Server Protocol (LSP) ===")
    assert MEME_BIN.exists(), f"Host binary {MEME_BIN} not found"

    passed = 0
    total = 0
    client = LspClient(MEME_BIN)

    try:
        # Test 1: Initialize handshake
        total += 1
        client.send({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}})
        r1 = client.read_message()
        if (
            r1
            and r1.get("id") == 1
            and "capabilities" in r1.get("result", {})
            and r1["result"]["capabilities"].get("hoverProvider") is True
            and r1["result"]["capabilities"].get("definitionProvider") is True
        ):
            print("[PASS] T1_lsp_initialize_handshake")
            passed += 1
        else:
            print(f"[FAIL] T1_lsp_initialize_handshake: {r1}")
            sys.exit(1)

        # Notify initialized
        client.send({"jsonrpc": "2.0", "method": "initialized", "params": {}})

        # Test 2: didOpen with clean code
        total += 1
        clean_code = "pakad price = 25;\njugaad get_price() {\n    ye_le price;\n}\n"
        doc_uri = "file:///sample.meme"
        client.send({
            "jsonrpc": "2.0",
            "method": "textDocument/didOpen",
            "params": {
                "textDocument": {
                    "uri": doc_uri,
                    "languageId": "memelang",
                    "version": 1,
                    "text": clean_code
                }
            }
        })
        r2 = client.read_message()
        if (
            r2
            and r2.get("method") == "textDocument/publishDiagnostics"
            and len(r2["params"]["diagnostics"]) == 0
        ):
            print("[PASS] T1_lsp_didopen_clean_diagnostics")
            passed += 1
        else:
            print(f"[FAIL] T1_lsp_didopen_clean_diagnostics: {r2}")
            sys.exit(1)

        # Test 3: didChange with syntax error
        total += 1
        error_code = "pakad x = ;\n"
        client.send({
            "jsonrpc": "2.0",
            "method": "textDocument/didChange",
            "params": {
                "textDocument": {
                    "uri": doc_uri,
                    "version": 2
                },
                "contentChanges": [
                    {"text": error_code}
                ]
            }
        })
        r3 = client.read_message()
        if (
            r3
            and r3.get("method") == "textDocument/publishDiagnostics"
            and len(r3["params"]["diagnostics"]) > 0
        ):
            diag = r3["params"]["diagnostics"][0]
            if diag.get("severity") == 1:
                print("[PASS] T1_lsp_didchange_syntax_error_diagnostics")
                passed += 1
            else:
                print(f"[FAIL] T1_lsp_didchange_syntax_error_diagnostics: severity {diag.get('severity')}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_lsp_didchange_syntax_error_diagnostics: {r3}")
            sys.exit(1)

        # Test 4: didChange recovering from syntax error
        total += 1
        client.send({
            "jsonrpc": "2.0",
            "method": "textDocument/didChange",
            "params": {
                "textDocument": {
                    "uri": doc_uri,
                    "version": 3
                },
                "contentChanges": [
                    {"text": clean_code}
                ]
            }
        })
        r4 = client.read_message()
        if (
            r4
            and r4.get("method") == "textDocument/publishDiagnostics"
            and len(r4["params"]["diagnostics"]) == 0
        ):
            print("[PASS] T1_lsp_didchange_recovers_clean_diagnostics")
            passed += 1
        else:
            print(f"[FAIL] T1_lsp_didchange_recovers_clean_diagnostics: {r4}")
            sys.exit(1)

        # Test 5: hover on keyword
        total += 1
        client.send({
            "jsonrpc": "2.0",
            "id": 5,
            "method": "textDocument/hover",
            "params": {
                "textDocument": {"uri": doc_uri},
                "position": {"line": 0, "character": 2}
            }
        })
        r5 = client.read_message()
        if (
            r5
            and r5.get("id") == 5
            and "result" in r5
            and "contents" in r5["result"]
            and "pakad" in r5["result"]["contents"].get("value", "")
        ):
            print("[PASS] T1_lsp_hover_keywords")
            passed += 1
        else:
            print(f"[FAIL] T1_lsp_hover_keywords: {r5}")
            sys.exit(1)

        # Test 6: completion list
        total += 1
        client.send({
            "jsonrpc": "2.0",
            "id": 6,
            "method": "textDocument/completion",
            "params": {
                "textDocument": {"uri": doc_uri},
                "position": {"line": 0, "character": 0}
            }
        })
        r6 = client.read_message()
        if (
            r6
            and r6.get("id") == 6
            and isinstance(r6.get("result"), list)
            and len(r6["result"]) >= 20
        ):
            labels = {item.get("label") for item in r6["result"]}
            if "pakad" in labels and "jugaad" in labels and "json" in labels and "file" in labels:
                print("[PASS] T1_lsp_completion_list")
                passed += 1
            else:
                print(f"[FAIL] T1_lsp_completion_list missing key completions: {labels}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_lsp_completion_list: {r6}")
            sys.exit(1)

        # Test 7: definition lookup
        total += 1
        client.send({
            "jsonrpc": "2.0",
            "id": 7,
            "method": "textDocument/definition",
            "params": {
                "textDocument": {"uri": doc_uri},
                "position": {"line": 2, "character": 12}
            }
        })
        r7 = client.read_message()
        if (
            r7
            and r7.get("id") == 7
            and "result" in r7
            and r7["result"] is not None
            and "range" in r7["result"]
            and r7["result"]["range"]["start"]["line"] == 0
        ):
            print("[PASS] T1_lsp_definition_lookup")
            passed += 1
        else:
            print(f"[FAIL] T1_lsp_definition_lookup: {r7}")
            sys.exit(1)

        # Test 8: shutdown and exit
        total += 1
        client.send({"jsonrpc": "2.0", "id": 8, "method": "shutdown"})
        r8 = client.read_message()
        if r8 and r8.get("id") == 8 and r8.get("result") is None:
            client.send({"jsonrpc": "2.0", "method": "exit"})
            client.proc.wait(timeout=5)
            if client.proc.returncode == 0:
                print("[PASS] T1_lsp_shutdown_and_exit")
                passed += 1
            else:
                print(f"[FAIL] T1_lsp_shutdown_and_exit exit code: {client.proc.returncode}")
                sys.exit(1)
        else:
            print(f"[FAIL] T1_lsp_shutdown_and_exit response: {r8}")
            sys.exit(1)

    finally:
        client.close()

    print(f"\nResults: {passed}/{total} tests passed.")
    print("✔ All Milestone 5 E2E Tests Passed successfully!")


if __name__ == "__main__":
    test_m5_suite()
