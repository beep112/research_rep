#include "hra_test.h"

// Union-Find helper function (static - only used internally)
static int uf_find(int *parent, int x) {
  if (parent[x] != x) {
    parent[x] = uf_find(parent, parent[x]);
  }
  return parent[x];
}

// Helper function for next permutation
bool next_permutation(int *arr, int n) {
  int i = n - 2;
  while (i >= 0 && arr[i] >= arr[i + 1])
    i--;
  if (i < 0)
    return false;

  int j = n - 1;
  while (arr[j] <= arr[i])
    j--;

  // Swap
  int temp = arr[i];
  arr[i] = arr[j];
  arr[j] = temp;

  // Reverse suffix
  i++;
  j = n - 1;
  while (i < j) {
    temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
    i++;
    j--;
  }

  return true;
}

// Initialize graph
void init_graph(Graph *g, int n_nodes) {
  g->n_nodes = n_nodes;
  g->n_edges = 0;

  for (int i = 0; i < MAX_NODES; i++) {
    g->in_degree[i] = 0;
    g->out_degree[i] = 0;
    for (int j = 0; j < MAX_NODES; j++) {
      g->adj_matrix[i][j] = -1; // No edge
    }
  }
}

// Add edge to graph
void add_edge(Graph *g, int from, int to, int regulation) {
  if (g->n_edges >= MAX_EDGES)
    return;

  g->edges[g->n_edges].from = from;
  g->edges[g->n_edges].to = to;
  g->edges[g->n_edges].regulation = regulation;
  g->n_edges++;

  g->adj_matrix[from][to] = regulation;
  g->out_degree[from]++;
  g->in_degree[to]++;
}

// Check if graph is weakly connected using Union-Find
bool is_weakly_connected(Graph *g) {
  if (g->n_nodes <= 1)
    return true;

  int parent[MAX_NODES];
  for (int i = 0; i < g->n_nodes; i++) {
    parent[i] = i;
  }

  // Union edges (treat as undirected for weak connectivity)
  for (int i = 0; i < g->n_edges; i++) {
    int root_from = uf_find(parent, g->edges[i].from);
    int root_to = uf_find(parent, g->edges[i].to);
    if (root_from != root_to) {
      parent[root_from] = root_to;
    }
  }

  // Check if all nodes are in same component
  int root = uf_find(parent, 0);
  for (int i = 1; i < g->n_nodes; i++) {
    if (uf_find(parent, i) != root)
      return false;
  }

  return true;
}

// Check if topology is heritable (all nodes have incoming edges)
bool is_heritable_topology(Graph *g) {
  for (int i = 0; i < g->n_nodes; i++) {
    if (g->in_degree[i] == 0)
      return false;
  }
  return true;
}

// Check if regulatory architecture is heritable (all nodes have gray incoming
// edges)
bool is_heritable_regulatory(Graph *g) {
  for (int i = 0; i < g->n_nodes; i++) {
    bool has_gray_incoming = false;
    for (int j = 0; j < g->n_nodes; j++) {
      if (g->adj_matrix[j][i] == 0) { // Gray edge
        has_gray_incoming = true;
        break;
      }
    }
    if (!has_gray_incoming)
      return false;
  }
  return true;
}

// Simple graph isomorphism check (naive implementation)
bool graphs_isomorphic(Graph *g1, Graph *g2) {
  if (g1->n_nodes != g2->n_nodes || g1->n_edges != g2->n_edges) {
    return false;
  }

  int n = g1->n_nodes;
  int perm[MAX_NODES];

  // Initialize permutation
  for (int i = 0; i < n; i++) {
    perm[i] = i;
  }

  // Check all permutations (factorial complexity - OK for small n)
  do {
    bool match = true;

    // Check if this permutation makes graphs identical
    for (int i = 0; i < n && match; i++) {
      for (int j = 0; j < n && match; j++) {
        if (g1->adj_matrix[i][j] != g2->adj_matrix[perm[i]][perm[j]]) {
          match = false;
        }
      }
    }

    if (match)
      return true;

  } while (next_permutation(perm, n));

  return false;
}

// Generate all possible edges for n nodes
int generate_all_edges(int n, Edge *all_edges) {
  int count = 0;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (i != j) {
        all_edges[count].from = i;
        all_edges[count].to = j;
        all_edges[count].regulation = 0; // Will be set later
        count++;
      }
    }
  }
  return count;
}

// Generate edge combinations
void generate_edge_combinations(Edge *all_edges, int total_edges, int k,
                                int start, Edge *current_combo, int combo_idx,
                                GraphCollection *topologies, int n_nodes) {
  if (combo_idx == k) {
    // Create graph from current combination
    Graph g;
    init_graph(&g, n_nodes);

    for (int i = 0; i < k; i++) {
      add_edge(&g, current_combo[i].from, current_combo[i].to, 0);
    }

    // Check if valid topology
    if (is_weakly_connected(&g) && is_heritable_topology(&g)) {
      // Check for isomorphism with existing topologies
      bool found_iso = false;
      for (int i = 0; i < topologies->count; i++) {
        if (graphs_isomorphic(&g, &topologies->graphs[i])) {
          found_iso = true;
          break;
        }
      }

      if (!found_iso && topologies->count < MAX_GRAPHS) {
        topologies->graphs[topologies->count] = g;
        topologies->count++;
      }
    }
    return;
  }

  for (int i = start; i < total_edges; i++) {
    current_combo[combo_idx] = all_edges[i];
    generate_edge_combinations(all_edges, total_edges, k, i + 1, current_combo,
                               combo_idx + 1, topologies, n_nodes);
  }
}

