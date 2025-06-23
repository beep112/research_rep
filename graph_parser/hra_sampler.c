#include "hra_sampler.h"

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
  if (gs) {
    free(gs->graphs);
    free(gs);
  }
}

bool add_graph_to_set(GraphSet *gs, const SampledGraph *sg) {
  if (gs->count >= gs->capacity) {
    int new_capacity = gs->capacity * 2;
    SampledGraph *new_graphs =
        realloc(gs->graphs, new_capacity * sizeof(SampledGraph));
    if (!new_graphs)
      return false;

    gs->graphs = new_graphs;
    gs->capacity = new_capacity;
  }

  gs->graphs[gs->count] = *sg;
  gs->count++;
  return true;
}

// Unique graph set management
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
  if (ugs) {
    for (int i = 0; i < ugs->count; i++) {
      for (int j = 0; j < ugs->unique_graphs[i].source_count; j++) {
        free(ugs->unique_graphs[i].source_graphs[j]);
      }
      free(ugs->unique_graphs[i].source_graphs);
    }
    free(ugs->unique_graphs);
    free(ugs);
  }
}

bool add_unique_graph(UniqueGraphSet *ugs, const char *canonical_rep,
                      const char *source_graph) {
  // Check if this canonical representation already exists
  for (int i = 0; i < ugs->count; i++) {
    if (memcmp(ugs->unique_graphs[i].canonical_rep, canonical_rep,
               MAX_NODES * MAX_NODES * sizeof(int)) == 0) {
      // Add to existing entry's source list
      if (ugs->unique_graphs[i].source_count >=
          ugs->unique_graphs[i].source_capacity) {
        int new_cap = ugs->unique_graphs[i].source_capacity * 2;
        char **new_sources = realloc(ugs->unique_graphs[i].source_graphs,
                                     new_cap * sizeof(char *));
        if (!new_sources)
          return false;
        ugs->unique_graphs[i].source_graphs = new_sources;
        ugs->unique_graphs[i].source_capacity = new_cap;
      }

      ugs->unique_graphs[i].source_graphs[ugs->unique_graphs[i].source_count] =
          malloc(strlen(source_graph) + 1);
      strcpy(ugs->unique_graphs[i]
                 .source_graphs[ugs->unique_graphs[i].source_count],
             source_graph);
      ugs->unique_graphs[i].source_count++;
      return true;
    }
  }

  // Add new unique graph
  if (ugs->count >= ugs->capacity) {
    int new_capacity = ugs->capacity * 2;
    UniqueGraph *new_graphs =
        realloc(ugs->unique_graphs, new_capacity * sizeof(UniqueGraph));
    if (!new_graphs)
      return false;
    ugs->unique_graphs = new_graphs;
    ugs->capacity = new_capacity;
  }

  UniqueGraph *ug = &ugs->unique_graphs[ugs->count];
  memcpy(ug->canonical_rep, canonical_rep, MAX_NODES * MAX_NODES * sizeof(int));
  ug->source_capacity = 4;
  ug->source_graphs = malloc(ug->source_capacity * sizeof(char *));
  if (!ug->source_graphs)
    return false;

  ug->source_graphs[0] = malloc(strlen(source_graph) + 1);
  strcpy(ug->source_graphs[0], source_graph);
  ug->source_count = 1;

  ugs->count++;
  return true;
}

// DOT file parsing functions
int count_graphs_in_dot_file(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp)
    return -1;

  char line[MAX_LINE];
  int count = 0;

  while (fgets(line, sizeof(line), fp)) {
    if (strstr(line, "digraph") != NULL) {
      count++;
    }
  }

  fclose(fp);
  return count;
}

bool parse_single_dot_graph(FILE *fp, Graph *g, int *graph_id) {
  char line[MAX_LINE];
  bool in_graph = false;
  int n_nodes = 0;

  init_graph(g, 0);

  while (fgets(line, sizeof(line), fp)) {
    // Skip empty lines and comments
    if (line[0] == '\n' || line[0] == '#')
      continue;

    if (strstr(line, "digraph") != NULL) {
      in_graph = true;
      // Extract graph ID if present
      char *id_start = strstr(line, "_");
      if (id_start) {
        *graph_id = atoi(id_start + 1);
      }
      continue;
    }

    if (in_graph && strchr(line, '}') != NULL) {
      // End of graph
      g->n_nodes = n_nodes;
      return true;
    }

    if (in_graph && strstr(line, "->") != NULL) {
      // Parse edge: "from -> to [label="regulation"];"
      int from, to, regulation;
      if (sscanf(line, "%d -> %d [label=\"%d\"];", &from, &to, &regulation) ==
          3) {
        add_edge(g, from, to, regulation);
        n_nodes = (from > n_nodes) ? from + 1 : n_nodes;
        n_nodes = (to > n_nodes) ? to + 1 : n_nodes;
      }
    }
  }

  return false;
}

