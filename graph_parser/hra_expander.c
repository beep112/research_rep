#include "hra_sampler.h"

// This is the subprocess that expands individual graphs
int main(int argc, char *argv[]) {
  if (argc != 6) {
    printf("Usage: %s <input_dot_file> <graph_index> <output_file> "
           "<start_size> <target_size>\n");
    return 1;
  }

  const char *input_file = argv[1];
  int graph_index = atoi(argv[2]);
  const char *output_file = argv[3];
  int start_size = atoi(argv[4]);
  int target_size = atoi(argv[5]);

  // Load the specific graph from the dot file
  FILE *input_fp = fopen(input_file, "r");
  if (!input_fp) {
    fprintf(stderr, "Error: Cannot open input file %s\n", input_file);
    return 1;
  }

  // Skip to the desired graph
  Graph base_graph;
  int current_index = 0;
  int graph_id;

  while (current_index <= graph_index) {
    if (!parse_single_dot_graph(input_fp, &base_graph, &graph_id)) {
      fprintf(stderr, "Error: Cannot find graph at index %d\n", graph_index);
      fclose(input_fp);
      return 1;
    }
    if (current_index == graph_index)
      break;
    current_index++;
  }
  fclose(input_fp);

  // Open output file
  FILE *output_fp = fopen(output_file, "w");
  if (!output_fp) {
    fprintf(stderr, "Error: Cannot open output file %s\n", output_file);
    return 1;
  }

  // Generate all possible extensions
  int graphs_generated = 0;
  expand_single_graph(&base_graph, target_size, output_fp, &graphs_generated,
                      graph_index);

  fclose(output_fp);

  printf("Expander: Generated %d graphs from source graph %d\n",
         graphs_generated, graph_index);
  return 0;
}

// Function to expand a single graph to target size
void expand_single_graph(const Graph *base, int target_size, FILE *output,
                         int *counter, int source_id) {
  if (base->n_nodes >= target_size) {
    // Already at target size, check if it's a valid HRA
    if (is_heritable_regulatory(base)) {
      fprintf(output, "digraph HRA_from_%d_graph_%03d {\n", source_id,
              (*counter)++);
      for (int i = 0; i < base->n_edges; i++) {
        fprintf(output, "  %d -> %d [label=\"%d\"];\n", base->edges[i].from,
                base->edges[i].to, base->edges[i].regulation);
      }
      fprintf(output, "}\n\n");
    }
    return;
  }

  // Add one node at a time
  int new_node = base->n_nodes;

  // Try all possible ways to connect the new node
  // For each existing node, decide if it connects to/from the new node
  int max_connections =
      1 << (2 * base->n_nodes); // 2 bits per existing node (in/out)

  for (int connection_pattern = 1; connection_pattern < max_connections;
       connection_pattern++) {
    Graph extended = *base;
    extended.n_nodes = base->n_nodes + 1;

    bool has_incoming = false;
    bool valid_extension = true;

    // Add connections based on the pattern
    for (int existing_node = 0; existing_node < base->n_nodes;
         existing_node++) {
      int in_bit = (connection_pattern >> (2 * existing_node)) & 1;
      int out_bit = (connection_pattern >> (2 * existing_node + 1)) & 1;

      if (in_bit) {
        // Add edge from existing_node to new_node
        // Try both regulatory types
        for (int reg = 0; reg <= 1; reg++) {
          Graph temp = extended;
          add_edge(&temp, existing_node, new_node, reg);
          has_incoming = true;

          if (out_bit) {
            // Also add edge from new_node to existing_node
            for (int out_reg = 0; out_reg <= 1; out_reg++) {
              Graph temp2 = temp;
              add_edge(&temp2, new_node, existing_node, out_reg);

              if (is_weakly_connected(&temp2) &&
                  is_heritable_topology(&temp2)) {
                expand_single_graph(&temp2, target_size, output, counter,
                                    source_id);
              }
            }
          } else {
            // Only incoming edge
            if (is_weakly_connected(&temp) && is_heritable_topology(&temp)) {
              expand_single_graph(&temp, target_size, output, counter,
                                  source_id);
            }
          }
        }
      } else if (out_bit) {
        // Only outgoing edge from new_node to existing_node
        for (int out_reg = 0; out_reg <= 1; out_reg++) {
          Graph temp = extended;
          add_edge(&temp, new_node, existing_node, out_reg);

          if (is_weakly_connected(&temp) && is_heritable_topology(&temp)) {
            expand_single_graph(&temp, target_size, output, counter, source_id);
          }
        }
      }
    }

    // Must have at least one incoming edge to maintain heritability
    if (!has_incoming)
      continue;
  }
}

