#ifndef HRA_TEST3_H
#define HRA_TEST3_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NODES 10
#define MAX_EDGES 90        // MAX_NODES * (MAX_NODES - 1)
#define MAX_GRAPHS 10000000 // 10 million

// Edge structure
typedef struct {
  int from;
  int to;
  int regulation; // 0 = gray, 1 = black
} Edge;

// Graph structure
typedef struct {
  int n_nodes;
  int n_edges;
  Edge edges[MAX_EDGES];
  int adj_matrix[MAX_NODES][MAX_NODES]; // -1: no edge, 0: gray, 1: black
  int in_degree[MAX_NODES];
  int out_degree[MAX_NODES];
} Graph;

// Graph collection
typedef struct {
  Graph *graphs; // Now dynamically allocated
  int count;
  int capacity;
} GraphCollection;

// Function declarations
void init_graph(Graph *g, int n_nodes);
void add_edge(Graph *g, int from, int to, int regulation);
bool is_weakly_connected(Graph *g);
bool is_heritable_topology(Graph *g);
bool is_heritable_regulatory(Graph *g);
bool next_permutation(int *arr, int n);
int generate_all_edges(int n, Edge *all_edges);
void generate_edge_combinations(Edge *all_edges, int total_edges, int k,
                                int start, Edge *current_combo, int combo_idx,
                                GraphCollection *topologies, int n_nodes,
                                int *seen_reps, long long *seen_count);
void generate_all_regulatory_from_topology(
    Graph *topology, unsigned long long *all_ras_count,
    unsigned long long *hras_count, int *global_seen_reps,
    long long *global_seen_count, int *hra_seen_reps, long long *hra_seen_count,
    GraphCollection *hra_collection);
void generate_hras(int n, bool verbose);
void count_hras_by_size(void);
void compute_canonical_rep(Graph *g, int *canon);
GraphCollection *create_graph_collection(void);
void free_graph_collection(GraphCollection *gc);
bool add_graph_to_collection(GraphCollection *gc, Graph *g);

#endif // HRA_TEST3_H
