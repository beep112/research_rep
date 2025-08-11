# research_rep

## HRA pipeline overview

This repo contains a small pipeline to expand and analyze heritable regulatory architectures (HRAs):

- hra_runner: multithreaded orchestrator that reads canonical HRAs of size n and expands each source graph to size n+1 via a subprocess.
- hra_expander: given an input .dot file and a specific graph index, it adds one new node and enumerates connection patterns; each valid expanded graph is written as DOT with provenance.
- hra_sampler: parsing, canonicalization, and analysis utilities used by both the runner and the expander to validate graphs and compute unique counts/ratios.

Recent behavior updates:
- Safer path handling (uses PATH_MAX) and larger line buffer.
- Canonicalization is permutation-invariant for n ≤ 6, collapsing isomorphic graphs correctly.
- Provenance is taken from a leading comment in each output graph block: `// Source:<id>`.
- The analyzer reports unique counts, multi-source overlaps, singleton count, and an optional ratio versus a reference canonical set (if `hras_dot_files/hras_n<target>.dot` exists).

## Build

From the graph_parser directory:

```bash
cd graph_parser
make
```

This produces two executables: `hra_runner` and `hra_expander`.

## Run the expansion + analysis

Runner usage:

```text
./hra_runner <dot_file> [num_threads] [verbose] [start_size] [target_size]
```

- dot_file: path to the canonical HRAs at size n (e.g., `hras_dot_files/hras_n3.dot`).
- num_threads: number of worker threads (default 4).
- verbose: 1 to print per-file processing, 0 for quiet.
- start_size: n of the input set (default 3).
- target_size: n+1 (default 4).

Examples (run from `graph_parser/`):

```bash
./hra_runner hras_dot_files/hras_n3.dot 4 0 3 4
./hra_runner hras_dot_files/hras_n4.dot 4 0 4 5
```

What happens:
- The runner partitions input graphs across threads and, per source index, invokes the expander:
	`./hra_expander <input_dot_file> <graph_index> <output_file> <start_size> <target_size>`
- Each thread writes its outputs under `hra_evolution_results/` in files named `thread_<tid>_graph_<idx>.dot`.
- Every accepted expanded graph is output as a DOT block preceded by a provenance line:
	`// Source:<id> Canonical: <adjacency...>`
- When all threads finish, the runner analyzes `hra_evolution_results/` and prints a summary; it also writes `hra_stats.csv` with per-graph source counts.

## Output details

- Generated graphs folder: `graph_parser/hra_evolution_results/`
- Per-thread files can contain multiple `digraph { ... }` blocks; each block begins with `// Source:<id>` (the original n-graph index) and includes its canonical adjacency snapshot.
- Analysis summary includes:
	- Total unique expanded graphs (modulo isomorphism).
	- Unique graphs reachable from the processed sources.
	- Graphs appearing from more than one source (overlap).
	- Singleton count (from exactly one source).
	- Optional ratio unique_from_sources / total_reference if `hras_dot_files/hras_n<target>.dot` is present.

## Visualizing graphs on GitHub Pages

A simple viewer is available at the repo root: `graphs.html`. It provides two tabs powered by Viz.js (client-side Graphviz):
- Canonical (graph_parser/hras_dot_files)
- Outputs (graph_parser/hra_evolution_results)

To populate the file lists used by the viewer, generate `index.json` files:

```bash
make -C graph_parser index
```

Commit the generated files:
- `graph_parser/hras_dot_files/index.json`
- `graph_parser/hra_evolution_results/index.json`

Open the viewer (once published by GitHub Pages):
- https://<your-username>.github.io/research_rep/graphs.html

### Viewer layout controls and spacing

The viewer injects layout attributes into each DOT block before rendering. Use the controls in each tab to select a Graphviz engine and adjust spacing:

- Engine: dot, neato, sfdp, twopi, circo. The Outputs tab defaults to sfdp for better spacing on dense graphs.
- Spacing: nodesep and ranksep are applied in the graph header, along with overlap=false and splines=true.

Recommendations:
- For small DAG-like graphs, start with dot and nodesep≈0.6, ranksep≈0.9.
- For larger/denser graphs, switch to sfdp and increase nodesep to ~0.9 and ranksep to ~1.2.

## Troubleshooting

- If you see “Error: No graphs found …”, run the runner from the `graph_parser/` directory or pass a correct relative/absolute path to the input .dot file.
- Ensure `make` completed and `hra_runner` and `hra_expander` exist in `graph_parser/`.
- Long paths are supported via PATH_MAX; if you hit truncation warnings, prefer shorter output directory paths.

## Notes

- The expander currently explores a bounded pattern space when adding one node and applies three validity checks: weak connectivity, heritable topology (no node with zero in-degree), and heritable regulatory (each node must have an incoming label 0 edge).
- Canonicalization enumerates all node relabelings for n ≤ 6 to choose a lexicographically minimal adjacency; for larger n, it falls back to the direct adjacency matrix.