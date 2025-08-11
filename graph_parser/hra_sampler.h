#ifndef HRA_SAMPLER_H
#define HRA_SAMPLER_H

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_NODES 10
#define MAX_EDGES 90
#define MAX_GRAPHS 1000000
#define MAX_FILENAME PATH_MAX
#define MAX_LINE 2048
#define MAX_THREADS 16

// Graph structures (reusing from your original code)
typedef struct {
  int from;
  int to;
  int regulation;
} Edge;

typedef struct {
  int n_nodes;
  int n_edges;
  Edge edges[MAX_EDGES];
  int adj_matrix[MAX_NODES][MAX_NODES];
  int in_degree[MAX_NODES];
  int out_degree[MAX_NODES];
} Graph;

// Sampling and analysis structures
typedef struct {
  char filename[MAX_FILENAME];
  int graph_id;
  Graph graph;
} SampledGraph;

typedef struct {
  SampledGraph *graphs;
  int count;
  int capacity;
} GraphSet;

typedef struct {
  int thread_id;
  SampledGraph *input_graph;
  char output_dir[MAX_FILENAME];
  char input_dot_file[MAX_FILENAME];
  int start_node_count;
  int target_node_count;
  int start_graph_index;
  int end_graph_index;
  int total_graphs;
  pthread_mutex_t *print_mutex;
} WorkerThread;

typedef struct {
  char canonical_rep[MAX_NODES * MAX_NODES * sizeof(int)];
  int source_count;
  char **source_graphs;
  int source_capacity;
} UniqueGraph;

typedef struct {
  UniqueGraph *unique_graphs;
  int count;
  int capacity;
} UniqueGraphSet;

// Function declarations
GraphSet *create_graph_set(int initial_capacity);
void free_graph_set(GraphSet *gs);
bool add_graph_to_set(GraphSet *gs, const SampledGraph *sg);

UniqueGraphSet *create_unique_graph_set(int initial_capacity);
void free_unique_graph_set(UniqueGraphSet *ugs);
bool add_unique_graph(UniqueGraphSet *ugs, const char *canonical_rep,
                      const char *source_graph);

int parse_dot_file(const char *filename, GraphSet *graph_set);
bool parse_single_dot_graph(FILE *fp, Graph *g, int *graph_id);
void print_graph(const Graph *g);

int count_graphs_in_dot_file(const char *filename);
SampledGraph *randomly_sample_graph(const char *filename, int total_graphs);

void *worker_thread(void *arg);
int expand_graph_to_size(const Graph *base_graph, int target_size,
                         const char *output_dir, int thread_id);

void generate_all_extensions(const Graph *base, int target_size,
                             FILE *output_file, int *graph_counter);
bool is_valid_extension(const Graph *g);
void compute_canonical_representation(const Graph *g, char *canonical_rep);

int analyze_results(const char *results_dir, int original_size, int target_size,
                    const char *source_dot_file, bool verbose);
void print_analysis_summary(const UniqueGraphSet *all_unique,
                            const UniqueGraphSet *from_n3, int total_n4_hra,
                            int n, int target_n);

bool next_permutation(int *arr, int n);
void init_graph(Graph *g, int n_nodes);
void add_edge(Graph *g, int from, int to, int regulation);
bool is_weakly_connected(const Graph *g);
bool is_heritable_topology(const Graph *g);
bool is_heritable_regulatory(const Graph *g);

// Functions for the expander subprocess
void expand_single_graph(const Graph *base, int target_size, FILE *output,
                         int *counter, int source_id);
void expand_single_graph_limited(const Graph *base, int target_size,
                                 FILE *output, int *counter, int source_id);
void generate_node_combinations(int total_nodes, int needed, int *selected,
                                int pos, int start, const Graph *base,
                                int new_node, int out_degree, int target_size,
                                FILE *output, int *counter, int source_id);
void generate_out_combinations(int total_nodes, int needed, int *selected,
                               int pos, int start, const Graph *base,
                               int new_node, int *in_nodes, int in_count,
                               int target_size, FILE *output, int *counter,
                               int source_id);
void generate_regulatory_combinations(const Graph *base, int new_node,
                                      int *in_nodes, int in_count,
                                      int *out_nodes, int out_count,
                                      int target_size, FILE *output,
                                      int *counter, int source_id);

#endif // HRA_SAMPLER_H