// Generate regulatory architectures from topology
void generate_regulatory_from_topology(Graph *topology,
                                       GraphCollection *reg_archs) {
  int n_edges = topology->n_edges;
  int total_patterns = 1 << n_edges; // 2^n_edges

  for (int pattern = 0; pattern < total_patterns; pattern++) {
    Graph reg_g = *topology; // Copy topology

    // Apply regulation pattern
    for (int i = 0; i < n_edges; i++) {
      int regulation = (pattern >> i) & 1;
      reg_g.edges[i].regulation = regulation;
      reg_g.adj_matrix[reg_g.edges[i].from][reg_g.edges[i].to] = regulation;
    }

    // Check if heritable regulatory architecture
    if (!is_heritable_regulatory(&reg_g)) {
      continue;
    }

    // Check for isomorphism with existing regulatory architectures
    bool found_iso = false;
    for (int i = 0; i < reg_archs->count; i++) {
      if (graphs_isomorphic(&reg_g, &reg_archs->graphs[i])) {
        found_iso = true;
        break;
      }
    }

    if (!found_iso && reg_archs->count < MAX_GRAPHS) {
      reg_archs->graphs[reg_archs->count] = reg_g;
      reg_archs->count++;
    }
  }
}

// Main HRA generation function
void generate_hras(int n, bool verbose) {
  if (n <= 0 || n > MAX_NODES) {
    printf("Error: n must be between 1 and %d\n", MAX_NODES);
    return;
  }

  clock_t start = clock();

  if (verbose) {
    printf("Finding weakly connected heritable topologies for n=%d...\n", n);
  }

  // For n=1, there are no valid heritable topologies (need incoming edges)
  if (n == 1) {
    printf("\nResults for n=%d:\n", n);
    printf("Heritable Topologies: 0\n");
    printf("Regulatory Architectures: 0\n");
    printf("Heritable Regulatory Architectures: 0\n");
    printf("Bits: n/a\n");
    printf("Time: 0.00 seconds\n");
    return;
  }

  // Step 1: Generate all possible edges
  Edge all_edges[MAX_EDGES];
  int total_edges = generate_all_edges(n, all_edges);

  // Allocate topologies on the heap
  GraphCollection *topologies =
      (GraphCollection *)malloc(sizeof(GraphCollection));
  if (topologies == NULL) {
    printf("Memory allocation failed for topologies.\n");
    return;
  }
  topologies->count = 0;

  Edge current_combo[MAX_EDGES];

  int min_edges = n; // Minimum edges for heredity

  for (int k = min_edges; k <= total_edges; k++) {
    if (verbose) {
      printf("Checking edge combinations with %d edges...\n", k);
    }
    generate_edge_combinations(all_edges, total_edges, k, 0, current_combo, 0,
                               topologies, n);
  }

  clock_t topology_time = clock();
  if (verbose) {
    printf("Found %d heritable topologies in %.2f seconds\n", topologies->count,
           ((double)(topology_time - start)) / CLOCKS_PER_SEC);
  }

  // Allocate heritable_archs on the heap
  GraphCollection *heritable_archs =
      (GraphCollection *)malloc(sizeof(GraphCollection));
  if (heritable_archs == NULL) {
    printf("Memory allocation failed for heritable_archs.\n");
    free(topologies);
    return;
  }
  heritable_archs->count = 0;

  if (verbose) {
    printf("Generating heritable regulatory architectures...\n");
  }

  for (int i = 0; i < topologies->count; i++) {
    if (verbose && i % 10 == 0) {
      printf("Processing topology %d/%d...\n", i + 1, topologies->count);
    }
    generate_regulatory_from_topology(&topologies->graphs[i], heritable_archs);
  }

  clock_t end = clock();
  double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  if (verbose) {
    printf("Found %d heritable regulatory architectures in %.2f seconds\n",
           heritable_archs->count, total_time);
    printf("Total time: %.2f seconds\n", total_time);
  }

  // Print results
  printf("\nResults for n=%d:\n", n);
  printf("Heritable Topologies: %d\n", topologies->count);
  printf("Regulatory Architectures: %d\n", heritable_archs->count);
  printf("Heritable Regulatory Architectures: %d\n", heritable_archs->count);
  if (heritable_archs->count > 0) {
    printf("Bits: %.2f\n", log2(heritable_archs->count));
  } else {
    printf("Bits: n/a\n");
  }
  printf("Time: %.2f seconds\n", total_time);

  // Free allocated memory
  free(topologies);
  free(heritable_archs);
}

// Count HRAs for different sizes
void count_hras_by_size(void) {
  printf("Computing HRAs for different entity sizes...\n");
  printf("\nSummary Table:\n");
  printf("Entities | Time (seconds)\n");
  printf("---------|---------------\n");

  for (int n = 1; n <= 6; n++) {
    clock_t start = clock();
    printf("\nComputing for n=%d...\n", n);
    generate_hras(n, true);
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("%d | %.2f\n", n, time_taken);
  }
}

int main() {
  printf("HRA Generator - C Implementation\n");
  printf("================================\n");

  count_hras_by_size();

  return 0;
}
