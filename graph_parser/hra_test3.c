#include "hra_test3.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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
  int tmp = arr[i];
  arr[i] = arr[j];
  arr[j] = tmp;
  for (i++, j = n - 1; i < j; i++, j--) {
    tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
  }
  return true;
}

// Initialize graph
void init_graph(Graph *g, int n_nodes) {
  g->n_nodes = n_nodes;
  g->n_edges = 0;
  memset(g->in_degree, 0, sizeof(g->in_degree));
  memset(g->out_degree, 0, sizeof(g->out_degree));
  for (int i = 0; i < MAX_NODES; i++)
    for (int j = 0; j < MAX_NODES; j++)
      g->adj_matrix[i][j] = -1;
}

// Add edge
void add_edge(Graph *g, int from, int to, int regulation) {
  if (g->n_edges >= MAX_EDGES || from < 0 || to < 0 || from >= MAX_NODES ||
      to >= MAX_NODES)
    return;
  g->edges[g->n_edges] = (Edge){from, to, regulation};
  g->adj_matrix[from][to] = regulation;
  g->out_degree[from]++;
  g->in_degree[to]++;
  g->n_edges++;
}

// Connectivity & heredity tests
bool is_weakly_connected(Graph *g) {
  if (g->n_nodes <= 1)
    return true;
  int parent[MAX_NODES];
  for (int i = 0; i < g->n_nodes; i++)
    parent[i] = i;
  for (int i = 0; i < g->n_edges; i++) {
    int u = g->edges[i].from, v = g->edges[i].to;
    int ru = uf_find(parent, u), rv = uf_find(parent, v);
    if (ru != rv)
      parent[ru] = rv;
  }
  int root = uf_find(parent, 0);
  for (int i = 1; i < g->n_nodes; i++)
    if (uf_find(parent, i) != root)
      return false;
  return true;
}

