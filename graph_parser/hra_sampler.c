
#include "hra_sampler.h"
int analyze_results(const char *results_dir, int original_size, int target_size,
                    const char *source_dot_file, bool verbose) {
  printf("Analyzing results in %s...\n", results_dir);
  DIR *dir = opendir(results_dir);
  if (!dir)
    return -1;

  UniqueGraphSet *all_unique = create_unique_graph_set(1000);
  UniqueGraphSet *from_n3 = create_unique_graph_set(1000);
  struct dirent *entry;
  int files_processed = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (!strstr(entry->d_name, ".dot"))
      continue;
    char path[MAX_FILENAME];
    snprintf(path, MAX_FILENAME, "%s/%s", results_dir, entry->d_name);
    if (verbose)
      printf(" Processing %s...\n", path);

    FILE *fp = fopen(path, "r");
    if (!fp)
      continue;

    Graph g;
    int graph_id;
    while (parse_single_dot_graph(fp, &g, &graph_id)) {
      char canonical_rep[MAX_NODES * MAX_NODES * sizeof(int)];
      compute_canonical_representation(&g, canonical_rep);

      char source_tag[32];
      sscanf(entry->d_name, "%*[^_]_%*[^_]_%d.dot", &graph_id);
      snprintf(source_tag, sizeof(source_tag), "graph_%03d", graph_id);

      add_unique_graph(all_unique, canonical_rep, source_tag);
      add_unique_graph(from_n3, canonical_rep, source_tag);
    }
    fclose(fp);
    files_processed++;
  }
  closedir(dir);

  printf("Processed %d result files\n", files_processed);
  print_analysis_summary(all_unique, from_n3, 5604, original_size, target_size);

  // Dump frequency CSV
  FILE *csv = fopen("hra_stats.csv", "w");
  if (csv) {
    fprintf(csv, "CanonicalID,SourceCount\n");
    for (int i = 0; i < all_unique->count; i++) {
      fprintf(csv, "%d,%d\n", i, all_unique->unique_graphs[i].source_count);
    }
    fclose(csv);
  }

  free_unique_graph_set(all_unique);
  free_unique_graph_set(from_n3);
  return 0;
}

// Graph set management functions
GraphSet *create_graph_set(int initial_capacity) {
  GraphSet *gs = malloc(sizeof(GraphSet));
  if (!gs)
    return NULL;
  gs->graphs = malloc(initial_capacity * sizeof(SampledGraph));
  if (!gs->graphs) {
    free(gs);
    return NULL;
  }
  gs->count = 0;
  gs->capacity = initial_capacity;
  return gs;
}

void free_graph_set(GraphSet *gs) {
  if (!gs)
    return;
  free(gs->graphs);
  free(gs);
}

bool add_graph_to_set(GraphSet *gs, const SampledGraph *sg) {
  if (gs->count >= gs->capacity) {
    int new_cap = gs->capacity * 2;
    SampledGraph *tmp = realloc(gs->graphs, new_cap * sizeof(SampledGraph));
    if (!tmp)
      return false;
    gs->graphs = tmp;
    gs->capacity = new_cap;
  }
  gs->graphs[gs->count++] = *sg;
  return true;
}

// Unique graph set
UniqueGraphSet *create_unique_graph_set(int initial_capacity) {
  UniqueGraphSet *ugs = malloc(sizeof(UniqueGraphSet));
  if (!ugs)
    return NULL;
  ugs->unique_graphs = malloc(initial_capacity * sizeof(UniqueGraph));
  if (!ugs->unique_graphs) {
    free(ugs);
    return NULL;
  }
  ugs->count = 0;
  ugs->capacity = initial_capacity;
  return ugs;
}

void free_unique_graph_set(UniqueGraphSet *ugs) {
  if (!ugs)
    return;
  for (int i = 0; i < ugs->count; i++) {
    for (int j = 0; j < ugs->unique_graphs[i].source_count; j++) {
      free(ugs->unique_graphs[i].source_graphs[j]);
    }
    free(ugs->unique_graphs[i].source_graphs);
  }
  free(ugs->unique_graphs);
  free(ugs);
}

bool add_unique_graph(UniqueGraphSet *ugs, const char *canonical_rep,
                      const char *source_graph) {
  for (int i = 0; i < ugs->count; i++) {
    if (memcmp(ugs->unique_graphs[i].canonical_rep, canonical_rep,
               MAX_NODES * MAX_NODES * sizeof(int)) == 0) {
      UniqueGraph *ug = &ugs->unique_graphs[i];
      if (ug->source_count >= ug->source_capacity) {
        int nc = ug->source_capacity * 2;
        char **tmp = realloc(ug->source_graphs, nc * sizeof(char *));
        if (!tmp)
          return false;
        ug->source_graphs = tmp;
        ug->source_capacity = nc;
      }
      ug->source_graphs[ug->source_count] = strdup(source_graph);
      ug->source_count++;
      return true;
    }
  }
  if (ugs->count >= ugs->capacity) {
    int nc = ugs->capacity * 2;
    UniqueGraph *tmp = realloc(ugs->unique_graphs, nc * sizeof(UniqueGraph));
    if (!tmp)
      return false;
    ugs->unique_graphs = tmp;
    ugs->capacity = nc;
  }
  UniqueGraph *ng = &ugs->unique_graphs[ugs->count];
  memcpy(ng->canonical_rep, canonical_rep, MAX_NODES * MAX_NODES * sizeof(int));
  ng->source_capacity = 4;
  ng->source_graphs = malloc(ng->source_capacity * sizeof(char *));
  ng->source_graphs[0] = strdup(source_graph);
  ng->source_count = 1;
  ugs->count++;
  return true;
}