// Optimized version for large graphs - limits the search space
void expand_single_graph_limited(const Graph *base, int target_size,
                                 FILE *output, int *counter, int source_id) {
  if (base->n_nodes >= target_size) {
    if (is_heritable_regulatory(base)) {
      fprintf(output, "digraph HRA_from_%d_graph_%03d {\n", source_id,
              (*counter)++);
      for (int i = 0; i < base->n_edges; i++) {
        fprintf(output, "  %d -> %d [label=\"%d\"];\n", base->edges[i].from,
                base->edges[i].to, base->edges[i].regulation);
      }
      fprintf(output, "}\n\n");
    }
    return;
  }

  int new_node = base->n_nodes;

  // Limit to reasonable connection patterns to avoid exponential explosion
  // Strategy: new node must have 1-3 incoming edges and 0-2 outgoing edges
  for (int in_degree = 1; in_degree <= 3 && in_degree <= base->n_nodes;
       in_degree++) {
    for (int out_degree = 0; out_degree <= 2 && out_degree <= base->n_nodes;
         out_degree++) {

      // Generate combinations of nodes for incoming edges
      int in_nodes[MAX_NODES];
      generate_node_combinations(base->n_nodes, in_degree, in_nodes, 0, 0, base,
                                 new_node, out_degree, target_size, output,
                                 counter, source_id);
    }
  }
}

void generate_node_combinations(int total_nodes, int needed, int *selected,
                                int pos, int start, const Graph *base,
                                int new_node, int out_degree, int target_size,
                                FILE *output, int *counter, int source_id) {
  if (pos == needed) {
    // Generate outgoing combinations
    int out_nodes[MAX_NODES];
    generate_out_combinations(total_nodes, out_degree, out_nodes, 0, 0, base,
                              new_node, selected, needed, target_size, output,
                              counter, source_id);
    return;
  }

  for (int i = start; i < total_nodes; i++) {
    selected[pos] = i;
    generate_node_combinations(total_nodes, needed, selected, pos + 1, i + 1,
                               base, new_node, out_degree, target_size, output,
                               counter, source_id);
  }
}

void generate_out_combinations(int total_nodes, int needed, int *selected,
                               int pos, int start, const Graph *base,
                               int new_node, int *in_nodes, int in_count,
                               int target_size, FILE *output, int *counter,
                               int source_id) {
  if (pos == needed) {
    // Create graph with these connections and all regulatory combinations
    generate_regulatory_combinations(base, new_node, in_nodes, in_count,
                                     selected, needed, target_size, output,
                                     counter, source_id);
    return;
  }

  for (int i = start; i < total_nodes; i++) {
    selected[pos] = i;
    generate_out_combinations(total_nodes, needed, selected, pos + 1, i + 1,
                              base, new_node, in_nodes, in_count, target_size,
                              output, counter, source_id);
  }
}

void generate_regulatory_combinations(const Graph *base, int new_node,
                                      int *in_nodes, int in_count,
                                      int *out_nodes, int out_count,
                                      int target_size, FILE *output,
                                      int *counter, int source_id) {
  int total_new_edges = in_count + out_count;
  if (total_new_edges > 10)
    return; // Limit to prevent explosion

  // Generate all 2^total_new_edges regulatory combinations
  int max_patterns = 1 << total_new_edges;

  for (int pattern = 0; pattern < max_patterns; pattern++) {
    Graph extended = *base;
    extended.n_nodes = base->n_nodes + 1;

    // Add incoming edges
    for (int i = 0; i < in_count; i++) {
      int regulation = (pattern >> i) & 1;
      add_edge(&extended, in_nodes[i], new_node, regulation);
    }

    // Add outgoing edges
    for (int i = 0; i < out_count; i++) {
      int regulation = (pattern >> (in_count + i)) & 1;
      add_edge(&extended, new_node, out_nodes[i], regulation);
    }

    if (is_weakly_connected(&extended) && is_heritable_topology(&extended)) {
      if (extended.n_nodes == target_size) {
        // At target size, check if it's heritable
        if (is_heritable_regulatory(&extended)) {
          fprintf(output, "digraph HRA_from_%d_graph_%03d {\n", source_id,
                  (*counter)++);
          for (int j = 0; j < extended.n_edges; j++) {
            fprintf(output, "  %d -> %d [label=\"%d\"];\n",
                    extended.edges[j].from, extended.edges[j].to,
                    extended.edges[j].regulation);
          }
          fprintf(output, "}\n\n");
        }
      } else {
        // Continue expanding
        expand_single_graph_limited(&extended, target_size, output, counter,
                                    source_id);
      }
    }
  }
}
