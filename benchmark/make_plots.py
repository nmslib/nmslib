#!/usr/bin/env python
"""
Generate recall-vs-QPS plots from a results JSON.

For each entry (a dict with a "result_list") the script:
- Creates directories in this exact order:
  nmslib_version / py_ver / os_ver / cpu / dataset_name / dist_type / K / is_index_reload
- Saves a plot (PNG) whose title includes: dataset_name, dist_type, K, cpu
- Uses a filename that includes the same fields + is_index_reload

Usage:
  python make_plots.py /path/to/results.json --out /path/to/output_dir

Dependencies:
  - matplotlib (pip install matplotlib)
"""

import argparse
import json
import re
from pathlib import Path

import matplotlib.pyplot as plt


def sanitize(s) -> str:
    """Return a filesystem-safe string."""
    s = str(s)
    s = s.replace(" ", "_")
    return re.sub(r"[^A-Za-z0-9._\-+]", "_", s)


def plot_entry(entry: dict, out_root: Path):
    """
    Plot recall vs QPS for a single entry and save the PNG under the required hierarchy.

    Required hierarchy (in order):
      nmslib_version / py_ver / os_ver / cpu / dataset_name / dist_type / K / is_index_reload
    Title must include: dataset_name, dist_type, K, cpu
    Filename must include: dataset_name, dist_type, K, cpu, is_index_reload
    """
    # Extract fields with safe fallbacks
    nmslib_version = sanitize(entry.get("nmslib_version", "unknown"))
    py_ver        = sanitize(entry.get("py_ver", "unknown"))
    os_ver        = sanitize(entry.get("os_ver", "unknown"))
    cpu           = sanitize(entry.get("cpu", "unknown"))
    dataset_name  = sanitize(entry.get("dataset_name", "unknown"))
    dist_type     = sanitize(entry.get("dist_type", "unknown"))
    K             = entry.get("K", "unknown")
    is_reload     = entry.get("is_index_reload", False)

    # Build output directory hierarchy
    out_dir = (
        out_root
        / f'nmslib_version={nmslib_version}'
        / f'py{py_ver}'
        / os_ver
        / cpu
        / dataset_name
        / dist_type
        / f'k={K}'
        / f'is_reload={is_reload}'
    )
    out_dir.mkdir(parents=True, exist_ok=True)

    # Prepare data
    results = entry.get("result_list", [])
    pts = [(r.get("recall"), r.get("qps")) for r in results if "qps" in r and "recall" in r]

    if not pts:
        # Nothing to plot for this entry
        return None

    # Sort (optional, makes the curve nicer to read)
    pts.sort(key=lambda t: (t[1], t[0]))  # by recall then qps

    x = [p[0] for p in pts]  # QPS
    y = [p[1] for p in pts]  # Recall

    # Plot
    plt.figure()
    plt.plot(x, y, marker="o")
    plt.xlabel("Recall")
    plt.ylabel("QPS")
    title = f"{dataset_name} | {dist_type} | K={K} | CPU={cpu} | OS={os_ver}"
    plt.title(title)
    plt.grid(True)
    plt.tight_layout()

    # Filename must include: dataset_name, dist_type, K, cpu, is_index_reload
    fname = f"{dataset_name}_{dist_type}_K{K}_{cpu}_reload-{is_reload}.png"
    out_path = out_dir / sanitize(fname)
    plt.savefig(out_path, dpi=150)
    plt.close()

    return out_path


def load_entries(data):
    """
    Normalize input JSON into a list of entry dicts.
    Accepts:
      - a list of dicts (each with 'result_list'),
      - a dict whose values may contain such lists,
      - or a single dict with 'result_list'.
    """
    if isinstance(data, list):
        return [e for e in data if isinstance(e, dict) and "result_list" in e]

    if isinstance(data, dict):
        entries = []
        # If top-level is a proper entry
        if "result_list" in data and isinstance(data["result_list"], list):
            entries.append(data)
        # Or gather from any list values
        for v in data.values():
            if isinstance(v, list):
                for e in v:
                    if isinstance(e, dict) and "result_list" in e:
                        entries.append(e)
        return entries

    return []


def main():
    parser = argparse.ArgumentParser(description="Generate recall vs QPS plots from JSON.")
    parser.add_argument("input_json", type=Path, help="Path to results JSON file.")
    parser.add_argument(
        "--out",
        type=Path,
        default=Path("plots"),
        help="Output directory root (default: ./plots)",
    )
    args = parser.parse_args()

    with args.input_json.open("r", encoding="utf-8") as f:
        data = json.load(f)

    entries = load_entries(data)
    if not entries:
        print("No valid entries with 'result_list' found in the input JSON.")
        return

    print(f"Found {len(entries)} entries. Writing plots under: {args.out.resolve()}")
    count = 0
    for entry in entries:
        out_path = plot_entry(entry, args.out)
        if out_path:
            count += 1
            print(f"  ✓ {out_path}")
        else:
            print("  – Skipped an entry (no recall/qps points).")
    print(f"Done. Generated {count} plot(s).")


if __name__ == "__main__":
    main()

