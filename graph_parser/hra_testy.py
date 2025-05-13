import itertools
import math
import time
from itertools import chain

import matplotlib.pyplot as plt
import networkx as nx
from networkx.algorithms import isomorphism


def generate_hras_accurate(n, verbose=True):
    """
    Generate all heritable regulatory architectures (HRAs) with n nodes.
    This version emphasizes correctness while optimizing where possible.
    """
    start_time = time.time()
    all_nodes = list(range(1, n + 1))
    all_edges = list(itertools.permutations(all_nodes, r=2))

    if verbose:
        print(f"Finding weakly connected heritable topologies for n={n}...")

    # Step 1: Find all weakly connected, heritable, non-isomorphic topologies
    # "Heritable topology" means every node has at least one incoming edge
    weakly_connected_heritable_non_iso_digraphs = []

    # Optimization: Start with the minimum number of edges needed for heredity
    # Each node needs at least one incoming edge, so minimum is n edges
    min_edges = n

    # For small n, we can afford to be thorough and check all possible edge configurations
    # For n=4, there are 12 possible edges, so 2^12 = 4096 possible edge configurations
    # which is still manageable with proper optimizations

    # Generate edge sets more efficiently by starting with at least min_edges
    valid_edge_sets = []
    for k in range(min_edges, len(all_edges) + 1):
        if verbose and k == min_edges:
            print(f"Generating edge sets with at least {min_edges} edges...")
        edge_combinations = itertools.combinations(all_edges, r=k)
        valid_edge_sets.extend(edge_combinations)

    if verbose:
        print(f"Checking {len(valid_edge_sets)} potential edge configurations...")

    # Process edge sets to find valid topologies
    for idx, edge_set in enumerate(valid_edge_sets):
        if verbose and idx % 10000 == 0 and idx > 0:
            print(f"Processed {idx}/{len(valid_edge_sets)} edge sets...")

        g = nx.DiGraph()
        g.add_nodes_from(all_nodes)
        g.add_edges_from(edge_set)

        # Quick check: must be weakly connected and all nodes must have incoming edges
        if not nx.is_weakly_connected(g):
            continue

        heritable = all(g.in_degree(i) > 0 for i in all_nodes)
        if not heritable:
            continue

        # Isomorphism check (only if we've passed the quick checks)
        found_isomorphic = False
        for existing in weakly_connected_heritable_non_iso_digraphs:
            if nx.is_isomorphic(g, existing):
                found_isomorphic = True
                break

        if not found_isomorphic:
            weakly_connected_heritable_non_iso_digraphs.append(g)

    topology_time = time.time()
    if verbose:
        print(
            f"Found {len(weakly_connected_heritable_non_iso_digraphs)} heritable topologies in {topology_time - start_time:.2f} seconds"
        )

    # Step 2: Generate all non-isomorphic regulatory architectures
    non_iso_regulatory_architectures = []

    if verbose:
        print("Generating regulatory architectures...")

    # Process each topology
    for g_idx, g in enumerate(weakly_connected_heritable_non_iso_digraphs):
        if verbose and g_idx % 10 == 0:
            print(
                f"Processing topology {g_idx+1}/{len(weakly_connected_heritable_non_iso_digraphs)}..."
            )

        edge_set = list(g.edges())
        edge_count = len(edge_set)

        # Generate all possible regulation patterns using product instead of combinations_with_replacement
        # This matches the reference implementation logic better
        for regulation_pattern in itertools.product(
            ["gray", "black"], repeat=edge_count
        ):
            # Create regulated graph
            regulated_g = nx.DiGraph()
            regulated_g.add_nodes_from(all_nodes)

            # Add edges with regulation attributes
            regulation_to_edges = {}
            for idx, (u, v) in enumerate(edge_set):
                regulated_g.add_edge(u, v)
                regulation_to_edges[(u, v)] = {"regulation": regulation_pattern[idx]}

            nx.set_edge_attributes(regulated_g, regulation_to_edges)

            # Check for isomorphism with existing regulatory architectures
            found_isomorphic = False
            for existing in non_iso_regulatory_architectures:
                matcher = isomorphism.DiGraphMatcher(
                    regulated_g,
                    existing,
                    edge_match=lambda e1, e2: e1.get("regulation")
                    == e2.get("regulation"),
                )
                if matcher.is_isomorphic():
                    found_isomorphic = True
                    break

            if not found_isomorphic:
                non_iso_regulatory_architectures.append(regulated_g)

    regulatory_time = time.time()
    if verbose:
        print(
            f"Found {len(non_iso_regulatory_architectures)} non-isomorphic regulatory architectures in {regulatory_time - topology_time:.2f} seconds"
        )

    # Step 3: Filter for heritable regulatory architectures
    # A regulatory architecture is heritable if every node has at least one incoming 'gray' edge
    heritable_non_iso_regulatory_architectures = []

    if verbose:
        print("Filtering for heritable regulatory architectures...")

    for regulated_g in non_iso_regulatory_architectures:
        heritable = True
        for node in all_nodes:
            has_gray_incoming = False
            for pred in regulated_g.predecessors(node):
                if regulated_g[pred][node].get("regulation") == "gray":
                    has_gray_incoming = True
                    break

            if not has_gray_incoming:
                heritable = False
                break

        if heritable:
            heritable_non_iso_regulatory_architectures.append(regulated_g)

    end_time = time.time()
    if verbose:
        print(
            f"Found {len(heritable_non_iso_regulatory_architectures)} heritable regulatory architectures in {end_time - start_time:.2f} seconds"
        )
        print(f"Total time: {end_time - start_time:.2f} seconds")

    return {
        "topologies": weakly_connected_heritable_non_iso_digraphs,
        "regulatory": non_iso_regulatory_architectures,
        "heritable": heritable_non_iso_regulatory_architectures,
    }


