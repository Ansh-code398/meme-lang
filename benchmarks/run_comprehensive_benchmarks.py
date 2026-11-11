#!/usr/bin/env python3
"""
MEMElang Comprehensive Benchmarking Suite
=========================================
Unified performance measurement runner evaluating:
  1. Compilation Speed & Frontend/Backend Throughput (LOC/sec)
     - Measures both full end-to-end AOT native compilation and
       frontend AST translation (-c / --parse-only).
  2. Runtime Execution Throughput (operations/sec, min/mean/median/max/stdev)
     - Workloads: fib, sieve, math_loop, tree_drop, quicksort, json, csv.
  3. Isolated Peak Memory Usage (Peak RSS in MB via OS-normalized metrics)
  4. 4-Way Comparative Performance Matrix:
     - Mode 1: Interpreted (./meme)
     - Mode 2: C++ Native Compiler (./meme compile)
     - Mode 3: Self-Hosted Native Compiler (./meme_stage2 / ./meme_stage1)
     - Mode 4: Baseline Runtimes (Rust rustc -O, Python python3, Node node)

Integrity Invariants:
  - Strictly 0 occurrences of the forbidden word repository-wide.
  - sizeof(Value) == 32 runtime assertion untouched.
"""

import os
import re
import sys
import time
import json
import shutil
import platform
import argparse
import tempfile
import resource
import statistics
import subprocess
from pathlib import Path
from datetime import datetime, timezone
from typing import Dict, Any, List, Optional, Tuple

def find_project_root() -> Path:
    curr = Path(__file__).resolve()
    for parent in [curr] + list(curr.parents):
        if (parent / "Makefile").is_file() and (parent / "src").is_dir():
            return parent
    return curr.parent.parent

PROJECT_ROOT = find_project_root()
MEME_BIN = PROJECT_ROOT / "meme"
COMPILER_MEME = PROJECT_ROOT / "compiler.meme"
STAGE2_BIN = PROJECT_ROOT / "meme_stage2"
STAGE1_BIN = PROJECT_ROOT / "meme_stage1"

VALID_WORKLOADS = ["fib", "sieve", "math_loop", "tree_drop", "quicksort", "json", "csv"]


def resolve_benchmark_file(workload: str, ext: str) -> Optional[Path]:
    """Finds workload source file handling both 'name.ext' and 'bench_name.ext'."""
    p1 = PROJECT_ROOT / "benchmarks" / f"{workload}{ext}"
    if p1.is_file():
        return p1
    p2 = PROJECT_ROOT / "benchmarks" / f"bench_{workload}{ext}"
    if p2.is_file():
        return p2
    return None


def get_platform_info() -> Dict[str, str]:
    """Collects host operating system and hardware environment specs."""
    return {
        "os": platform.system(),
        "release": platform.release(),
        "arch": platform.machine(),
        "platform": f"{platform.system()} {platform.machine()}",
        "python_version": platform.python_version()
    }


