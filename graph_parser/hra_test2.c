#include "hra_test2.h"

// Union-Find with path compression
static int uf_find(int *parent, int x) {
  if (parent[x] != x) {
    parent[x] = uf_find(parent, parent[x]);
  }
  return parent[x];
}

// Optimized next_permutation
bool next_permutation(int *arr, int n) {
  if (n <= 1)
    return false;

  int i = n - 2;
  while (i >= 0 && arr[i] >= arr[i + 1])
    i--;
  if (i < 0)
    return false;

  int j = n - 1;
  while (arr[j] <= arr[i])
    j--;

  int temp = arr[i];
  arr[i] = arr[j];
  arr[j] = temp;

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

// Initialize graph efficiently
void init_graph(Graph *g, int n_nodes) {
  g->n_nodes = n_nodes;
  g->n_edges = 0;
  memset(g->in_degree, 0, sizeof(g->in_degree));
  memset(g->out_degree, 0, sizeof(g->out_degree));

  for (int i = 0; i < MAX_NODES; i++) {
    for (int j = 0; j < MAX_NODES; j++) {
      g->adj_matrix[i][j] = -1;
    }
  }
}

// Add edge with bounds checking
void add_edge(Graph *g, int from, int to, int regulation) {
  if (g->n_edges >= MAX_EDGES)
    return;

  g->edges[g->n_edges] = (Edge){from, to, regulation};
  g->adj_matrix[from][to] = regulation;
  g->out_degree[from]++;
  g->in_degree[to]++;
  g->n_edges++;
}

// Optimized connectivity check
bool is_weakly_connected(Graph *g) {
  if (g->n_nodes <= 1)
    return true;

  int parent[MAX_NODES];
  for (int i = 0; i < g->n_nodes; i++)
    parent[i] = i;

  for (int i = 0; i < g->n_edges; i++) {
    int u = g->edges[i].from;
    int v = g->edges[i].to;
    int ru = uf_find(parent, u);
    int rv = uf_find(parent, v);
    if (ru != rv)
      parent[ru] = rv;
  }

  int root = uf_find(parent, 0);
  for (int i = 1; i < g->n_nodes; i++) {
    if (uf_find(parent, i) != root)
      return false;
  }
  return true;
}

// Efficient heredity checks
bool is_heritable_topology(Graph *g) {
  for (int i = 0; i < g->n_nodes; i++) {
    if (g->in_degree[i] == 0)
      return false;
  }
  return true;
}

bool is_heritable_regulatory(Graph *g) {
  for (int i = 0; i < g->n_nodes; i++) {
    bool has_gray = false;
    for (int j = 0; j < g->n_nodes; j++) {
      if (g->adj_matrix[j][i] == 0) {
        has_gray = true;
        break;
      }
    }
    if (!has_gray)
      return false;
  }
  return true;
}

// Canonical representation to replace isomorphism checks
void compute_canonical_rep(Graph *g, int *canon) {
  int n = g->n_nodes;
  int perm[MAX_NODES];
  for (int i = 0; i < n; i++)
    perm[i] = i;

  int best[MAX_NODES * MAX_NODES] = {0};
  int found = 0;

  do {
    int current[MAX_NODES * MAX_NODES];
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        current[i * n + j] = g->adj_matrix[perm[i]][perm[j]];
      }
    }

    if (!found) {
      memcpy(best, current, n * n * sizeof(int));
      found = 1;
    } else if (memcmp(current, best, n * n * sizeof(int)) < 0) {
      memcpy(best, current, n * n * sizeof(int));
    }
  } while (next_permutation(perm, n));

  memcpy(canon, best, n * n * sizeof(int));
}

// Generate all possible edges
int generate_all_edges(int n, Edge *all_edges) {
  int count = 0;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (i != j) {
        all_edges[count++] = (Edge){i, j, 0};
      }
    }
  }
  return count;
}

// Generate edge combinations with canonical filtering
void generate_edge_combinations(Edge *all_edges, int total_edges, int k,
                                int start, Edge *current_combo, int combo_idx,
                                GraphCollection *topologies, int n_nodes,
                                int *seen_reps, int *seen_count) {
  if (combo_idx == k) {
    Graph g;
    init_graph(&g, n_nodes);
    for (int i = 0; i < k; i++) {
      add_edge(&g, current_combo[i].from, current_combo[i].to, 0);
    }

    if (is_weakly_connected(&g) && is_heritable_topology(&g)) {
      int canon_rep[MAX_NODES * MAX_NODES];
      compute_canonical_rep(&g, canon_rep);

      bool found = false;
      for (int i = 0; i < *seen_count; i++) {
        if (memcmp(seen_reps + i * MAX_NODES * MAX_NODES, canon_rep,
                   n_nodes * n_nodes * sizeof(int)) == 0) {
          found = true;
          break;
        }
      }

      if (!found && topologies->count < MAX_GRAPHS &&
          *seen_count < MAX_GRAPHS) {
        memcpy(seen_reps + (*seen_count) * MAX_NODES * MAX_NODES, canon_rep,
               n_nodes * n_nodes * sizeof(int));
        (*seen_count)++;
        topologies->graphs[topologies->count++] = g;
      }
    }
    return;
  }

  for (int i = start; i < total_edges; i++) {
    current_combo[combo_idx] = all_edges[i];
    generate_edge_combinations(all_edges, total_edges, k, i + 1, current_combo,
                               combo_idx + 1, topologies, n_nodes, seen_reps,
                               seen_count);
  }
}