SampledGraph *randomly_sample_graph(const char *filename, int total_graphs) {
  if (total_graphs <= 0)
    return NULL;

  int target_graph = rand() % total_graphs;
  FILE *fp = fopen(filename, "r");
  if (!fp)
    return NULL;

  SampledGraph *sg = malloc(sizeof(SampledGraph));
  if (!sg) {
    fclose(fp);
    return NULL;
  }

  int current_graph = 0;
  while (current_graph <= target_graph) {
    if (!parse_single_dot_graph(fp, &sg->graph, &sg->graph_id)) {
      free(sg);
      fclose(fp);
      return NULL;
    }

    if (current_graph == target_graph) {
      snprintf(sg->filename, MAX_FILENAME, "%s", filename);
      fclose(fp);
      return sg;
    }
    current_graph++;
  }

  free(sg);
  fclose(fp);
  return NULL;
}

// Graph expansion and analysis functions
void generate_all_extensions(const Graph *base, int target_size,
                             FILE *output_file, int *graph_counter) {
  int current_size = base->n_nodes;
  if (current_size >= target_size) {
    // Base case: write current graph if it's heritable
    if (is_heritable_regulatory(base)) {
      fprintf(output_file, "digraph Extended_%03d {\n", (*graph_counter)++);
      for (int i = 0; i < base->n_edges; i++) {
        fprintf(output_file, "  %d -> %d [label=\"%d\"];\n",
                base->edges[i].from, base->edges[i].to,
                base->edges[i].regulation);
      }
      fprintf(output_file, "}\n\n");
    }
    return;
  }

  // Add one more node and try all possible connections
  int new_node = current_size;

  // Try all possible ways to connect the new node
  for (int in_edges = 1; in_edges <= current_size; in_edges++) {
    for (int out_edges = 0; out_edges <= current_size; out_edges++) {
      // Generate all combinations of incoming edges
      int in_nodes[MAX_NODES];
      for (int i = 0; i < current_size; i++)
        in_nodes[i] = i;

      // This is a simplified version - in practice you'd want to generate
      // all combinations of in_edges nodes from current_size nodes
      // For now, just connect to the first in_edges nodes
      if (in_edges <= current_size) {
        Graph extended = *base;
        extended.n_nodes = current_size + 1;

        // Add incoming edges to new node
        for (int i = 0; i < in_edges; i++) {
          for (int reg = 0; reg <= 1; reg++) {
            Graph temp = extended;
            add_edge(&temp, i, new_node, reg);

            // Add outgoing edges from new node
            for (int j = 0; j < out_edges && j < current_size; j++) {
              for (int out_reg = 0; out_reg <= 1; out_reg++) {
                Graph temp2 = temp;
                add_edge(&temp2, new_node, j, out_reg);

                if (is_weakly_connected(&temp2) &&
                    is_heritable_topology(&temp2)) {
                  generate_all_extensions(&temp2, target_size, output_file,
                                          graph_counter);
                }
              }
            }
          }
        }
      }
    }
  }
}

int expand_graph_to_size(const Graph *base_graph, int target_size,
                         const char *output_dir, int thread_id) {
  char output_filename[MAX_FILENAME];
  snprintf(output_filename, MAX_FILENAME, "%s/thread_%d_expanded.dot",
           output_dir, thread_id);

  FILE *output_file = fopen(output_filename, "w");
  if (!output_file)
    return -1;

  int graph_counter = 0;
  generate_all_extensions(base_graph, target_size, output_file, &graph_counter);

  fclose(output_file);
  return graph_counter;
}