// DOT parsing
int count_graphs_in_dot_file(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp)
    return -1;
  char line[MAX_LINE];
  int c = 0;
  while (fgets(line, sizeof(line), fp))
    if (strstr(line, "digraph"))
      c++;
  fclose(fp);
  return c;
}

bool parse_single_dot_graph(FILE *fp, Graph *g, int *graph_id) {
  char line[MAX_LINE];
  bool in_graph = false;
  int max_node_id = -1; // Track the highest node ID seen
  init_graph(g, 0);

  while (fgets(line, sizeof(line), fp)) {
    if (!in_graph && strstr(line, "digraph")) {
      in_graph = true;
      char *p = strchr(line, '_');
      if (p)
        *graph_id = atoi(p + 1);
      continue;
    }
    if (in_graph && strchr(line, '}')) {
      // Number of nodes is max_node_id + 1 (since nodes are 0-indexed)
      g->n_nodes = max_node_id + 1;
      return true;
    }
    if (in_graph && strstr(line, "->")) {
      int f, t, r;
      if (sscanf(line, "%d -> %d [label=\"%d\"];", &f, &t, &r) == 3) {
        add_edge(g, f, t, r);
        // Update max_node_id to track the highest node ID
        if (f > max_node_id)
          max_node_id = f;
        if (t > max_node_id)
          max_node_id = t;
      }
    }
  }
  return false;
}
// Graph utilities
void init_graph(Graph *g, int n) {
  g->n_nodes = n;
  g->n_edges = 0;
  memset(g->in_degree, 0, sizeof(g->in_degree));
  memset(g->out_degree, 0, sizeof(g->out_degree));
  for (int i = 0; i < MAX_NODES; i++) {
    for (int j = 0; j < MAX_NODES; j++) {
      g->adj_matrix[i][j] = -1; // -1 means no edge
    }
  }
}

void add_edge(Graph *g, int from, int to, int regulation) {
  if (g->n_edges >= MAX_EDGES)
    return;
  g->edges[g->n_edges] = (Edge){from, to, regulation};
  g->adj_matrix[from][to] = regulation;
  g->out_degree[from]++;
  g->in_degree[to]++;
  g->n_edges++;
}

bool is_weakly_connected(const Graph *g) {
  if (g->n_nodes <= 1)
    return true;
  int parent[MAX_NODES];
  for (int i = 0; i < g->n_nodes; i++)
    parent[i] = i;
  for (int i = 0; i < g->n_edges; i++) {
    int u = g->edges[i].from, v = g->edges[i].to;
    while (parent[u] != u)
      u = parent[u];
    while (parent[v] != v)
      v = parent[v];
    if (u != v)
      parent[u] = v;
  }
  int root = 0;
  while (parent[root] != root)
    root = parent[root];
  for (int i = 1; i < g->n_nodes; i++) {
    int x = i;
    while (parent[x] != x)
      x = parent[x];
    if (x != root)
      return false;
  }
  return true;
}

bool is_heritable_topology(const Graph *g) {
  for (int i = 0; i < g->n_nodes; i++)
    if (g->in_degree[i] == 0)
      return false;
  return true;
}

bool is_heritable_regulatory(const Graph *g) {
  for (int i = 0; i < g->n_nodes; i++) {
    bool gray = false;
    for (int j = 0; j < g->n_nodes; j++)
      if (g->adj_matrix[j][i] == 0) {
        gray = true;
        break;
      }
    if (!gray)
      return false;
  }
  return true;
}

void compute_canonical_representation(const Graph *g, char *canonical_rep) {
  memset(canonical_rep, 0, MAX_NODES * MAX_NODES * sizeof(int));
  for (int i = 0; i < g->n_nodes; i++)
    for (int j = 0; j < g->n_nodes; j++)
      ((int *)canonical_rep)[i * g->n_nodes + j] = g->adj_matrix[i][j];
}

void print_analysis_summary(const UniqueGraphSet *all_unique,
                            const UniqueGraphSet *from_n3, int total_n4_hra,
                            int n, int target_n) {
  printf("=== Analysis Summary ===");
  printf("Original: n=%d, Target: n=%d", n, target_n);
  printf("Unique graphs total: %d", all_unique->count);
  printf("From n=%d sources: %d", n, from_n3->count);
  if (total_n4_hra > 0) {
    double r = (double)from_n3->count / total_n4_hra;
    printf("Ratio: %.4f (from/total)", r);
  }
  printf("Graphs appearing >1 sources:");
  int multi = 0;
  for (int i = 0; i < all_unique->count; i++) {
    if (all_unique->unique_graphs[i].source_count > 1) {
      printf("  Graph %d: %d sources", i,
             all_unique->unique_graphs[i].source_count);
      multi++;
    }
  }
  if (!multi)
    printf("  (none)");
}
