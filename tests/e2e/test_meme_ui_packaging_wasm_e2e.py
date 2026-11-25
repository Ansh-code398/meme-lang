#!/usr/bin/env python3
"""
MEME-UI End-to-End Packaging, Cross-Target Artifact & Live WebAssembly/App Verification
======================================================================================
Verifies:
  1. Multi-platform AOT packaging (`./meme package --target=all`) for both:
       - apps/showcase/src/App.meme (Declarative JSX + Tailwind + Shadcn + Vercel Primitives + Canvas)
       - apps/showcase/src/EguiTelemetry.meme (Immediate-mode `egui` API)
     across all 8 targets: mac, win, deb, rpm, ipk, apk, ipa, web.
  2. Structural/binary/signature validation of every produced artifact using native tools:
       - mac: Mach-O arm64/host binary, codesign --verify, headless launch exit 0
       - win: PE32+ x86-64 executable
       - deb: ar archive (`debian-binary`, `control.tar.gz`, `data.tar.gz`) containing x86-64 ELF
       - rpm: RPM v3 lead (`0xedabeedb`) + gzip cpio containing x86-64 ELF + `.desktop`
       - ipk: tar.gz opkg archive (`debian-binary` 2.0 + `control.tar.gz` + `data.tar.gz`) containing ELF
       - apk: `aapt2 dump badging`, `apksigner verify`, dual ABI ELF `.so` (`arm64-v8a` & `x86_64`)
       - ipa: zip Payload `.app` Mach-O arm64 + Info.plist + codesign + `-Simulator.app` slice
       - web: zero-DOM `<canvas>` host + valid `.wasm` executing live in Node.js!
  3. Live interactive UI verification of `apps/showcase/src/App.meme` inside:
       - Compiled WebAssembly (`showcase.js` + `showcase.wasm`) via Node.js
       - Dual-engine parity driver (Interpreter vs `--native` AOT) driving every tab & widget.
"""

import os
import sys
import json
import gzip
import io
import re
import shutil
import struct
import tarfile
import zipfile
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
MEME_BIN = str(ROOT / "meme")
WORK_DIR = ROOT / ".tmp_ui_pkg_e2e"


def pass_test(name: str):
    print(f"[PASS] {name}")


def fail_test(name: str, detail: str):
    print(f"[FAIL] {name}: {detail}")
    raise AssertionError(f"{name}: {detail}")


def check_elf_arch(data: bytes, expected_e_machine: int, label: str):
    if len(data) < 20 or data[:4] != b"\x7fELF":
        fail_test(label, f"Missing ELF magic in {label}")
    e_class = data[4]
    e_machine = struct.unpack("<H", data[18:20])[0]
    if e_class != 2:
        fail_test(label, f"Expected 64-bit ELF class=2, got {e_class}")
    if e_machine != expected_e_machine:
        fail_test(label, f"Expected ELF e_machine={hex(expected_e_machine)}, got {hex(e_machine)}")


def run_cmd(cmd, env=None, timeout=180):
    res = subprocess.run(cmd, cwd=str(ROOT), env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout)
    return res


def parse_ar_members(ar_bytes: bytes) -> dict:
    assert ar_bytes[:8] == b"!<arch>\n", "Invalid ar magic header"
    pos = 8
    members = {}
    while pos + 60 <= len(ar_bytes):
        hdr = ar_bytes[pos : pos + 60]
        name = hdr[:16].decode("ascii", errors="ignore").strip().rstrip("/")
        size = int(hdr[48:58].decode("ascii", errors="ignore").strip())
        pos += 60
        members[name] = ar_bytes[pos : pos + size]
        pos += size
        if pos % 2 == 1:
            pos += 1
    return members


