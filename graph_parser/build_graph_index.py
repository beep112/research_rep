#!/usr/bin/env python3
"""
Generate index.json files listing all .dot files for the graphs.html viewer.

Outputs:
- graph_parser/hras_dot_files/index.json
- graph_parser/hra_evolution_results/index.json

Each JSON is a simple list of file paths relative to the repo root so that
graphs.html can fetch and render them on GitHub Pages.
"""
import json
import os
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]

CANON_DIR = REPO_ROOT / "graph_parser" / "hras_dot_files"
OUT_DIR = REPO_ROOT / "graph_parser" / "hra_evolution_results"

def list_dot_files(directory: Path):
    if not directory.exists():
        return []
    items = []
    for name in sorted(os.listdir(directory)):
        if name.lower().endswith(".dot"):
            rel = Path("graph_parser") / directory.name / name
            items.append(str(rel).replace(os.sep, "/"))
    return items

def write_index(directory: Path, entries):
    directory.mkdir(parents=True, exist_ok=True)
    index_path = directory / "index.json"
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(entries, f, indent=2)
    print(f"Wrote {len(entries)} entries to {index_path.relative_to(REPO_ROOT)}")

def main():
    canon_entries = list_dot_files(CANON_DIR)
    out_entries = list_dot_files(OUT_DIR)
    write_index(CANON_DIR, canon_entries)
    write_index(OUT_DIR, out_entries)

if __name__ == "__main__":
    main()
