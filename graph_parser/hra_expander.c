#include "hra_sampler.h"

int main(int argc, char *argv[]) {
  if (argc != 6) {
    printf("Usage: %s <input_dot_file> <graph_index> <output_file> "
           "<start_size> <target_size>\n",
           argv[0]);
    return 1;
  }

  const char *input_file = argv[1];
  int graph_index = atoi(argv[2]);
  const char *output_file = argv[3];
  int target_size = atoi(argv[5]);

  // Load the graph
  FILE *input_fp = fopen(input_file, "r");
  if (!input_fp) {
    fprintf(stderr, "Error: Cannot open input file %s\n", input_file);
    return 1;
  }

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

  // Only check weak connectivity for base graph
  if (!is_weakly_connected(&base_graph)) {
    fprintf(stderr, "Base graph %d is not weakly connected\n", graph_index);
    return 1;
  }

  // Open output file
  FILE *output_fp = fopen(output_file, "w");
  if (!output_fp) {
    fprintf(stderr, "Error: Cannot open output file %s\n", output_file);
    return 1;
  }

  int graphs_generated = 0;
  expand_single_graph(&base_graph, target_size, output_fp, &graphs_generated,
                      graph_index);
  fclose(output_fp);

  printf("Expander: Generated %d graphs from source graph %d\n",
         graphs_generated, graph_index);
  return 0;
}

void expand_single_graph(const Graph *base, int target_size, FILE *output,
                         int *counter, int source_id) {
  // Base case: if we've reached target size
  if (base->n_nodes == target_size) {
    if (is_weakly_connected(base) && is_heritable_topology(base) &&
        is_heritable_regulatory(base)) {

      // Compute canonical representation
      int nn = base->n_nodes * base->n_nodes;
      char canonical_rep[nn * sizeof(int)];
      compute_canonical_representation(base, canonical_rep);

      // Write to output file
      fprintf(output, "// Source:%d Canonical:", source_id);
      for (int i = 0; i < nn; i++) {
        fprintf(output, " %d", ((int *)canonical_rep)[i]);
      }
      fprintf(output, "\n");

      // Output digraph
      fprintf(output, "digraph HRA_from_%d_graph_%03d {\n", source_id,
              (*counter)++);
      for (int e = 0; e < base->n_edges; e++) {
        fprintf(output, "  %d -> %d [label=\"%d\"];\n", base->edges[e].from,
                base->edges[e].to, base->edges[e].regulation);
      }
      fprintf(output, "}\n\n");
    }
    return;
  }

  // Recursive case: expand the graph
  int new_node = base->n_nodes;
  int max_pattern = 1 << (2 * base->n_nodes);

  for (int pattern = 0; pattern < max_pattern; pattern++) {
    Graph extended = *base;
    extended.n_nodes = new_node + 1;

    // Track if we added any edges
    bool edges_added = false;

    // Try all combinations of incoming/outgoing edges
    for (int n = 0; n < base->n_nodes; n++) {
      int in_bit = (pattern >> (2 * n)) & 1;
      int out_bit = (pattern >> (2 * n + 1)) & 1;

      if (in_bit) {
        edges_added = true;
        // Add incoming edge with both regulation types
        for (int reg = 0; reg <= 1; reg++) {
          Graph temp = extended;
          add_edge(&temp, n, new_node, reg);

          if (out_bit) {
            // Add outgoing edge with both regulation types
            for (int out_reg = 0; out_reg <= 1; out_reg++) {
              Graph temp2 = temp;
              add_edge(&temp2, new_node, n, out_reg);
              expand_single_graph(&temp2, target_size, output, counter,
                                  source_id);
            }
          } else {
            expand_single_graph(&temp, target_size, output, counter, source_id);
          }
        }
      } else if (out_bit) {
        edges_added = true;
        // Add outgoing edge only
        for (int out_reg = 0; out_reg <= 1; out_reg++) {
          Graph temp = extended;
          add_edge(&temp, new_node, n, out_reg);
          expand_single_graph(&temp, target_size, output, counter, source_id);
        }
      }
    }

    // Special case: new node with no edges (invalid, skip)
    if (!edges_added)
      continue;
  }
}