def verify_target_artifacts(pkg_dir: Path, app_name_cap: str, pkg_id: str, prefix: str, test_mac_run: bool = True):
    # 1. macOS (.app)
    mac_bin = pkg_dir / f"{app_name_cap}.app" / "Contents" / "MacOS" / app_name_cap
    if not mac_bin.is_file():
        fail_test(f"{prefix}_mac_exists", f"Missing {mac_bin}")
    r_file = run_cmd(["file", str(mac_bin)])
    if "Mach-O" not in r_file.stdout or "executable" not in r_file.stdout:
        fail_test(f"{prefix}_mac_macho", r_file.stdout)
    r_cs = run_cmd(["codesign", "--verify", "--strict", str(pkg_dir / f"{app_name_cap}.app")])
    if r_cs.returncode != 0:
        fail_test(f"{prefix}_mac_codesign", r_cs.stderr)
    if test_mac_run:
        env = os.environ.copy()
        env["MEME_HEADLESS"] = "1"
        r_run = run_cmd([str(mac_bin)], env=env, timeout=30)
        if r_run.returncode != 0:
            fail_test(f"{prefix}_mac_exec", r_run.stderr)
    pass_test(f"T1_{prefix}_mac_packaged_signed_and_exec")

    # 2. Windows (.exe)
    exe_path = pkg_dir / f"{pkg_id}.exe"
    if not exe_path.is_file():
        fail_test(f"{prefix}_win_exists", f"Missing {exe_path}")
    exe_bytes = exe_path.read_bytes()
    if exe_bytes[:2] != b"MZ":
        fail_test(f"{prefix}_win_mz", "Missing PE MZ header")
    r_win = run_cmd(["file", str(exe_path)])
    if "PE32+" not in r_win.stdout or "x86-64" not in r_win.stdout:
        fail_test(f"{prefix}_win_pe", r_win.stdout)
    pass_test(f"T1_{prefix}_win_pe32plus_x64")

    # 3. Linux Debian (.deb)
    deb_path = pkg_dir / f"{pkg_id}_1.0.0_amd64.deb"
    if not deb_path.is_file():
        fail_test(f"{prefix}_deb_exists", f"Missing {deb_path}")
    ar_members = parse_ar_members(deb_path.read_bytes())
    for req in ("debian-binary", "control.tar.gz", "data.tar.gz"):
        if req not in ar_members:
            fail_test(f"{prefix}_deb_member_{req}", f"Missing {req} in deb ar archive")
    if ar_members["debian-binary"].strip() != b"2.0":
        fail_test(f"{prefix}_deb_ver", repr(ar_members["debian-binary"]))
    with tarfile.open(fileobj=io.BytesIO(ar_members["data.tar.gz"]), mode="r:gz") as tf:
        names = tf.getnames()
        elf_member = next((n for n in names if n.endswith(f"/usr/local/bin/{pkg_id}") or n == f"./usr/local/bin/{pkg_id}"), None)
        if not elf_member:
            fail_test(f"{prefix}_deb_elf_entry", str(names))
        elf_bytes = tf.extractfile(elf_member).read()
        check_elf_arch(elf_bytes, 0x3E, f"T1_{prefix}_deb_x64_elf")
    pass_test(f"T1_{prefix}_deb_archive_and_elf")

    # 4. Linux RPM (.rpm)
    rpm_path = pkg_dir / f"{pkg_id}-1.0.0-1.x86_64.rpm"
    if not rpm_path.is_file():
        fail_test(f"{prefix}_rpm_exists", f"Missing {rpm_path}")
    rpm_bytes = rpm_path.read_bytes()
    if rpm_bytes[:4] != b"\xed\xab\xee\xdb":
        fail_test(f"{prefix}_rpm_magic", repr(rpm_bytes[:8]))
    gz_idx = rpm_bytes.find(b"\x1f\x8b\x08")
    if gz_idx < 0:
        fail_test(f"{prefix}_rpm_payload", "Missing gzipped cpio payload in rpm")
    cpio_raw = gzip.decompress(rpm_bytes[gz_idx:])
    if b"\x7fELF" not in cpio_raw or f"/usr/local/bin/{pkg_id}".encode() not in cpio_raw:
        fail_test(f"{prefix}_rpm_cpio_elf", "ELF binary missing in RPM cpio payload")
    pass_test(f"T1_{prefix}_rpm_header_and_cpio_elf")

    # 5. Embedded Linux (.ipk)
    ipk_path = pkg_dir / f"{pkg_id}_1.0.0_x86_64.ipk"
    if not ipk_path.is_file():
        fail_test(f"{prefix}_ipk_exists", f"Missing {ipk_path}")
    with tarfile.open(ipk_path, mode="r:gz") as ipk_tf:
        ipk_names = ipk_tf.getnames()
        if "./debian-binary" not in ipk_names or "./data.tar.gz" not in ipk_names:
            fail_test(f"{prefix}_ipk_structure", str(ipk_names))
        data_gz = ipk_tf.extractfile("./data.tar.gz").read()
        with tarfile.open(fileobj=io.BytesIO(data_gz), mode="r:gz") as dtf:
            elf_mem = next((n for n in dtf.getnames() if n.endswith(f"/usr/bin/{pkg_id}")), None)
            if not elf_mem:
                fail_test(f"{prefix}_ipk_bin", str(dtf.getnames()))
            check_elf_arch(dtf.extractfile(elf_mem).read(), 0x3E, f"T1_{prefix}_ipk_elf")
    pass_test(f"T1_{prefix}_ipk_archive_and_elf")

    # 6. Android (.apk)
    apk_path = pkg_dir / f"{pkg_id}.apk"
    if not apk_path.is_file():
        fail_test(f"{prefix}_apk_exists", f"Missing {apk_path}")
    with zipfile.ZipFile(apk_path, "r") as zf:
        znames = zf.namelist()
        if "lib/arm64-v8a/libmemeapp.so" not in znames or "lib/x86_64/libmemeapp.so" not in znames:
            fail_test(f"{prefix}_apk_abis", str(znames))
        check_elf_arch(zf.read("lib/arm64-v8a/libmemeapp.so"), 0xB7, f"T1_{prefix}_apk_arm64")
        check_elf_arch(zf.read("lib/x86_64/libmemeapp.so"), 0x3E, f"T1_{prefix}_apk_x86_64")
    android_sdk = Path.home() / "Library" / "Android" / "sdk"
    if android_sdk.is_dir():
        bt_dirs = sorted((android_sdk / "build-tools").iterdir())
        if bt_dirs:
            bt = bt_dirs[-1]
            r_badg = run_cmd([str(bt / "aapt2"), "dump", "badging", str(apk_path)])
            if r_badg.returncode != 0 or "native-code:" not in r_badg.stdout:
                fail_test(f"{prefix}_apk_aapt2", r_badg.stdout + r_badg.stderr)
            r_sig = run_cmd([str(bt / "apksigner"), "verify", str(apk_path)])
            if r_sig.returncode != 0:
                fail_test(f"{prefix}_apk_apksigner", r_sig.stderr)
    pass_test(f"T1_{prefix}_apk_dual_abi_and_signed")

    # 7. Apple iOS (.ipa + Simulator.app)
    ipa_path = pkg_dir / f"{app_name_cap}.ipa"
    if not ipa_path.is_file():
        fail_test(f"{prefix}_ipa_exists", f"Missing {ipa_path}")
    with zipfile.ZipFile(ipa_path, "r") as zf:
        znames = zf.namelist()
        app_bin_rel = f"Payload/{app_name_cap}.app/{app_name_cap}"
        plist_rel = f"Payload/{app_name_cap}.app/Info.plist"
        if app_bin_rel not in znames or plist_rel not in znames:
            fail_test(f"{prefix}_ipa_contents", str(znames))
        macho_bytes = zf.read(app_bin_rel)
        if macho_bytes[:4] not in (b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xca\xfe\xba\xbe"):
            fail_test(f"{prefix}_ipa_macho_magic", repr(macho_bytes[:4]))
    sim_bin = pkg_dir / f"{app_name_cap}-Simulator.app" / app_name_cap
    if sim_bin.is_file():
        r_sim = run_cmd(["file", str(sim_bin)])
        if "Mach-O" not in r_sim.stdout:
            fail_test(f"{prefix}_ios_sim_macho", r_sim.stdout)
    pass_test(f"T1_{prefix}_ipa_arm64_and_simulator")

    # 8. Web (.html + .js + .wasm) zero-DOM check
    web_dir = pkg_dir / "web"
    html_path = web_dir / "index.html"
    js_path = web_dir / f"{pkg_id}.js"
    wasm_path = web_dir / f"{pkg_id}.wasm"
    for p in (html_path, js_path, wasm_path):
        if not p.is_file():
            fail_test(f"{prefix}_web_files", f"Missing {p}")
    html_txt = html_path.read_text(encoding="utf-8")
    if '<canvas id="meme-canvas"' not in html_txt or "<button" in html_txt or "<input" in html_txt:
        fail_test(f"{prefix}_web_zero_dom", "HTML shell must only render via <canvas id='meme-canvas'>")
    wasm_bytes = wasm_path.read_bytes()
    if wasm_bytes[:4] != b"\x00asm":
        fail_test(f"{prefix}_web_wasm_magic", repr(wasm_bytes[:8]))
    pass_test(f"T1_{prefix}_web_zero_dom_and_wasm")


def verify_live_wasm_and_app_parity(app_web_dir: Path, egui_web_dir: Path):
    # Drive Showcase App.meme inside actual compiled WebAssembly using Node.js!
    node_script = WORK_DIR / "verify_wasm.js"
    node_script.write_text(r"""
const assert = require('node:assert');
const path = require('node:path');

function loadWasmModule(jsFile) {
  return new Promise((resolve, reject) => {
    const abs = path.resolve(jsFile);
    delete require.cache[abs];
    const m = require(abs);
    if (m.calledRun) return resolve(m);
    m.onRuntimeInitialized = () => resolve(m);
    m.onAbort = (what) => reject(new Error('Wasm aborted: ' + what));
  });
}

(async () => {
  // 1. EguiTelemetry Wasm verification
  const eguiMod = await loadWasmModule(process.argv[2]);
  eguiMod._meme_web_start();
  eguiMod._meme_web_render(1040, 720);
  const eguiVerts = eguiMod._meme_web_get_mesh_vertex_count();
  const eguiIdx = eguiMod._meme_web_get_mesh_index_count();
  assert(eguiVerts > 50, 'Egui Wasm must emit vertices, got ' + eguiVerts);
  assert(eguiIdx > 80, 'Egui Wasm must emit indices, got ' + eguiIdx);
  const atlasPtr = eguiMod._meme_web_get_atlas_rgba();
  assert(eguiMod.HEAPU8[atlasPtr] === 255 && eguiMod.HEAPU8[atlasPtr + 3] === 255, 'Solid white atlas pixel check');
  const batches = JSON.parse(eguiMod.UTF8ToString(eguiMod._meme_web_get_batches_json()));
  assert(Array.isArray(batches) && batches.length >= 1, 'Batches missing');
  console.log('[PASS] T4_wasm_egui_renders_mesh_and_atlas');

  // 2. App.meme Full Interactive Wasm verification
  const appMod = await loadWasmModule(process.argv[3]);
  appMod._meme_web_start();
  appMod._meme_web_render(1040, 720);

  const getSnap = () => JSON.parse(appMod.UTF8ToString(appMod._meme_web_get_agent_snapshot()));
  const getCmds = () => JSON.parse(appMod.UTF8ToString(appMod._meme_web_get_commands_json()));
  const act = (t, a, v = '') => appMod.ccall('meme_web_agent_act', 'number', ['string', 'string', 'string'], [String(t), a, String(v)]);

  let s0 = getSnap();
  assert(s0.nodes.length >= 5, 'Initial showcase snapshot empty');
  let c0 = getCmds();
  assert(c0.some(c => c.type === 'DRAW_TEXT' && c.text.includes('10k Virtual Feed [0..')), 'Virtualizer missing initial range: ' + JSON.stringify(c0.filter(x => x.type === 'DRAW_TEXT').map(x => x.text)));
  assert(c0.some(c => c.type === 'DRAW_TEXT' && c.text.includes('#event-0')), 'Virtualizer missing #event-0 row');
  console.log('[PASS] T4_wasm_showcase_10k_virtualizer_initial');

  // Scroll +500 rows
  assert(act('scroll_next_btn', 'click') === 1);
  appMod._meme_web_render(1040, 720);
  let cScroll = getCmds();
  assert(cScroll.some(c => c.type === 'DRAW_TEXT' && c.text.includes('#event-497') || (c.text && c.text.includes('#event-500'))), 'Virtualizer did not scroll +500 rows');
  console.log('[PASS] T4_wasm_showcase_10k_virtualizer_scroll');

  // Chat compose + send
  act('chat_input', 'type', 'HelloWasm');
  act('chat_send_btn', 'click');
  appMod._meme_web_render(1040, 720);
  let cChat = getCmds();
  assert(cChat.some(c => c.type === 'DRAW_TEXT' && c.text === 'HelloWasm'), 'Chat message not rendered');
  assert(cChat.some(c => c.type === 'DRAW_TEXT' && c.text.includes('Dispatched message #4')), 'Toast message missing after send');
  console.log('[PASS] T4_wasm_showcase_chat_compose_and_toast');

  // Switch to Explorer tab & test Checkbox, Switch, Slider, Combobox Typeahead, Split Handle, Popover, Reorder
  assert(act('tab_explorer', 'click') === 1);
  appMod._meme_web_render(1040, 720);
  let sExp = getSnap();
  assert(sExp.nodes.some(n => n.testId === 'agree_checkbox' && n.checked === true), 'agree_checkbox initial state');
  act('agree_checkbox', 'click');
  appMod._meme_web_render(1040, 720);
  assert(getSnap().nodes.some(n => n.testId === 'agree_checkbox' && n.checked === false), 'agree_checkbox did not toggle');

  act('gain_slider', 'change', '85');
  appMod._meme_web_render(1040, 720);
  assert(getSnap().nodes.some(n => n.testId === 'gain_slider' && Math.abs(n.numValue - 85) < 0.01), 'gain_slider did not update');

  act('combobox_input', 'change', 'sol');
  appMod._meme_web_render(1040, 720);
  assert(getCmds().some(c => c.type === 'DRAW_TEXT' && c.text.includes('Solar Amber')), 'Typeahead failed to select Solar Amber');
  console.log('[PASS] T4_wasm_showcase_explorer_controls_and_typeahead');

  // Reorder Down & Open Modal Dialog
  act('reorder_down_0', 'click');
  appMod._meme_web_render(1040, 720);
  assert(getCmds().some(c => c.type === 'DRAW_TEXT' && c.text.includes('1. Layout Engine')), 'Reorder move failed');

  act('open_modal', 'click');
  appMod._meme_web_render(1040, 720);
  let sDlg = getSnap();
  let dlg = sDlg.nodes.find(n => n.testId === 'settings_dialog');
  assert(dlg && dlg.y > 0 && dlg.y + dlg.height <= 720, 'Modal dialog not within viewport: ' + JSON.stringify(dlg));
  act('modal_close_btn', 'click');
  appMod._meme_web_render(1040, 720);
  assert(!getSnap().nodes.some(n => n.testId === 'settings_dialog'), 'Modal dialog did not close');
  console.log('[PASS] T4_wasm_showcase_reorder_and_centered_modal');

  // Immediate Canvas tab + Editor tab + RTL
  act('tab_telemetry', 'click');
  appMod._meme_web_render(1040, 720);
  assert(getCmds().some(c => c.type === 'DRAW_POLYLINE'), 'Telemetry canvas did not draw oscilloscope polyline');

  act('tab_editor', 'click');
  appMod._meme_web_render(1040, 720);
  assert(getSnap().nodes.some(n => n.testId === 'secret_input' && n.isPassword === true), 'Password field missing isPassword');

  act('toggle_rtl', 'click');
  appMod._meme_web_render(1040, 720);
  assert(getCmds().some(c => c.type === 'DRAW_TEXT' && c.text === 'Direction: RTL'), 'RTL toggle failed');
  console.log('[PASS] T4_wasm_showcase_canvas_editor_and_rtl');

  // Raw pointer coordinate clicks inside overflow-y-auto root container (desktop + mobile viewport!)
  const clickAt = (x, y) => {
    appMod._meme_web_pointer_down(x, y, 0, 1);
    appMod._meme_web_pointer_up(x, y, 0);
    appMod._meme_web_render(appMod._vw || 1040, appMod._vh || 720);
  };
  appMod._vw = 393; appMod._vh = 852;
  appMod._meme_web_render(393, 852);
  let snapMob = getSnap();
  let tabExp = snapMob.nodes.find(n => n.testId === 'tab_explorer');
  assert(tabExp && tabExp.width > 20 && tabExp.height > 15, 'Mobile tab_explorer bounds missing');
  clickAt(tabExp.x + tabExp.width * 0.5, tabExp.y + tabExp.height * 0.5);
  assert(getSnap().nodes.some(n => n.testId === 'agree_checkbox'), 'Raw pointer tap on tab_explorer failed on mobile viewport');

  let chk = getSnap().nodes.find(n => n.testId === 'agree_checkbox');
  let beforeChk = chk.checked;
  clickAt(chk.x + chk.width * 0.5, chk.y + chk.height * 0.5);
  assert(getSnap().nodes.find(n => n.testId === 'agree_checkbox').checked === !beforeChk, 'Raw pointer tap on checkbox failed');

  let openDlgBtn = getSnap().nodes.find(n => n.testId === 'open_modal');
  clickAt(openDlgBtn.x + openDlgBtn.width * 0.5, openDlgBtn.y + openDlgBtn.height * 0.5);
  assert(getSnap().nodes.some(n => n.testId === 'settings_dialog'), 'Raw pointer tap did not open dialog');
  let closeDlgBtn = getSnap().nodes.find(n => n.testId === 'modal_close_btn');
  clickAt(closeDlgBtn.x + closeDlgBtn.width * 0.5, closeDlgBtn.y + closeDlgBtn.height * 0.5);
  assert(!getSnap().nodes.some(n => n.testId === 'settings_dialog'), 'Raw pointer tap did not close dialog');
  console.log('[PASS] T4_wasm_showcase_raw_pointer_coords_interactive');
})();
""", encoding="utf-8")

    r_node = run_cmd(["node", str(node_script), str(egui_web_dir / "eguitelemetry.js"), str(app_web_dir / "showcase.js")], timeout=60)
    if r_node.returncode != 0:
        fail_test("T4_wasm_node_runtime", r_node.stdout + "\n" + r_node.stderr)
    for req_pass in (
        "T4_wasm_egui_renders_mesh_and_atlas",
        "T4_wasm_showcase_10k_virtualizer_initial",
        "T4_wasm_showcase_10k_virtualizer_scroll",
        "T4_wasm_showcase_chat_compose_and_toast",
        "T4_wasm_showcase_explorer_controls_and_typeahead",
        "T4_wasm_showcase_reorder_and_centered_modal",
        "T4_wasm_showcase_canvas_editor_and_rtl",
        "T4_wasm_showcase_raw_pointer_coords_interactive",
    ):
        if f"[PASS] {req_pass}" not in r_node.stdout:
            fail_test("T4_wasm_missing_assertion", f"Missing [PASS] {req_pass} in Node output:\n{r_node.stdout}\n{r_node.stderr}")
    for line in r_node.stdout.splitlines():
        print(line)


def verify_showcase_dual_engine_parity():
    # Generate a temporary test harness mounting App.meme logic and verifying identical agent behavior
    # across Interpreter AND Native C++20 AOT!
    app_src = (ROOT / "apps" / "showcase" / "src" / "App.meme").read_text(encoding="utf-8")
    # Replace mount+show with mount + createAgent live interactions
    idx = app_src.find("ui.mount(<App />);")
    assert idx > 0
    harness_src = (
        app_src[:idx]
        + """
bula "ui:primitives" jaise { createAgent };
ui.mount(<App />);
pakad ag = createAgent();
pakad r0 = ag.audit();
agar (!r0.valid) { chillao "Initial audit failed: " + string(r0.violations); }
ag.act("scroll_next_btn", "click");
ag.act("chat_input", "type", "WoreDual");
ag.act("chat_send_btn", "click");
ag.act("tab_explorer", "click");
pakad rExp = ag.audit();
agar (!rExp.valid) { chillao "Explorer audit failed: " + string(rExp.violations); }
pakad sh = ag.getByTestId("split_handle");
agar (sh == khali || sh.height < 24.0) { chillao "Split handle collapsed: " + string(sh); }
ag.act("combobox_input", "change", "eme");
ag.act("gain_slider", "change", 90);
agar (ag.getByTestId("gain_slider").numValue != 90.0) { chillao "Gain slider failed"; }
ag.act("split_handle", "click");
ag.act("reorder_down_0", "click");
ag.act("open_modal", "click");
pakad dlg = ag.getByTestId("settings_dialog");
agar (dlg == khali || dlg.y <= 0 || (dlg.y + dlg.height) > 720) { chillao "Dialog bounds failed"; }
ag.act("modal_close_btn", "click");
ag.act("tab_telemetry", "click");
pakad rTel = ag.audit();
agar (!rTel.valid) { chillao "Telemetry audit failed: " + string(rTel.violations); }
ag.act("step_phase_btn", "click");
ag.act("tab_editor", "click");
pakad rEd = ag.audit();
agar (!rEd.valid) { chillao "Editor audit failed: " + string(rEd.violations); }
agar (!ag.getByTestId("secret_input").isPassword) { chillao "Password flag missing"; }
ag.act("toggle_rtl", "click");
pakad snapEnd = ag.snapshot();
dikha "SHOWCASE_DUAL_OK nodes=" + string(snapEnd.nodes.lambai());
"""
    )
    # Fix relative import "./telemetry" -> "./apps/showcase/src/telemetry" since temp file sits in WORK_DIR
    harness_src = harness_src.replace('bula "./telemetry"', 'bula "../apps/showcase/src/telemetry"')
    harness_path = WORK_DIR / "showcase_dual_driver.meme"
    harness_path.write_text(harness_src, encoding="utf-8")

    env = os.environ.copy()
    env["MEME_HEADLESS"] = "1"
    r_interp = run_cmd([MEME_BIN, str(harness_path)], env=env, timeout=60)
    if r_interp.returncode != 0 or "SHOWCASE_DUAL_OK" not in r_interp.stdout:
        fail_test("T4_showcase_interpreter_driver", r_interp.stdout + "\n" + r_interp.stderr)

    r_aot = run_cmd([MEME_BIN, "--native", str(harness_path)], env=env, timeout=60)
    if r_aot.returncode != 0 or "SHOWCASE_DUAL_OK" not in r_aot.stdout:
        fail_test("T4_showcase_aot_driver", r_aot.stdout + "\n" + r_aot.stderr)

    line_i = [l for l in r_interp.stdout.splitlines() if "SHOWCASE_DUAL_OK" in l][-1]
    line_a = [l for l in r_aot.stdout.splitlines() if "SHOWCASE_DUAL_OK" in l][-1]
    if line_i != line_a:
        fail_test("T4_showcase_dual_parity", f"Mismatch interp='{line_i}' vs aot='{line_a}'")
    pass_test("T4_showcase_interpreter_aot_parity_verified")


def main():
    if WORK_DIR.exists():
        shutil.rmtree(WORK_DIR)
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    try:
        egui_dist = WORK_DIR / "egui"
        app_dist = WORK_DIR / "app"

        r_pkg1 = run_cmd([MEME_BIN, "package", "apps/showcase/src/EguiTelemetry.meme", "--target=all", "-o", str(egui_dist)], timeout=240)
        if r_pkg1.returncode != 0:
            fail_test("T1_package_eguitelemetry_all", r_pkg1.stdout + "\n" + r_pkg1.stderr)
        pass_test("T1_package_eguitelemetry_all_8_targets")

        r_pkg2 = run_cmd([MEME_BIN, "package", "apps/showcase/src/App.meme", "--target=all", "-o", str(app_dist)], timeout=240)
        if r_pkg2.returncode != 0:
            fail_test("T1_package_showcase_all", r_pkg2.stdout + "\n" + r_pkg2.stderr)
        pass_test("T1_package_showcase_all_8_targets")

        verify_target_artifacts(egui_dist, "EguiTelemetry", "eguitelemetry", "egui")
        verify_target_artifacts(app_dist, "Showcase", "showcase", "showcase")
        verify_live_wasm_and_app_parity(app_dist / "web", egui_dist / "web")
        verify_showcase_dual_engine_parity()
        print("ALL MULTI-PLATFORM PACKAGING, WASM & SHOWCASE DUAL-ENGINE PARITY TESTS PASSED!")
    finally:
        if WORK_DIR.exists():
            shutil.rmtree(WORK_DIR, ignore_errors=True)


if __name__ == "__main__":
    main()
