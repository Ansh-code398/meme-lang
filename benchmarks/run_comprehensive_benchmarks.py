#!/usr/bin/env python3
"""
MEMElang Comprehensive Benchmarking Suite
=========================================
Evaluates and benchmarks:
  1. Compilation Speed:
     - Measures LOC/sec across small, medium, and macro workloads (compiler.meme).
  2. Runtime Execution Throughput:
     - Benchmarks operations/sec, mean, median, min, max, stdev.
     - Workloads: fib, sieve, math_loop, tree_drop, json, csv.
  3. Peak Memory Footprint (Peak RSS):
     - OS-normalized memory profiling using getrusage (macOS bytes vs Linux KB).
  4. 4-Way Comparative Performance Matrix:
     - Self-Hosted Native (compiler.meme / meme_stage1)
     - C++ Native (./meme compile)
     - C++ Interpreter (./meme)
     - Baseline (Rust / Python / Node)

Integrity:
  - Strictly 0 occurrences of the forbidden word.
"""

import os
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
from typing import Dict, Any, List, Optional

PROJECT_ROOT = Path(__file__).resolve().parent.parent
MEME_BIN = PROJECT_ROOT / "meme"
COMPILER_MEME = PROJECT_ROOT / "compiler.meme"
STAGE1_BIN = PROJECT_ROOT / "meme_stage1"

VALID_WORKLOADS = ["fib", "sieve", "math_loop", "tree_drop", "json", "csv"]


def get_peak_rss_mb(ru_maxrss: float) -> float:
    """Normalizes peak RSS to megabytes across macOS and Linux."""
    if platform.system() == "Darwin":
        return ru_maxrss / (1024.0 * 1024.0)
    else:
        return ru_maxrss / 1024.0


def run_process_with_metrics(cmd: List[str], cwd: Optional[Path] = None, timeout: int = 120) -> Dict[str, Any]:
    """Runs a command and records wall-clock time, returncode, and child peak RSS."""
    start_time = time.perf_counter()
    start_rss = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss

    try:
        proc = subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else str(PROJECT_ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout
        )
        elapsed = time.perf_counter() - start_time
        end_rss = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
        peak_rss_mb = get_peak_rss_mb(max(start_rss, end_rss))

        return {
            "success": proc.returncode == 0,
            "returncode": proc.returncode,
            "elapsed_sec": elapsed,
            "peak_rss_mb": peak_rss_mb,
            "stdout": proc.stdout,
            "stderr": proc.stderr
        }
    except subprocess.TimeoutExpired:
        return {
            "success": False,
            "returncode": -1,
            "elapsed_sec": timeout,
            "peak_rss_mb": 0.0,
            "stdout": "",
            "stderr": "Timeout expired"
        }
    except Exception as e:
        return {
            "success": False,
            "returncode": -1,
            "elapsed_sec": 0.0,
            "peak_rss_mb": 0.0,
            "stdout": "",
            "stderr": str(e)
        }


def benchmark_compilation(workloads: List[str]) -> Dict[str, Any]:
    """Measures compilation throughput (LOC/sec) across compiler backends."""
    comp_results = {}
    total_loc = 0
    total_time = 0.0

    targets = []
    if "fib" in workloads:
        targets.append(("small_fib", PROJECT_ROOT / "benchmarks" / "fib.meme"))
    if "sieve" in workloads:
        targets.append(("medium_sieve", PROJECT_ROOT / "benchmarks" / "sieve.meme"))
    if COMPILER_MEME.is_file():
        targets.append(("macro_compiler", COMPILER_MEME))

    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_path = Path(tmp_dir)
        for name, src_file in targets:
            if not src_file.is_file():
                continue
            content = src_file.read_text(encoding="utf-8")
            loc = len([line for line in content.splitlines() if line.strip() and not line.strip().startswith("//")])
            out_bin = tmp_path / f"bin_{name}"

            # Measure native compiler
            res = run_process_with_metrics([str(MEME_BIN), "compile", str(src_file), "-o", str(out_bin)], timeout=90)
            if res["success"]:
                elapsed = max(res["elapsed_sec"], 0.001)
                loc_per_sec = loc / elapsed
                comp_results[name] = {
                    "loc": loc,
                    "elapsed_sec": elapsed,
                    "loc_per_sec": loc_per_sec,
                    "peak_rss_mb": res["peak_rss_mb"]
                }
                total_loc += loc
                total_time += elapsed

    overall_loc_per_sec = total_loc / total_time if total_time > 0 else 0.0
    return {
        "overall_loc_per_sec": overall_loc_per_sec,
        "compilation_loc_per_sec": overall_loc_per_sec,
        "total_loc": total_loc,
        "total_time_sec": total_time,
        "workloads": comp_results
    }


