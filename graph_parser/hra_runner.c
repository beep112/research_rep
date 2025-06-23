#include "hra_sampler.h"

// Global mutex for thread-safe printing
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <dot_file> [num_threads] [verbose]\n");
    printf("  dot_file: Path to the .dot file containing n=3 HRAs\n");
    printf("  num_threads: Number of worker threads (default: 4)\n");
    printf("  verbose: Enable verbose output (any third argument)\n");
    return 1;
  }

  const char *dot_file = argv[1];
  int num_threads = (argc > 2) ? atoi(argv[2]) : 4;
  bool verbose = (argc > 3);

  if (num_threads <= 0 || num_threads > MAX_THREADS) {
    num_threads = 4;
  }

  printf("HRA Evolutionary Sampler\n");
  printf("========================\n");
  printf("Input file: %s\n", dot_file);
  printf("Threads: %d\n", num_threads);
  printf("Verbose: %s\n", verbose ? "Yes" : "No");
  printf("\n");

  // Check if input file exists
  FILE *test_file = fopen(dot_file, "r");
  if (!test_file) {
    printf("Error: Cannot open input file %s\n", dot_file);
    return 1;
  }
  fclose(test_file);

  // Count total graphs in the file
  printf("Analyzing input file...\n");
  int total_graphs = count_graphs_in_dot_file(dot_file);
  if (total_graphs <= 0) {
    printf("Error: No valid graphs found in %s\n", dot_file);
    return 1;
  }
  printf("Found %d graphs in input file\n", total_graphs);

  // Create output directory
  const char *output_dir = "hra_evolution_results";
  if (mkdir(output_dir, 0755) != 0 && errno != EEXIST) {
    printf("Error: Cannot create output directory %s\n", output_dir);
    return 1;
  }

  // Initialize random seed
  srand((unsigned int)time(NULL));

  // Determine how many graphs each thread should process
  int graphs_per_thread = (total_graphs + num_threads - 1) / num_threads;

  printf("\nStarting parallel evolution analysis...\n");
  printf("Each thread will process ~%d graphs\n", graphs_per_thread);
  printf("\n");

  // Create worker threads
  pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
  WorkerThread *worker_args = malloc(num_threads * sizeof(WorkerThread));

  if (!threads || !worker_args) {
    printf("Error: Memory allocation failed\n");
    return 1;
  }

  clock_t start_time = clock();

  // Launch worker threads - each processes a subset of graphs
  for (int i = 0; i < num_threads; i++) {
    worker_args[i].thread_id = i;
    worker_args[i].input_graph = NULL;
    snprintf(worker_args[i].output_dir, MAX_FILENAME, "%s", output_dir);
    worker_args[i].start_node_count = 3;
    worker_args[i].target_node_count = 4;
    worker_args[i].print_mutex = &print_mutex;

    // Calculate which graphs this thread should process
    int start_graph = i * graphs_per_thread;
    int end_graph = (i + 1) * graphs_per_thread;
    if (end_graph > total_graphs)
      end_graph = total_graphs;

    // Store the dot file and graph range for this thread
    strncpy(worker_args[i].input_dot_file, dot_file, MAX_FILENAME - 1);
    worker_args[i].start_graph_index = start_graph;
    worker_args[i].end_graph_index = end_graph;
    worker_args[i].total_graphs = total_graphs;

    if (pthread_create(&threads[i], NULL, worker_thread, &worker_args[i]) !=
        0) {
      printf("Error: Failed to create thread %d\n", i);
      return 1;
    }
  }

  // Wait for all threads to complete
  printf("Waiting for threads to complete...\n");
  for (int i = 0; i < num_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      printf("Error: Failed to join thread %d\n", i);
    }
  }

  clock_t end_time = clock();
  double elapsed = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

  printf("\nAll threads completed in %.2f seconds\n", elapsed);
  printf("Analyzing results...\n");

  // Analyze the results
  int analysis_result = analyze_results(output_dir, 3, 4, dot_file, verbose);

  if (analysis_result == 0) {
    printf("\nAnalysis completed successfully!\n");
  } else {
    printf("\nAnalysis completed with warnings.\n");
  }

  // Cleanup
  free(threads);
  free(worker_args);
  pthread_mutex_destroy(&print_mutex);

  return 0;
}

// Thread worker function - processes a range of graphs from the dot file
void *worker_thread(void *arg) {
  WorkerThread *worker = (WorkerThread *)arg;

  pthread_mutex_lock(worker->print_mutex);
  printf("Thread %d: Processing graphs %d to %d\n", worker->thread_id,
         worker->start_graph_index, worker->end_graph_index - 1);
  pthread_mutex_unlock(worker->print_mutex);

  int graphs_processed = 0;

  // Process each graph in the assigned range
  for (int graph_idx = worker->start_graph_index;
       graph_idx < worker->end_graph_index; graph_idx++) {
    // Create output file for this graph's expansions
    char thread_output_file[MAX_FILENAME];
    snprintf(thread_output_file, MAX_FILENAME, "%s/thread_%d_graph_%d.dot",
             worker->output_dir, worker->thread_id, graph_idx);

    // Call the expander subprocess
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./hra_expander \"%s\" %d \"%s\" %d %d",
             worker->input_dot_file, graph_idx, thread_output_file,
             worker->start_node_count, worker->target_node_count);

    int result = system(cmd);
    if (result == 0) {
      graphs_processed++;
    } else {
      pthread_mutex_lock(worker->print_mutex);
      printf("Thread %d: Warning - subprocess failed for graph %d\n",
             worker->thread_id, graph_idx);
      pthread_mutex_unlock(worker->print_mutex);
    }
  }

  pthread_mutex_lock(worker->print_mutex);
  printf("Thread %d completed: processed %d graphs\n", worker->thread_id,
         graphs_processed);
  pthread_mutex_unlock(worker->print_mutex);

  return NULL;
}