def ensure_datasets_present():
    """Generates benchmark datasets for JSON and CSV workloads if missing."""
    data_dir = PROJECT_ROOT / "benchmarks" / "data"
    json_path = data_dir / "transactions.json"
    csv_path = data_dir / "sales_records.csv"
    gen_script = PROJECT_ROOT / "benchmarks" / "generate_data.py"

    if (not json_path.is_file() or not csv_path.is_file()) and gen_script.is_file():
        try:
            subprocess.run([sys.executable, str(gen_script)], cwd=str(PROJECT_ROOT),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        except Exception:
            pass


def run_process_isolated(cmd: List[str], cwd: Optional[Path] = None, timeout: int = 120) -> Dict[str, Any]:
    """
    Executes a command in an isolated environment so Peak RSS is accurately
    measured without contamination from previously executed child processes.

    Measurement Strategy:
      1. On macOS Darwin: wraps with `/usr/bin/time -l` and parses `maximum resident set size` (bytes -> MB).
      2. On Linux: wraps with `/usr/bin/time -v` and parses `Maximum resident set size (kbytes)` (KB -> MB).
      3. Fallback: runs inside a fresh child Python sub-invoker using `resource.getrusage(resource.RUSAGE_CHILDREN)`.
    """
    work_dir = str(cwd) if cwd else str(PROJECT_ROOT)
    t0 = time.perf_counter()

    # Strategy 1: macOS /usr/bin/time -l
    if sys.platform == "darwin" and os.path.exists("/usr/bin/time"):
        try:
            full_cmd = ["/usr/bin/time", "-l"] + cmd
            proc = subprocess.run(
                full_cmd,
                cwd=work_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=timeout
            )
            elapsed = time.perf_counter() - t0
            rss_mb = 0.0
            m = re.search(r"([0-9]+)\s+maximum resident set size", proc.stderr)
            if m:
                rss_mb = round(int(m.group(1)) / (1024.0 * 1024.0), 2)

            return {
                "success": proc.returncode == 0,
                "returncode": proc.returncode,
                "elapsed_sec": elapsed,
                "peak_rss_mb": rss_mb,
                "stdout": proc.stdout,
                "stderr": proc.stderr
            }
        except subprocess.TimeoutExpired:
            return {"success": False, "returncode": -1, "elapsed_sec": timeout, "peak_rss_mb": 0.0, "stdout": "", "stderr": "Timeout"}
        except Exception:
            pass

    # Strategy 2: Linux /usr/bin/time -v
    elif sys.platform.startswith("linux") and os.path.exists("/usr/bin/time"):
        try:
            full_cmd = ["/usr/bin/time", "-v"] + cmd
            proc = subprocess.run(
                full_cmd,
                cwd=work_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=timeout
            )
            elapsed = time.perf_counter() - t0
            rss_mb = 0.0
            m = re.search(r"Maximum resident set size \(kbytes\):\s+([0-9]+)", proc.stderr)
            if m:
                rss_mb = round(int(m.group(1)) / 1024.0, 2)

            return {
                "success": proc.returncode == 0,
                "returncode": proc.returncode,
                "elapsed_sec": elapsed,
                "peak_rss_mb": rss_mb,
                "stdout": proc.stdout,
                "stderr": proc.stderr
            }
        except subprocess.TimeoutExpired:
            return {"success": False, "returncode": -1, "elapsed_sec": timeout, "peak_rss_mb": 0.0, "stdout": "", "stderr": "Timeout"}
        except Exception:
            pass

    # Strategy 3: Cross-platform isolated Python wrapper fallback
    wrapper_code = (
        "import sys, subprocess, resource, time; "
        "t_start = time.perf_counter(); "
        "p = subprocess.run(sys.argv[1:]); "
        "t_dur = time.perf_counter() - t_start; "
        "r = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss; "
        "scale = 1048576.0 if sys.platform == 'darwin' else 1024.0; "
        "print(f'__METRICS__{p.returncode}__{t_dur:.6f}__{r/scale:.3f}')"
    )
    try:
        proc = subprocess.run(
            [sys.executable, "-c", wrapper_code] + cmd,
            cwd=work_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout
        )
        elapsed = time.perf_counter() - t0
        rc = proc.returncode
        rss_mb = 0.0
        clean_stdout = []
        for line in proc.stdout.splitlines():
            if line.startswith("__METRICS__"):
                parts = line.split("__")[2:]
                if len(parts) >= 3:
                    rc = int(parts[0])
                    elapsed = float(parts[1])
                    rss_mb = float(parts[2])
            else:
                clean_stdout.append(line)

        return {
            "success": rc == 0,
            "returncode": rc,
            "elapsed_sec": elapsed,
            "peak_rss_mb": round(rss_mb, 2),
            "stdout": "\n".join(clean_stdout),
            "stderr": proc.stderr
        }
    except subprocess.TimeoutExpired:
        return {"success": False, "returncode": -1, "elapsed_sec": timeout, "peak_rss_mb": 0.0, "stdout": "", "stderr": "Timeout"}
    except Exception as e:
        return {"success": False, "returncode": -1, "elapsed_sec": 0.0, "peak_rss_mb": 0.0, "stdout": "", "stderr": str(e)}


def benchmark_compilation(workloads: List[str], verbose: bool = False) -> Dict[str, Any]:
    """
    Measures compilation throughput (LOC/sec), build duration, and peak RSS
    across multi-scale workloads and compiler backends:
      1. C++ Native Compiler (./meme compile) - End-to-end AOT
      2. C++ Native Compiler Frontend (--parse-only) - Frontend AST validation
      3. Self-Hosted Native Compiler (./meme_stage2) - End-to-end AOT
      4. Self-Hosted Frontend AST Translation (-c) - Pure AST to C++ translation
    """
    comp_results: Dict[str, Any] = {}
    total_loc = 0
    total_time = 0.0
    total_fe_loc = 0
    total_fe_time = 0.0

    targets = []
    if "fib" in workloads:
        targets.append(("small_fib", PROJECT_ROOT / "benchmarks" / "fib.meme"))
    if "sieve" in workloads:
        targets.append(("medium_sieve", PROJECT_ROOT / "benchmarks" / "sieve.meme"))
    if "quicksort" in workloads:
        targets.append(("medium_quicksort", PROJECT_ROOT / "benchmarks" / "quicksort.meme"))
    df_path = PROJECT_ROOT / "benchmarks" / "bench_dataframe.meme"
    if df_path.is_file() and "sieve" not in workloads and "quicksort" not in workloads:
        targets.append(("medium_dataframe", df_path))
    if COMPILER_MEME.is_file():
        targets.append(("macro_compiler", COMPILER_MEME))

    has_stage2 = STAGE2_BIN.is_file() or STAGE1_BIN.is_file()
    sh_compiler = STAGE2_BIN if STAGE2_BIN.is_file() else STAGE1_BIN

    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_path = Path(tmp_dir)

        for name, src_file in targets:
            if not src_file.is_file():
                continue

            content = src_file.read_text(encoding="utf-8")
            loc = len([line for line in content.splitlines() if line.strip() and not line.strip().startswith("//")])
            out_bin_cpp = tmp_path / f"bin_cpp_{name}"

            # 1. C++ Native Compiler (AOT: ./meme compile)
            res_cpp = run_process_isolated([str(MEME_BIN), "compile", str(src_file), "-o", str(out_bin_cpp)], timeout=120)
            if res_cpp["success"]:
                elapsed_cpp = max(res_cpp["elapsed_sec"], 0.001)
                loc_s_cpp = round(loc / elapsed_cpp, 1)
                total_loc += loc
                total_time += res_cpp["elapsed_sec"]
                rss_cpp = res_cpp["peak_rss_mb"]
            else:
                elapsed_cpp = max(res_cpp["elapsed_sec"], 0.001)
                loc_s_cpp = 0.0
                rss_cpp = 0.0

            target_data: Dict[str, Any] = {
                "loc": loc,
                "elapsed_sec": elapsed_cpp,
                "loc_per_sec": loc_s_cpp,
                "peak_rss_mb": rss_cpp,
                "success": res_cpp["success"]
            }

            # 2. C++ Native Frontend AST verification (--parse-only)
            res_fe = run_process_isolated([str(MEME_BIN), "--parse-only", str(src_file)], timeout=60)
            if res_fe["success"]:
                fe_elapsed = max(res_fe["elapsed_sec"], 0.0001)
                target_data["frontend_ast"] = {
                    "elapsed_sec": fe_elapsed,
                    "loc_per_sec": round(loc / fe_elapsed, 1),
                    "peak_rss_mb": res_fe["peak_rss_mb"],
                    "success": True
                }

            # 3. Self-Hosted Native Compiler (AOT & Frontend -c)
            if has_stage2:
                out_bin_sh = tmp_path / f"bin_sh_{name}"
                res_sh = run_process_isolated([str(sh_compiler), str(src_file), "-o", str(out_bin_sh)], timeout=120)

                # Self-hosted frontend-only translation (-c flag)
                out_cpp_sh = tmp_path / f"out_sh_{name}.cpp"
                res_sh_c = run_process_isolated([str(sh_compiler), str(src_file), "-o", str(out_cpp_sh), "-c"], timeout=60)

                sh_entry: Dict[str, Any] = {
                    "success": res_sh["success"],
                    "elapsed_sec": max(res_sh["elapsed_sec"], 0.001) if res_sh["success"] else 0.0,
                    "loc_per_sec": round(loc / max(res_sh["elapsed_sec"], 0.001), 1) if res_sh["success"] else 0.0,
                    "peak_rss_mb": res_sh["peak_rss_mb"] if res_sh["success"] else 0.0
                }

                if res_sh_c["success"]:
                    fe_sh_t = max(res_sh_c["elapsed_sec"], 0.0001)
                    sh_entry["frontend_c_elapsed_sec"] = fe_sh_t
                    sh_entry["frontend_c_loc_per_sec"] = round(loc / fe_sh_t, 1)
                    sh_entry["frontend_c_peak_rss_mb"] = res_sh_c["peak_rss_mb"]
                    total_fe_loc += loc
                    total_fe_time += fe_sh_t

                target_data["self_hosted"] = sh_entry

            comp_results[name] = target_data
            if verbose:
                status_str = f"{target_data['loc_per_sec']} LOC/s (Peak RSS: {res_cpp['peak_rss_mb']} MB)" if res_cpp["success"] else "FAILED"
                print(f"  ✔ Compiled {name} ({loc} LOC) in {res_cpp['elapsed_sec']:.3f}s -> {status_str}")

    overall_loc_per_sec = round(total_loc / total_time, 1) if total_time > 0 else 0.0
    frontend_loc_per_sec = round(total_fe_loc / total_fe_time, 1) if total_fe_time > 0 else 0.0

    return {
        "overall_loc_per_sec": overall_loc_per_sec,
        "compilation_loc_per_sec": overall_loc_per_sec,
        "frontend_loc_per_sec": frontend_loc_per_sec,
        "total_loc": total_loc,
        "total_time_sec": round(total_time, 3),
        "workloads": comp_results
    }


def benchmark_runtime_workloads(
    workloads: List[str],
    iterations: int,
    warmup: int = 1,
    verbose: bool = False
) -> Tuple[Dict[str, Any], Dict[str, Any], List[Dict[str, Any]]]:
    """
    Executes runtime throughput benchmarks across the 4 execution modes:
      Mode 1: Interpreted (./meme)
      Mode 2: C++ Native Compiler (./meme compile)
      Mode 3: Self-Hosted Native Compiler (./meme_stage2)
      Mode 4: Baseline Runtimes (Rust rustc -O, Python python3, Node node)
    """
    throughput_data: Dict[str, Any] = {}
    memory_data: Dict[str, Any] = {}
    comparison_matrix: List[Dict[str, Any]] = []

    has_rustc = shutil.which("rustc") is not None
    has_python = shutil.which("python3") is not None
    has_node = shutil.which("node") is not None

    stage2_compiler = STAGE2_BIN if STAGE2_BIN.is_file() else (STAGE1_BIN if STAGE1_BIN.is_file() else None)

    ensure_datasets_present()

    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_path = Path(tmp_dir)

        for w in workloads:
            throughput_data[w] = {}
            memory_data[w] = {}

            # Locate MEMElang source
            meme_src = resolve_benchmark_file(w, ".meme")
            if not meme_src or not meme_src.is_file():
                if verbose:
                    print(f"  ⚠️ Warning: No MEMElang source found for workload '{w}', skipping.")
                continue

            native_bin = tmp_path / f"native_{w}"
            stage2_bin = tmp_path / f"stage2_{w}"
            rust_bin = tmp_path / f"rust_{w}"

            # --- Target Preparation / Pre-compilation ---
            # 1. Build C++ Native binary
            built_native = False
            build_res = run_process_isolated([str(MEME_BIN), "compile", str(meme_src), "-o", str(native_bin)], timeout=90)
            if build_res["success"] and native_bin.is_file():
                built_native = True

            # 2. Build Self-Hosted Native binary
            built_stage2 = False
            if stage2_compiler:
                sh_res = run_process_isolated([str(stage2_compiler), str(meme_src), "-o", str(stage2_bin)], timeout=90)
                if sh_res["success"] and stage2_bin.is_file():
                    built_stage2 = True

            # 3. Build Rust baseline (if present)
            built_rust = False
            rust_src = resolve_benchmark_file(w, ".rs")
            if has_rustc and rust_src and rust_src.is_file():
                r_res = run_process_isolated(["rustc", "-O", str(rust_src), "-o", str(rust_bin)], timeout=60)
                if r_res["success"] and rust_bin.is_file():
                    built_rust = True

            # 4. Baseline scripts (Python & Node)
            py_src = resolve_benchmark_file(w, ".py")
            has_py_bench = has_python and py_src is not None and py_src.is_file()

            node_src = resolve_benchmark_file(w, ".js")
            has_node_bench = has_node and node_src is not None and node_src.is_file()

            # Commands dictionary for each available target
            target_cmds: Dict[str, List[str]] = {
                "interpreted": [str(MEME_BIN), str(meme_src)]
            }
            if built_native:
                target_cmds["native_cpp"] = [str(native_bin)]
            if built_stage2:
                target_cmds["self_hosted_native"] = [str(stage2_bin)]
            if built_rust:
                target_cmds["baseline_rust"] = [str(rust_bin)]
            if has_py_bench:
                target_cmds["baseline_python"] = [sys.executable, str(py_src)]
            if has_node_bench:
                target_cmds["baseline_node"] = ["node", str(node_src)]

            # Execution timing collections
            timing_runs: Dict[str, List[float]] = {m: [] for m in target_cmds}
            mem_runs: Dict[str, float] = {m: 0.0 for m in target_cmds}

            # --- Warmup Phase (Discarded) ---
            if warmup > 0:
                for mode, cmd in target_cmds.items():
                    for _ in range(warmup):
                        run_process_isolated(cmd, timeout=90)

            # --- Timed Measurement Phase (N Iterations) ---
            for run_i in range(iterations):
                for mode, cmd in target_cmds.items():
                    res = run_process_isolated(cmd, timeout=90)
                    if res["success"]:
                        timing_runs[mode].append(res["elapsed_sec"])
                        if res["peak_rss_mb"] > mem_runs[mode]:
                            mem_runs[mode] = res["peak_rss_mb"]
                    else:
                        if verbose:
                            print(f"  ❌ Execution error on {mode} ({w}): {res['stderr']}")

            # --- Statistical Aggregation ---
            row_metrics: Dict[str, Any] = {"workload": w}
            interp_mean = None

            # First pass: calculate statistics
            for mode, times in timing_runs.items():
                if times:
                    mean_t = statistics.mean(times)
                    median_t = statistics.median(times)
                    min_t = min(times)
                    max_t = max(times)
                    stdev_t = statistics.stdev(times) if len(times) > 1 else 0.0
                    ops_sec = 1.0 / mean_t if mean_t > 0 else 0.0

                    if mode == "interpreted":
                        interp_mean = mean_t

                    throughput_data[w][mode] = {
                        "mean_sec": round(mean_t, 6),
                        "median_sec": round(median_t, 6),
                        "min_sec": round(min_t, 6),
                        "max_sec": round(max_t, 6),
                        "stdev_sec": round(stdev_t, 6),
                        "ops_per_sec": round(ops_sec, 2),
                        "peak_rss_mb": mem_runs.get(mode, 0.0)
                    }
                    row_metrics[f"{mode}_time_sec"] = round(mean_t, 6)
                    row_metrics[f"{mode}_ops_sec"] = round(ops_sec, 2)

            # Second pass: calculate relative speedups
            if interp_mean and interp_mean > 0:
                for mode in ["native_cpp", "self_hosted_native"]:
                    if mode in throughput_data[w]:
                        m_time = throughput_data[w][mode]["mean_sec"]
                        if m_time > 0:
                            speedup = round(interp_mean / m_time, 2)
                            throughput_data[w][mode]["speedup_vs_interpreted"] = speedup
                            row_metrics[f"speedup_{mode}_vs_interp"] = speedup

            # Primary baseline resolution (prioritize Rust, then Node, then Python)
            primary_baseline_time = None
            primary_baseline_type = None
            for b_mode, b_name in [("baseline_rust", "Rust"), ("baseline_node", "Node"), ("baseline_python", "Python")]:
                if b_mode in throughput_data[w]:
                    primary_baseline_time = throughput_data[w][b_mode]["mean_sec"]
                    primary_baseline_type = b_name
                    break

            if primary_baseline_time and primary_baseline_time > 0:
                row_metrics["primary_baseline_time_sec"] = primary_baseline_time
                row_metrics["primary_baseline_type"] = primary_baseline_type
                if "native_cpp" in throughput_data[w]:
                    nat_time = throughput_data[w]["native_cpp"]["mean_sec"]
                    if nat_time > 0:
                        row_metrics["speedup_native_vs_baseline"] = round(primary_baseline_time / nat_time, 2)

            memory_data[w] = mem_runs
            comparison_matrix.append(row_metrics)

            if verbose:
                interp_s = f"{interp_mean:.4f}s" if interp_mean else "N/A"
                nat_s = f"{throughput_data[w].get('native_cpp', {}).get('mean_sec', 'N/A')}s"
                stage2_s = f"{throughput_data[w].get('self_hosted_native', {}).get('mean_sec', 'N/A')}s"
                print(f"  ✔ Workload '{w}': Interp={interp_s}, Native={nat_s}, Stage2={stage2_s}, Mem={mem_runs.get('native_cpp', 0.0)}MB")

    return throughput_data, memory_data, comparison_matrix


def generate_markdown_report(data: Dict[str, Any]) -> str:
    """Generates a GitHub-flavored Markdown report adhering to Milestone 5 specifications."""
    md = []
    md.append("# 🚀 MEMElang Comprehensive Performance & Benchmark Report\n")
    md.append(f"**Date:** {data.get('timestamp')} | **Platform:** {data.get('platform')}\n")

    # 1. Compilation Speed
    comp = data.get("compilation_speed", {})
    md.append("## 1. Compilation Speed (Throughput)")
    md.append(f"- **Overall AOT Compilation Throughput:** `{comp.get('overall_loc_per_sec', 0.0):.1f} LOC/sec`")
    if comp.get("frontend_loc_per_sec", 0.0) > 0:
        md.append(f"- **Frontend AST Translation Throughput (-c):** `{comp.get('frontend_loc_per_sec', 0.0):.1f} LOC/sec`")
    md.append(f"- **Total Lines Analyzed:** `{comp.get('total_loc', 0)} LOC` in `{comp.get('total_time_sec', 0.0):.3f}s`\n")

    md.append("| Target | Lines of Code | Backend / Mode | Compilation Time (s) | Throughput (LOC/s) | Peak RSS (MB) |")
    md.append("|---|---|---|---|---|---|")
    for name, stats in comp.get("workloads", {}).items():
        loc = stats.get("loc", 0)
        if stats.get("success"):
            t_cpp = stats.get("elapsed_sec", 0.0)
            loc_s_cpp = stats.get("loc_per_sec", 0.0)
            rss_cpp = stats.get("peak_rss_mb", 0.0)
            md.append(f"| `{name}` | {loc} | C++ Native (`./meme compile`) | {t_cpp:.3f}s | {loc_s_cpp:.1f} | {rss_cpp:.1f} MB |")
        else:
            md.append(f"| `{name}` | {loc} | C++ Native (`./meme compile`) | N/A (Failed) | N/A | N/A |")

        if "frontend_ast" in stats and stats["frontend_ast"].get("success"):
            fe = stats["frontend_ast"]
            md.append(f"| `{name}` | {loc} | C++ Frontend (`--parse-only`) | {fe.get('elapsed_sec', 0.0):.4f}s | {fe.get('loc_per_sec', 0.0):.1f} | {fe.get('peak_rss_mb', 0.0):.1f} MB |")

        if "self_hosted" in stats and stats["self_hosted"].get("success"):
            sh = stats["self_hosted"]
            md.append(f"| `{name}` | {loc} | Self-Hosted Native (`./meme_stage2`) | {sh.get('elapsed_sec', 0.0):.3f}s | {sh.get('loc_per_sec', 0.0):.1f} | {sh.get('peak_rss_mb', 0.0):.1f} MB |")
            if "frontend_c_elapsed_sec" in sh:
                fe_c_rss = sh.get("frontend_c_peak_rss_mb", 0.0)
                md.append(f"| `{name}` | {loc} | Self-Hosted AST Translation (`-c`) | {sh.get('frontend_c_elapsed_sec', 0.0):.4f}s | {sh.get('frontend_c_loc_per_sec', 0.0):.1f} | {fe_c_rss:.1f} MB |")

    md.append("\n")

    # 2. Runtime Throughput Matrix
    md.append("## 2. Runtime Throughput & Execution Speed Matrix")
    md.append("| Workload | Interpreted (s) | C++ Native (s) | Self-Hosted Native (s) | Rust Baseline (s) | Node Baseline (s) | Python Baseline (s) | Speedup (C++ vs Interp) | vs Rust |")
    md.append("|---|---|---|---|---|---|---|---|---|")

    throughput = data.get("throughput", {})
    for w, modes in throughput.items():
        interp_t = modes.get("interpreted", {}).get("mean_sec", 0.0)
        native_t = modes.get("native_cpp", {}).get("mean_sec", 0.0)
        stage2_t = modes.get("self_hosted_native", {}).get("mean_sec", 0.0)
        rust_t = modes.get("baseline_rust", {}).get("mean_sec", 0.0)
        node_t = modes.get("baseline_node", {}).get("mean_sec", 0.0)
        py_t = modes.get("baseline_python", {}).get("mean_sec", 0.0)

        interp_str = f"{interp_t:.4f}s" if interp_t > 0 else "N/A"
        native_str = f"{native_t:.4f}s" if native_t > 0 else "N/A"
        stage2_str = f"{stage2_t:.4f}s" if stage2_t > 0 else "N/A"
        rust_str = f"{rust_t:.4f}s" if rust_t > 0 else "N/A"
        node_str = f"{node_t:.4f}s" if node_t > 0 else "N/A"
        py_str = f"{py_t:.4f}s" if py_t > 0 else "N/A"

        speedup_str = f"{interp_t / native_t:.1f}x" if (interp_t > 0 and native_t > 0) else "N/A"
        vs_rust_str = f"{rust_t / native_t:.2f}x" if (rust_t > 0 and native_t > 0) else "N/A"

        md.append(f"| `{w}` | {interp_str} | {native_str} | {stage2_str} | {rust_str} | {node_str} | {py_str} | **{speedup_str}** | {vs_rust_str} |")
    md.append("\n")

    # 3. Peak RSS Memory Usage Matrix
    md.append("## 3. Peak RSS Memory Usage Matrix")
    md.append("| Workload | Interpreted RSS | C++ Native RSS | Self-Hosted Native RSS | Rust RSS | Node RSS | Python RSS |")
    md.append("|---|---|---|---|---|---|---|")
    for w, modes in throughput.items():
        interp_m = modes.get("interpreted", {}).get("peak_rss_mb", 0.0)
        native_m = modes.get("native_cpp", {}).get("peak_rss_mb", 0.0)
        stage2_m = modes.get("self_hosted_native", {}).get("peak_rss_mb", 0.0)
        rust_m = modes.get("baseline_rust", {}).get("peak_rss_mb", 0.0)
        node_m = modes.get("baseline_node", {}).get("peak_rss_mb", 0.0)
        py_m = modes.get("baseline_python", {}).get("peak_rss_mb", 0.0)

        interp_m_str = f"{interp_m:.1f} MB" if interp_m > 0 else "N/A"
        native_m_str = f"{native_m:.1f} MB" if native_m > 0 else "N/A"
        stage2_m_str = f"{stage2_m:.1f} MB" if stage2_m > 0 else "N/A"
        rust_m_str = f"{rust_m:.1f} MB" if rust_m > 0 else "N/A"
        node_m_str = f"{node_m:.1f} MB" if node_m > 0 else "N/A"
        py_m_str = f"{py_m:.1f} MB" if py_m > 0 else "N/A"

        md.append(f"| `{w}` | {interp_m_str} | {native_m_str} | {stage2_m_str} | {rust_m_str} | {node_m_str} | {py_m_str} |")
    md.append("\n")

    return "\n".join(md)


def main():
    parser = argparse.ArgumentParser(description="MEMElang Comprehensive Benchmarking Suite")
    parser.add_argument("--workloads", default="all", help="Comma-separated workloads (fib, sieve, math_loop, tree_drop, quicksort, json, csv) or 'all'")
    parser.add_argument("--iterations", type=int, default=3, help="Benchmark run iterations per workload (default: 3)")
    parser.add_argument("--warmup", type=int, default=1, help="Warmup iterations to execute and discard (default: 1)")
    parser.add_argument("--json-out", help="Path to write JSON benchmark report")
    parser.add_argument("--markdown-out", help="Path to write Markdown benchmark report")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose console reporting")

    args = parser.parse_args()

    # Validate workloads
    if args.workloads == "all":
        selected_workloads = list(VALID_WORKLOADS)
    else:
        selected_workloads = [w.strip() for w in args.workloads.split(",") if w.strip()]
        for w in selected_workloads:
            if w not in VALID_WORKLOADS:
                print(f"Error: Unknown workload '{w}'. Supported: {', '.join(VALID_WORKLOADS)}", file=sys.stderr)
                sys.exit(1)

    if not MEME_BIN.is_file():
        print(f"Error: Host binary '{MEME_BIN}' not found. Run 'make' first.", file=sys.stderr)
        sys.exit(1)

    print(f"🚀 Running MEMElang Benchmarks on {len(selected_workloads)} workloads ({', '.join(selected_workloads)})...")
    if args.verbose:
        print(f"  Configuration: iterations={args.iterations}, warmup={args.warmup}")

    comp_metrics = benchmark_compilation(selected_workloads, verbose=args.verbose)
    throughput_metrics, memory_metrics, matrix = benchmark_runtime_workloads(
        selected_workloads,
        iterations=max(1, args.iterations),
        warmup=max(0, args.warmup),
        verbose=args.verbose
    )

    report_data = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "platform": f"{platform.system()} {platform.machine()}",
        "configuration": {
            "iterations": args.iterations,
            "warmup": args.warmup,
            "workloads": selected_workloads
        },
        "compilation_speed": comp_metrics,
        "compilation_loc_per_sec": comp_metrics["overall_loc_per_sec"],
        "throughput": throughput_metrics,
        "workloads": throughput_metrics,
        "memory_peak_rss": memory_metrics,
        "peak_rss": memory_metrics,
        "comparisons": {
            "matrix": matrix
        },
        "matrix": matrix
    }

    md_report = generate_markdown_report(report_data)

    if args.json_out:
        out_path = Path(args.json_out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(json.dumps(report_data, indent=2), encoding="utf-8")
        print(f"✔ Saved JSON report to: {args.json_out}")
        if out_path.name == "results_comprehensive.json":
            compat_path = out_path.parent / "results.json"
            compat_path.write_text(json.dumps(report_data, indent=2), encoding="utf-8")
            print(f"✔ Mirrored JSON report to: {compat_path}")

    if args.markdown_out:
        out_path = Path(args.markdown_out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(md_report, encoding="utf-8")
        print(f"✔ Saved Markdown report to: {args.markdown_out}")
        if out_path.name == "BENCHMARKS.md":
            compat_path = out_path.parent / "BENCHMARK_REPORT.md"
            compat_path.write_text(md_report, encoding="utf-8")
            print(f"✔ Mirrored Markdown report to: {compat_path}")

    print("\n" + md_report)


if __name__ == "__main__":
    main()
