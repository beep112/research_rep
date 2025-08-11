// checking to make sure that this shows in the pull request
#include "hra_sampler.h"

int main(int argc, char *argv[]) {
  if (argc != 6) {
    fprintf(stderr,
            "Usage: %s <input_dot_file> <graph_index> <output_file> "
            "<start_size> <target_size>\n",
            argv[0]);
    return 1;
  }

  const char *input_file = argv[1];
  int graph_index = atoi(argv[2]);
  const char *output_file = argv[3];
  int start_size = atoi(argv[4]);
  int target_size = atoi(argv[5]);

  // Load the graph
  FILE *input_fp = fopen(input_file, "r");
  if (!input_fp) {
    fprintf(stderr, "Error: Cannot open input file %s\n", input_file);
    return 1;
  }

  Graph base_graph;
  int current_index = 0;
  int graph_id = 0;

  // Skip to the correct graph
  while (current_index <= graph_index) {
    if (!parse_single_dot_graph(input_fp, &base_graph, &graph_id)) {
      fprintf(stderr, "Error: Cannot find graph at index %d\n", graph_index);
      fclose(input_fp);
      return 1;
    }
    if (current_index == graph_index) {
      break;
    }
    current_index++;
  }
  fclose(input_fp);

  // Validate base graph
  if (base_graph.n_nodes != start_size) {
    fprintf(stderr, "Error: Base graph has %d nodes, expected %d\n",
            base_graph.n_nodes, start_size);
    return 1;
  }

  // Check weak connectivity for base graph
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
  // Safety bounds checking
  if (base->n_nodes >= MAX_NODES || target_size > MAX_NODES) {
    return;
  }

  // Base case: if we've reached target size
  if (base->n_nodes == target_size) {
    if (is_weakly_connected(base) && is_heritable_topology(base) &&
        is_heritable_regulatory(base)) {

      // Compute canonical representation
      char canonical_rep[MAX_NODES * MAX_NODES * sizeof(int)];
      compute_canonical_representation(base, canonical_rep);

      // Write to output file with metadata
      fprintf(output, "// Source:%d Canonical:", source_id);
      for (int i = 0; i < base->n_nodes * base->n_nodes; i++) {
        fprintf(output, " %d", ((int *)canonical_rep)[i]);
      }
      fprintf(output, "\n");

      // Output digraph
      fprintf(output, "digraph HRA_from_%d_graph_%03d {\n", source_id,
              (*counter));
      for (int e = 0; e < base->n_edges; e++) {
        fprintf(output, "  %d -> %d [label=\"%d\"];\n", base->edges[e].from,
                base->edges[e].to, base->edges[e].regulation);
      }
      fprintf(output, "}\n\n");

      (*counter)++;
    }
    return;
  }

  // Only proceed if we need exactly one more node
  if (base->n_nodes + 1 != target_size) {
    return;
  }

  int new_node = base->n_nodes;
  int existing_nodes = base->n_nodes;

  // Prevent unreasonable expansion
  if (existing_nodes > 6)
    return;

  // Generate ONE expanded graph with ALL possible connections
  // Then check if it meets criteria - much simpler approach

  // For n=4 to n=5: new node is 4, existing nodes are 0,1,2,3
  // Try a more systematic approach with reasonable limits

  // Cap the number of existing nodes we'll process
  if (existing_nodes > 5)
    return;

  // Create expanded graph
  Graph extended = *base;
  extended.n_nodes = new_node + 1;

  // Systematically build ONE version by copying the base structure correctly
  // Reset edges array and rebuild properly
  int original_edges = extended.n_edges;

  // Actually, let's take a step back and use a different approach

  // Method: Generate one specific expansion pattern at a time
  // Instead of recursive branching, generate each complete pattern and test it

  // For 4 existing nodes, generate all 4^4 = 256 connection patterns
  long max_patterns = 1;
  for (int i = 0; i < existing_nodes; i++) {
    max_patterns *= 4;
  }

  // Safety cap
  if (max_patterns > 1000)
    max_patterns = 1000;

  for (long pattern = 0; pattern < max_patterns; pattern++) {
    // Create fresh graph for this pattern
    Graph test_graph;
    init_graph(&test_graph, new_node + 1);

    // Copy all original edges
    for (int i = 0; i < base->n_edges; i++) {
      add_edge(&test_graph, base->edges[i].from, base->edges[i].to,
               base->edges[i].regulation);
    }

    // Apply this specific connection pattern
    long temp_pattern = pattern;
    bool has_connections = false;

    // Build ALL connections for this pattern first, then test once
    for (int existing_node = 0; existing_node < existing_nodes;
         existing_node++) {
      int connection_type = temp_pattern % 4;
      temp_pattern /= 4;

      switch (connection_type) {
      case 0: // No connection
        break;
      case 1: // existing -> new with regulation 0
        add_edge(&test_graph, existing_node, new_node, 0);
        has_connections = true;
        break;
      case 2: // new -> existing with regulation 0
        add_edge(&test_graph, new_node, existing_node, 0);
        has_connections = true;
        break;
      case 3: // Bidirectional with regulation (0,0) - simplest version
        add_edge(&test_graph, existing_node, new_node, 0);
        add_edge(&test_graph, new_node, existing_node, 0);
        has_connections = true;
        break;
      }
    }

    // Test this single completed graph
    if (test_graph.n_nodes == target_size) {
      if (is_weakly_connected(&test_graph) &&
          is_heritable_topology(&test_graph) &&
          is_heritable_regulatory(&test_graph)) {

        // Write successful graph
        char canonical_rep[MAX_NODES * MAX_NODES * sizeof(int)];
        compute_canonical_representation(&test_graph, canonical_rep);

        fprintf(output, "// Source:%d Canonical:", source_id);
        for (int i = 0; i < test_graph.n_nodes * test_graph.n_nodes; i++) {
          fprintf(output, " %d", ((int *)canonical_rep)[i]);
        }
        fprintf(output, "\n");

        fprintf(output, "digraph HRA_from_%d_graph_%03d {\n", source_id,
                (*counter));
        for (int e = 0; e < test_graph.n_edges; e++) {
          fprintf(output, "  %d -> %d [label=\"%d\"];\n",
                  test_graph.edges[e].from, test_graph.edges[e].to,
                  test_graph.edges[e].regulation);
        }
        fprintf(output, "}\n\n");

        (*counter)++;
      }
    }
  }
}