// Optimized regulatory architecture generation
void generate_regulatory_from_topology(Graph *topology,
                                       unsigned long long *hras_count,
                                       int *global_seen_reps,
                                       int *global_seen_count) {
  int n = topology->n_nodes;
  int n_edges = topology->n_edges;
  unsigned long long total_patterns = 1ULL << n_edges;

  for (unsigned long pattern = 0; pattern < total_patterns; pattern++) {
    // Early termination for invalid patterns
    bool valid = true;
    for (int i = 0; i < n; i++) {
      bool has_gray = false;
      for (int j = 0; j < n_edges; j++) {
        if (topology->edges[j].to == i && ((pattern >> j) & 1) == 0) {
          has_gray = true;
          break;
        }
      }
      if (!has_gray) {
        valid = false;
        break;
      }
    }
    if (!valid)
      continue;

    // Apply pattern
    Graph reg_g = *topology;
    for (int i = 0; i < n_edges; i++) {
      int reg = (pattern >> i) & 1;
      reg_g.edges[i].regulation = reg;
      reg_g.adj_matrix[reg_g.edges[i].from][reg_g.edges[i].to] = reg;
    }

    // Compute canonical representation
    int canon_rep[MAX_NODES * MAX_NODES];
    compute_canonical_rep(&reg_g, canon_rep);

    // Check global cache
    bool found = false;
    for (int i = 0; i < *global_seen_count; i++) {
      if (memcmp(global_seen_reps + i * MAX_NODES * MAX_NODES, canon_rep,
                 n * n * sizeof(int)) == 0) {
        found = true;
        break;
      }
    }

    if (!found) {
      if (*global_seen_count < MAX_GRAPHS) {
        memcpy(global_seen_reps + (*global_seen_count) * MAX_NODES * MAX_NODES,
               canon_rep, n * n * sizeof(int));
        (*global_seen_count)++;
        (*hras_count)++;
      }
    }
  }
}

// Main optimized HRA generation
void generate_hras(int n, bool verbose) {
  if (n <= 0 || n > MAX_NODES) {
    printf("Error: n must be between 1 and %d\n", MAX_NODES);
    return;
  }

  clock_t start = clock();
  if (verbose) {
    printf("Finding topologies for n=%d...\n", n);
  }

  // Handle n=1 separately
  if (n == 1) {
    printf("\nResults for n=%d:\n", n);
    printf("Heritable Topologies: 0\n");
    printf("Heritable Regulatory Architectures: 0\n");
    printf("Bits: n/a\n");
    printf("Time: %.2f seconds\n", 0.0);
    return;
  }

  // Generate all possible edges
  Edge all_edges[MAX_EDGES];
  int total_edges = generate_all_edges(n, all_edges);

  // Generate topologies
  GraphCollection topologies = {.count = 0};
  Edge current_combo[MAX_EDGES];
  int seen_reps[MAX_GRAPHS * MAX_NODES * MAX_NODES];
  int seen_count = 0;

  int min_edges = n;
  for (int k = min_edges; k <= total_edges; k++) {
    if (verbose) {
      printf("Checking %d-edge combinations...\n", k);
    }
    generate_edge_combinations(all_edges, total_edges, k, 0, current_combo, 0,
                               &topologies, n, seen_reps, &seen_count);
  }

  clock_t topology_time = clock();
  if (verbose) {
    printf("Found %d topologies in %.2f seconds\n", topologies.count,
           ((double)(topology_time - start)) / CLOCKS_PER_SEC);
    printf("Generating regulatory architectures...\n");
  }

  // Generate regulatory architectures
  unsigned long long hras_count = 0;
  int global_seen_reps[MAX_GRAPHS * MAX_NODES * MAX_NODES];
  int global_seen_count = 0;

  for (int i = 0; i < topologies.count; i++) {
    if (verbose && i % 10 == 0) {
      printf("Processing topology %d/%d...\n", i + 1, topologies.count);
    }
    generate_regulatory_from_topology(&topologies.graphs[i], &hras_count,
                                      global_seen_reps, &global_seen_count);
  }

  clock_t end = clock();
  double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  // Print results
  printf("\nResults for n=%d:\n", n);
  printf("Heritable Topologies: %d\n", topologies.count);
  printf("Heritable Regulatory Architectures: %llu\n", hras_count);
  if (hras_count > 0) {
    printf("Bits: %.2f\n", log2(hras_count));
  } else {
    printf("Bits: n/a\n");
  }
  printf("Time: %.2f seconds\n", total_time);
}

// Optimized counting with reduced output
void count_hras_by_size(void) {
  printf("Computing HRAs...\n");
  printf("\nSummary Table:\n");
  printf(" n | Topologies | HRAs     | Time (s)\n");
  printf("---+------------+----------+---------\n");

  for (int n = 1; n <= 6; n++) {
    clock_t start = clock();
    generate_hras(n, false);
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf(" %d | %s\n", n, (n == 1) ? "N/A      " : "");
  }
}

int main() {
  printf("Optimized HRA Generator (macOS Compatible)\n");
  printf("==========================================\n");

  count_hras_by_size();
  return 0;
}