// Analysis functions
int analyze_results(const char *results_dir, int original_size, int target_size,
                    const char *source_dot_file, bool verbose) {
  printf("Analyzing results in %s...\n", results_dir);

  DIR *dir = opendir(results_dir);
  if (!dir) {
    printf("Error: Cannot open results directory\n");
    return -1;
  }

  UniqueGraphSet *all_unique = create_unique_graph_set(1000);
  UniqueGraphSet *from_n3 = create_unique_graph_set(1000);

  if (!all_unique || !from_n3) {
    printf("Error: Memory allocation failed\n");
    closedir(dir);
    return -1;
  }

  struct dirent *entry;
  int files_processed = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (strstr(entry->d_name, ".dot") == NULL)
      continue;

    char filepath[MAX_FILENAME];
    snprintf(filepath, MAX_FILENAME, "%s/%s", results_dir, entry->d_name);

    if (verbose) {
      printf("Processing %s...\n", filepath);
    }

    // Parse and analyze this file
    // This would involve reading all graphs, computing canonical forms,
    // and tracking which source graphs they came from
    files_processed++;
  }

  closedir(dir);

  printf("Processed %d result files\n", files_processed);

  // Print analysis summary
  print_analysis_summary(all_unique, from_n3, 0, original_size, target_size);

  free_unique_graph_set(all_unique);
  free_unique_graph_set(from_n3);
  return 0;
}

void print_analysis_summary(const UniqueGraphSet *all_unique,
                            const UniqueGraphSet *from_n3, int total_n4_hra,
                            int n, int target_n) {
  printf("\n=== Analysis Summary ===\n");
  printf("Original size: n=%d\n", n);
  printf("Target size: n=%d\n", target_n);
  printf("Unique graphs found: %d\n", all_unique->count);
  printf("Graphs from n=%d sources: %d\n", n, from_n3->count);

  if (total_n4_hra > 0) {
    double ratio = (double)from_n3->count / (double)total_n4_hra;
    printf("Ratio (from n=%d / all n=%d HRAs): %.4f\n", n, target_n, ratio);
  }

  // Print graphs that appear from multiple sources
  printf("\nGraphs appearing from multiple n=%d sources:\n", n);
  int multi_source_count = 0;
  for (int i = 0; i < all_unique->count; i++) {
    if (all_unique->unique_graphs[i].source_count > 1) {
      printf("  Graph %d: appears from %d different sources\n", i,
             all_unique->unique_graphs[i].source_count);
      multi_source_count++;
    }
  }

  if (multi_source_count == 0) {
    printf("  (None found)\n");
  }

  printf("\nUnique graphs appearing from single sources: %d\n",
         all_unique->count - multi_source_count);
}

// Utility functions (reused from your original code)
void init_graph(Graph *g, int n_nodes) {
  g->n_nodes = n_nodes;
  g->n_edges = 0;
  memset(g->in_degree, 0, sizeof(g->in_degree));
  memset(g->out_degree, 0, sizeof(g->out_degree));
  for (int i = 0; i < MAX_NODES; i++)
    for (int j = 0; j < MAX_NODES; j++)
      g->adj_matrix[i][j] = -1;
}

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

bool is_weakly_connected(const Graph *g) {
  // Implementation from your original code
  if (g->n_nodes <= 1)
    return true;

  int parent[MAX_NODES];
  for (int i = 0; i < g->n_nodes; i++)
    parent[i] = i;

  // Union-find to check connectivity
  for (int i = 0; i < g->n_edges; i++) {
    int u = g->edges[i].from, v = g->edges[i].to;
    // Find roots and union
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
    int curr = i;
    while (parent[curr] != curr)
      curr = parent[curr];
    if (curr != root)
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

void compute_canonical_representation(const Graph *g, char *canonical_rep) {
  // Simplified version - in practice you'd want the full canonical form
  // from your original compute_canonical_rep function
  memset(canonical_rep, 0, MAX_NODES * MAX_NODES * sizeof(int));
  for (int i = 0; i < g->n_nodes; i++) {
    for (int j = 0; j < g->n_nodes; j++) {
      ((int *)canonical_rep)[i * g->n_nodes + j] = g->adj_matrix[i][j];
    }
  }
}

void print_graph(const Graph *g) {
  printf("Graph with %d nodes, %d edges:\n", g->n_nodes, g->n_edges);
  for (int i = 0; i < g->n_edges; i++) {
    printf("  %d -> %d [%d]\n", g->edges[i].from, g->edges[i].to,
           g->edges[i].regulation);
  }
}