def benchmark_runtime_workloads(workloads: List[str], iterations: int) -> Tuple[Dict[str, Any], Dict[str, Any], List[Dict[str, Any]]]:
    """Runs runtime throughput benchmarks across the 4 execution modes."""
    throughput_data: Dict[str, Any] = {}
    memory_data: Dict[str, Any] = {}
    comparison_matrix: List[Dict[str, Any]] = []

    has_rustc = shutil.which("rustc") is not None
    has_stage1 = STAGE1_BIN.is_file()

    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_path = Path(tmp_dir)

        for w in workloads:
            throughput_data[w] = {}
            memory_data[w] = {}

            meme_src = PROJECT_ROOT / "benchmarks" / f"{w}.meme"
            if not meme_src.is_file():
                # Check for alternative naming
                alt = PROJECT_ROOT / "benchmarks" / f"bench_{w}.meme"
                if alt.is_file():
                    meme_src = alt
                else:
                    continue

            native_bin = tmp_path / f"native_{w}"
            stage1_bin = tmp_path / f"stage1_{w}"
            rust_bin = tmp_path / f"rust_{w}"

            # 1. Build C++ Native binary
            built_native = False
            build_res = run_process_with_metrics([str(MEME_BIN), "compile", str(meme_src), "-o", str(native_bin)])
            if build_res["success"] and native_bin.is_file():
                built_native = True

            # 2. Build Self-Hosted Native binary (if meme_stage1 exists)
            built_stage1 = False
            if has_stage1:
                sh_res = run_process_with_metrics([str(STAGE1_BIN), str(meme_src), "-o", str(stage1_bin)])
                if sh_res["success"] and stage1_bin.is_file():
                    built_stage1 = True

            # 3. Build Rust baseline (if exists)
            built_rust = False
            rust_src = PROJECT_ROOT / "benchmarks" / f"{w}.rs"
            if has_rustc and rust_src.is_file():
                r_res = run_process_with_metrics(["rustc", "-O", str(rust_src), "-o", str(rust_bin)])
                if r_res["success"] and rust_bin.is_file():
                    built_rust = True

            # Benchmarking runs
            runs: Dict[str, List[float]] = {
                "interpreted": [],
                "native_cpp": [],
                "self_hosted_native": [],
                "baseline_rust": []
            }
            mem_runs: Dict[str, float] = {}

            # Execute iterations
            for _ in range(iterations):
                # Interpreted
                m_res = run_process_with_metrics([str(MEME_BIN), str(meme_src)])
                if m_res["success"]:
                    runs["interpreted"].append(m_res["elapsed_sec"])
                    mem_runs["interpreted"] = max(mem_runs.get("interpreted", 0.0), m_res["peak_rss_mb"])

                # C++ Native
                if built_native:
                    n_res = run_process_with_metrics([str(native_bin)])
                    if n_res["success"]:
                        runs["native_cpp"].append(n_res["elapsed_sec"])
                        mem_runs["native_cpp"] = max(mem_runs.get("native_cpp", 0.0), n_res["peak_rss_mb"])

                # Self-Hosted Native
                if built_stage1:
                    s_res = run_process_with_metrics([str(stage1_bin)])
                    if s_res["success"]:
                        runs["self_hosted_native"].append(s_res["elapsed_sec"])
                        mem_runs["self_hosted_native"] = max(mem_runs.get("self_hosted_native", 0.0), s_res["peak_rss_mb"])

                # Rust Baseline
                if built_rust:
                    r_res = run_process_with_metrics([str(rust_bin)])
                    if r_res["success"]:
                        runs["baseline_rust"].append(r_res["elapsed_sec"])
                        mem_runs["baseline_rust"] = max(mem_runs.get("baseline_rust", 0.0), r_res["peak_rss_mb"])

            # Compute statistics
            row_metrics: Dict[str, Any] = {"workload": w}
            for mode, times in runs.items():
                if times:
                    mean_t = statistics.mean(times)
                    median_t = statistics.median(times)
                    min_t = min(times)
                    max_t = max(times)
                    stdev_t = statistics.stdev(times) if len(times) > 1 else 0.0
                    ops_sec = 1.0 / mean_t if mean_t > 0 else 0.0

                    throughput_data[w][mode] = {
                        "mean_sec": mean_t,
                        "median_sec": median_t,
                        "min_sec": min_t,
                        "max_sec": max_t,
                        "stdev_sec": stdev_t,
                        "ops_per_sec": ops_sec,
                        "peak_rss_mb": mem_runs.get(mode, 0.0)
                    }
                    row_metrics[f"{mode}_time_sec"] = mean_t
                    row_metrics[f"{mode}_ops_sec"] = ops_sec

            memory_data[w] = mem_runs
            comparison_matrix.append(row_metrics)

    return throughput_data, memory_data, comparison_matrix