def plot_hras(hras, columns=5, max_plots=50):
    """Plot the HRAs in a grid layout"""
    if not hras:
        print("No HRAs to plot")
        return

    # Limit the number of plots if needed
    if len(hras) > max_plots:
        print(f"Limiting display to first {max_plots} HRAs out of {len(hras)}")
        hras = hras[:max_plots]

    rows = (len(hras) + columns - 1) // columns
    plt.figure(figsize=(columns * 3, rows * 3))

    for i, g in enumerate(hras):
        ax = plt.subplot(rows, columns, i + 1)
        ax.axis("off")
        pos = nx.circular_layout(g)

        # Color nodes based on whether they regulate other nodes
        node_color_map = []
        for node in sorted(g.nodes()):
            if g.out_degree(node) == 0:
                node_color_map.append("blue")
            else:
                node_color_map.append("red")

        # Get edge colors from the regulation attribute
        edges = g.edges()
        edge_color_map = [g[u][v]["regulation"] for u, v in edges]

        nx.draw_networkx_nodes(g, pos, node_color=node_color_map, node_size=300, ax=ax)
        nx.draw_networkx_edges(
            g,
            pos,
            edge_color=edge_color_map,
            width=2,
            connectionstyle="arc3,rad=0.2",
            ax=ax,
        )
        nx.draw_networkx_labels(g, pos, font_color="white", ax=ax)

    plt.tight_layout()
    plt.show()


def count_hras_by_size():
    """Count HRAs for different entity sizes to verify against expected results"""
    results = []

    for n in range(1, 5):  # Test for n=1,2,3,4
        start_time = time.time()
        print(f"\nComputing for n={n}...")

        data = generate_hras_accurate(n)

        heritable_topologies = len(data["topologies"])
        regulatory_archs = len(data["regulatory"])
        heritable_archs = len(data["heritable"])

        if heritable_archs > 0:
            bits = round(math.log2(heritable_archs), 2)
        else:
            bits = "n/a"

        results.append(
            {
                "n": n,
                "heritable_topologies": heritable_topologies,
                "regulatory_archs": regulatory_archs,
                "heritable_archs": heritable_archs,
                "bits": bits,
                "time": time.time() - start_time,
            }
        )

        print(f"\nResults for n={n}:")
        print(f"Heritable Topologies: {heritable_topologies}")
        print(f"Regulatory Architectures: {regulatory_archs}")
        print(f"Heritable Regulatory Architectures: {heritable_archs}")
        print(f"Bits: {bits}")
        print(f"Time: {results[-1]['time']:.2f} seconds")

    # Print summary table
    print("\nSummary Table:")
    print("Entities | Heritable Topologies | Regulatory Archs | Heritable Archs | Bits")
    print("---------|---------------------|-----------------|----------------|------")
    for r in results:
        print(
            f"{r['n']} | {r['heritable_topologies']} | {r['regulatory_archs']} | {r['heritable_archs']} | {r['bits']}"
        )

    return results


# Example usage
if __name__ == "__main__":
    # Verify counts for smaller n values first
    results = count_hras_by_size()

    # Optional: For n=3, plot the heritable architectures
    data = generate_hras_accurate(4, verbose=False)
    plot_hras(data["heritable"])