bool is_heritable_topology(Graph *g) {
  for (int i = 0; i < g->n_nodes; i++)
    if (g->in_degree[i] == 0)
      return false;
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

// Canonical representation
void compute_canonical_rep(Graph *g, int *canon) {
  int n = g->n_nodes;
  int perm[MAX_NODES], best[MAX_NODES * MAX_NODES],
      current[MAX_NODES * MAX_NODES];
  for (int i = 0; i < n; i++)
    perm[i] = i;
  memset(best, 0, sizeof(best));
  bool first = true;
  do {
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        current[i * n + j] = g->adj_matrix[perm[i]][perm[j]];
    if (first || memcmp(current, best, n * n * sizeof(int)) < 0) {
      memcpy(best, current, n * n * sizeof(int));
      first = false;
    }
  } while (next_permutation(perm, n));
  memcpy(canon, best, n * n * sizeof(int));
}

// Generate all possible edges
int generate_all_edges(int n, Edge *all_edges) {
  int cnt = 0;
  for (int i = 0; i < n && cnt < MAX_EDGES; i++)
    for (int j = 0; j < n && cnt < MAX_EDGES; j++)
      if (i != j)
        all_edges[cnt++] = (Edge){i, j, 0};
  return cnt;
}

// Generate edge combinations for canonical topologies
void generate_edge_combinations(Edge *all_edges, int total_edges, int k,
                                int start, Edge *current_combo, int combo_idx,
                                GraphCollection *topologies, int n_nodes,
                                int *seen_reps, long long *seen_count) {
  if (combo_idx == k) {
    Graph g;
    init_graph(&g, n_nodes);
    for (int i = 0; i < k; i++)
      add_edge(&g, current_combo[i].from, current_combo[i].to, 0);
    if (is_weakly_connected(&g) && is_heritable_topology(&g)) {
      int canon[MAX_NODES * MAX_NODES];
      compute_canonical_rep(&g, canon);
      bool found = false;
      for (long long i = 0; i < *seen_count; i++) {
        if (memcmp(seen_reps + i * MAX_NODES * MAX_NODES, canon,
                   n_nodes * n_nodes * sizeof(int)) == 0) {
          found = true;
          break;
        }
      }
      if (!found && *seen_count < MAX_GRAPHS) {
        memcpy(seen_reps + (*seen_count) * MAX_NODES * MAX_NODES, canon,
               n_nodes * n_nodes * sizeof(int));
        (*seen_count)++;
        add_graph_to_collection(topologies, &g);
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

// Write Graph to DOT format
void write_graph_dot(Graph *g, FILE *f, int id, int n) {
  fprintf(f, "digraph HRA_n%d_%03d {\n", n, id);
  for (int i = 0; i < g->n_edges; i++) {
    Edge *e = &g->edges[i];
    fprintf(f, "  %d -> %d [label=\"%d\"];\n", e->from, e->to, e->regulation);
  }
  fprintf(f, "}\n");
}

// Generate all regulatory architectures from a topology, storing HRAs
void generate_all_regulatory_from_topology(
    Graph *topology, unsigned long long *all_ras_count,
    unsigned long long *hras_count, int *global_seen_reps,
    long long *global_seen_count, int *hra_seen_reps, long long *hra_seen_count,
    GraphCollection *hra_collection) {
  int n = topology->n_nodes, e = topology->n_edges;
  if (e > 32)
    return;
  unsigned long long total = 1ULL << e;
  for (unsigned long long pat = 0; pat < total; pat++) {
    Graph reg = *topology;
    for (int i = 0; i < e; i++) {
      int r = (pat >> i) & 1;
      reg.edges[i].regulation = r;
      reg.adj_matrix[reg.edges[i].from][reg.edges[i].to] = r;
    }
    int canon[MAX_NODES * MAX_NODES];
    compute_canonical_rep(&reg, canon);
    bool seen = false;
    for (long long i = 0; i < *global_seen_count; i++) {
      if (memcmp(global_seen_reps + i * MAX_NODES * MAX_NODES, canon,
                 n * n * sizeof(int)) == 0) {
        seen = true;
        break;
      }
    }
    if (!seen && *global_seen_count < MAX_GRAPHS) {
      memcpy(global_seen_reps + (*global_seen_count) * MAX_NODES * MAX_NODES,
             canon, n * n * sizeof(int));
      (*global_seen_count)++;
      (*all_ras_count)++;
      if (is_heritable_regulatory(&reg)) {
        bool hra_seen = false;
        for (long long i = 0; i < *hra_seen_count; i++) {
          if (memcmp(hra_seen_reps + i * MAX_NODES * MAX_NODES, canon,
                     n * n * sizeof(int)) == 0) {
            hra_seen = true;
            break;
          }
        }
        if (!hra_seen && *hra_seen_count < MAX_GRAPHS) {
          memcpy(hra_seen_reps + (*hra_seen_count) * MAX_NODES * MAX_NODES,
                 canon, n * n * sizeof(int));
          (*hra_seen_count)++;
          (*hras_count)++;
          add_graph_to_collection(hra_collection, &reg);
        }
      }
    }
  }
}

// Create graph collection
GraphCollection *create_graph_collection(void) {
  GraphCollection *gc = malloc(sizeof(GraphCollection));
  if (!gc)
    return NULL;

  gc->count = 0;
  gc->capacity = 1024; // Initial capacity
  gc->graphs = malloc(gc->capacity * sizeof(Graph));

  if (!gc->graphs) {
    free(gc);
    return NULL;
  }
  return gc;
}

// Free graph collection
void free_graph_collection(GraphCollection *gc) {
  if (gc) {
    free(gc->graphs);
    free(gc);
  }
}

// Add graph to collection
bool add_graph_to_collection(GraphCollection *gc, Graph *g) {
  if (gc->count >= MAX_GRAPHS)
    return false;

  if (gc->count >= gc->capacity) {
    // Double capacity when needed
    int new_capacity = gc->capacity * 2;
    if (new_capacity > MAX_GRAPHS)
      new_capacity = MAX_GRAPHS;

    Graph *new_graphs = realloc(gc->graphs, new_capacity * sizeof(Graph));
    if (!new_graphs)
      return false;

    gc->graphs = new_graphs;
    gc->capacity = new_capacity;
  }

  gc->graphs[gc->count] = *g;
  gc->count++;
  return true;
}

// Main optimized HRA generation with per-file DOT export and directory
void generate_hras(int n, bool verbose) {
  if (n <= 0 || n > MAX_NODES) {
    printf("Error: n must be between 1 and %d\n", MAX_NODES);
    return;
  }
  if (n == 1) {
    printf("\nResults for n=%d...\n", n);
    return;
  }

  // Prepare output directory
  const char *out_dir = "hras_dot_files";
  if (mkdir(out_dir, 0755) != 0 && errno != EEXIST) {
    perror("mkdir");
    exit(1);
  }

  // Allocate reps arrays
  int *seen_reps = malloc(MAX_GRAPHS * MAX_NODES * MAX_NODES * sizeof(int));
  int *global_seen_reps =
      malloc(MAX_GRAPHS * MAX_NODES * MAX_NODES * sizeof(int));
  int *hra_seen_reps = malloc(MAX_GRAPHS * MAX_NODES * MAX_NODES * sizeof(int));

  if (!seen_reps || !global_seen_reps || !hra_seen_reps) {
    printf("Memory allocation failed\n");
    free(seen_reps);
    free(global_seen_reps);
    free(hra_seen_reps);
    return;
  }

  // Create graph collections on heap
  GraphCollection *topologies = create_graph_collection();
  GraphCollection *hra_collection = create_graph_collection();

  if (!topologies || !hra_collection) {
    printf("Failed to create graph collections\n");
    free(seen_reps);
    free(global_seen_reps);
    free(hra_seen_reps);
    if (topologies)
      free_graph_collection(topologies);
    if (hra_collection)
      free_graph_collection(hra_collection);
    return;
  }

  long long seen_count = 0;
  long long global_seen_count = 0;
  long long hra_seen_count = 0;
  unsigned long long all_ras_count = 0;
  unsigned long long hras_count = 0;

  clock_t start_time = clock();

  // Generate all possible edges
  Edge all_edges[MAX_EDGES];
  int total_edges = generate_all_edges(n, all_edges);

  printf("Generating topologies for n=%d...\n", n);

  // Generate all valid topologies with different numbers of edges
  for (int k = n; k <= total_edges; k++) {
    Edge current_combo[MAX_EDGES];
    generate_edge_combinations(all_edges, total_edges, k, 0, current_combo, 0,
                               topologies, n, seen_reps, &seen_count);
  }

  printf("Found %d canonical topologies\n", topologies->count);

  // Generate all regulatory architectures from each topology
  for (int i = 0; i < topologies->count; i++) {
    if (verbose && (i % 100 == 0 || i == topologies->count - 1)) {
      printf("Processing topology %d/%d...\n", i + 1, topologies->count);
    }
    generate_all_regulatory_from_topology(
        &topologies->graphs[i], &all_ras_count, &hras_count, global_seen_reps,
        &global_seen_count, hra_seen_reps, &hra_seen_count, hra_collection);
  }

  // Write HRA graphs to DOT files
  if (hra_collection->count > 0) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/hras_n%d.dot", out_dir, n);
    FILE *f = fopen(filename, "w");
    if (f) {
      for (int i = 0; i < hra_collection->count; i++) {
        write_graph_dot(&hra_collection->graphs[i], f, i + 1, n);
        if (i < hra_collection->count - 1) {
          fprintf(f, "\n");
        }
      }
      fclose(f);
      printf("HRA graphs written to %s\n", filename);
    } else {
      printf("Failed to open %s for writing\n", filename);
    }
  }

  clock_t end_time = clock();
  double elapsed = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

  printf("\nResults for n=%d:\n", n);
  printf("  Canonical topologies: %d\n", topologies->count);
  printf("  Total regulatory architectures: %llu\n", all_ras_count);
  printf("  Heritable regulatory architectures (HRAs): %llu\n", hras_count);
  printf("  Time: %.2f seconds\n", elapsed);

  // Cleanup
  free(seen_reps);
  free(global_seen_reps);
  free(hra_seen_reps);
  free_graph_collection(topologies);
  free_graph_collection(hra_collection);
}

// Function that can be called to count HRAs by size
void count_hras_by_size(void) {
  printf("Counting HRAs by network size...\n");
  for (int n = 2; n <= 5; n++) {
    generate_hras(n, false);
  }
}

// Main function
int main(int argc, char *argv[]) {
  int n = 3; // default value
  bool verbose = false;

  // Parse command line arguments
  if (argc > 1) {
    n = atoi(argv[1]);
    if (n <= 0 || n > MAX_NODES) {
      printf("Usage: %s [n] [verbose]\n", argv[0]);
      printf("  n: number of nodes (1-%d), default=3\n", MAX_NODES);
      printf("  verbose: any second argument enables verbose output\n");
      return 1;
    }
  }

  if (argc > 2) {
    verbose = true;
  }

  printf("HRA Generator - Heritable Regulatory Architectures\n");
  printf("================================================\n");

  if (argc == 1) {
    // If no arguments, run for multiple sizes
    count_hras_by_size();
  } else {
    // Run for specific size
    generate_hras(n, verbose);
  }

  return 0;
}