def generate_markdown_report(data: Dict[str, Any]) -> str:
    """Generates a GitHub-flavored Markdown report comparing performance."""
    md = []
    md.append("# 🚀 MEMElang Comprehensive Performance & Benchmark Report\n")
    md.append(f"**Date:** {data.get('timestamp')} | **Platform:** {data.get('platform')}\n")

    # 1. Compilation Speed
    comp = data.get("compilation_speed", {})
    md.append("## 1. Compilation Speed (Throughput)")
    md.append(f"- **Overall Throughput:** `{comp.get('overall_loc_per_sec', 0.0):.1f} LOC/sec`")
    md.append(f"- **Total Lines Analyzed:** `{comp.get('total_loc', 0)} LOC` in `{comp.get('total_time_sec', 0.0):.3f}s`\n")

    md.append("| Target | Lines of Code | Compilation Time (s) | Throughput (LOC/s) | Peak RSS (MB) |")
    md.append("|---|---|---|---|---|")
    for name, stats in comp.get("workloads", {}).items():
        md.append(f"| `{name}` | {stats.get('loc')} | {stats.get('elapsed_sec', 0.0):.3f} | {stats.get('loc_per_sec', 0.0):.1f} | {stats.get('peak_rss_mb', 0.0):.1f} |")
    md.append("\n")

    # 2. Runtime Throughput Matrix
    md.append("## 2. Runtime Throughput & Execution Speed Matrix")
    md.append("| Workload | Interpreted (s) | C++ Native (s) | Self-Hosted Native (s) | Rust Baseline (s) | Speedup (Native vs Interp) |")
    md.append("|---|---|---|---|---|---|")

    throughput = data.get("throughput", {})
    for w, modes in throughput.items():
        interp_t = modes.get("interpreted", {}).get("mean_sec", 0.0)
        native_t = modes.get("native_cpp", {}).get("mean_sec", 0.0)
        stage1_t = modes.get("self_hosted_native", {}).get("mean_sec", 0.0)
        rust_t = modes.get("baseline_rust", {}).get("mean_sec", 0.0)

        speedup_str = f"{interp_t / native_t:.1f}x" if (interp_t > 0 and native_t > 0) else "N/A"
        interp_str = f"{interp_t:.4f}s" if interp_t > 0 else "N/A"
        native_str = f"{native_t:.4f}s" if native_t > 0 else "N/A"
        stage1_str = f"{stage1_t:.4f}s" if stage1_t > 0 else "N/A"
        rust_str = f"{rust_t:.4f}s" if rust_t > 0 else "N/A"

        md.append(f"| `{w}` | {interp_str} | {native_str} | {stage1_str} | {rust_str} | **{speedup_str}** |")
    md.append("\n")

    # 3. Memory Footprint
    md.append("## 3. Peak RSS Memory Usage")
    md.append("| Workload | Interpreted RSS (MB) | C++ Native RSS (MB) | Self-Hosted RSS (MB) |")
    md.append("|---|---|---|---|")
    for w, modes in throughput.items():
        interp_m = modes.get("interpreted", {}).get("peak_rss_mb", 0.0)
        native_m = modes.get("native_cpp", {}).get("peak_rss_mb", 0.0)
        stage1_m = modes.get("self_hosted_native", {}).get("peak_rss_mb", 0.0)
        md.append(f"| `{w}` | {interp_m:.1f} MB | {native_m:.1f} MB | {stage1_m:.1f} MB |")
    md.append("\n")

    return "\n".join(md)


def main():
    parser = argparse.ArgumentParser(description="MEMElang Comprehensive Benchmarking Suite")
    parser.add_argument("--workloads", default="all", help="Comma-separated workloads (fib, sieve, math_loop, tree_drop, json, csv) or 'all'")
    parser.add_argument("--iterations", type=int, default=3, help="Benchmark run iterations per workload")
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
        print(f"Error: Host binary '{MEME_BIN}' not found.", file=sys.stderr)
        sys.exit(1)

    print(f"🚀 Running MEMElang Benchmarks on {len(selected_workloads)} workloads ({', '.join(selected_workloads)})...")
    comp_metrics = benchmark_compilation(selected_workloads)
    throughput_metrics, memory_metrics, matrix = benchmark_runtime_workloads(selected_workloads, max(1, args.iterations))

    report_data = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "platform": f"{platform.system()} {platform.machine()}",
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
        Path(args.json_out).write_text(json.dumps(report_data, indent=2), encoding="utf-8")
        print(f"✔ Saved JSON report to: {args.json_out}")

    if args.markdown_out:
        Path(args.markdown_out).write_text(md_report, encoding="utf-8")
        print(f"✔ Saved Markdown report to: {args.markdown_out}")

    print("\n" + md_report)


if __name__ == "__main__":
    main()
