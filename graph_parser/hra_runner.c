
#include "hra_sampler.h"

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

extern void *worker_thread(void *arg);
extern int count_graphs_in_dot_file(const char *filename);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <dot_file> [num_threads] [verbose] [start_size] "
           "[target_size]\n",
           argv[0]);
    return 1;
  }

  const char *dot_file = argv[1];
  int num_threads = (argc > 2) ? atoi(argv[2]) : 4;
  bool verbose = (argc > 3) ? atoi(argv[3]) : 0;
  int start_node_count = (argc > 4) ? atoi(argv[4]) : 3;
  int target_node_count = (argc > 5) ? atoi(argv[5]) : 4;

  if (num_threads <= 0 || num_threads > MAX_THREADS) {
    num_threads = 4;
  }

  printf("HRA Evolutionary Sampler\n========================\n");
  printf("Input file: %s\nThreads: %d\nVerbose: %s\n", dot_file, num_threads,
         verbose ? "Yes" : "No");
  printf("Expanding from n=%d to n=%d\n\n", start_node_count,
         target_node_count);

  int total_graphs = count_graphs_in_dot_file(dot_file);
  if (total_graphs <= 0) {
    fprintf(stderr, "Error: No graphs found in %s\n", dot_file);
    return 1;
  }
  printf("Found %d graphs in input file\n\n", total_graphs);

  const char *output_dir = "hra_evolution_results";
  if (mkdir(output_dir, 0755) != 0 && errno != EEXIST) {
    perror("Failed to create output directory");
    return 1;
  }

  pthread_t threads[MAX_THREADS];
  WorkerThread args[MAX_THREADS];
  int per_thread = (total_graphs + num_threads - 1) / num_threads;
  clock_t start = clock();

  // Create worker threads
  for (int i = 0; i < num_threads; i++) {
    args[i].thread_id = i;
    strncpy(args[i].input_dot_file, dot_file, MAX_FILENAME - 1);
    args[i].input_dot_file[MAX_FILENAME - 1] = '\0';
    args[i].start_graph_index = i * per_thread;
    args[i].end_graph_index = ((i + 1) * per_thread > total_graphs)
                                  ? total_graphs
                                  : (i + 1) * per_thread;
    strncpy(args[i].output_dir, output_dir, MAX_FILENAME - 1);
    args[i].output_dir[MAX_FILENAME - 1] = '\0';
    args[i].start_node_count = start_node_count;
    args[i].target_node_count = target_node_count;
    args[i].print_mutex = &print_mutex;
    args[i].total_graphs = total_graphs;

    if (pthread_create(&threads[i], NULL, worker_thread, &args[i]) != 0) {
      perror("Failed to create thread");
      return 1;
    }
  }

  // Wait for all threads to complete
  for (int i = 0; i < num_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("Failed to join thread");
    }
  }

  double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
  printf("\nAll threads completed in %.2f seconds\n", elapsed);
  printf("Analyzing results...\n");

  // Pass the correct original and target sizes
  analyze_results(output_dir, start_node_count, target_node_count, dot_file,
                  verbose);

  pthread_mutex_destroy(&print_mutex);
  return 0;
}
// Worker thread function
void *worker_thread(void *arg) {
  WorkerThread *worker = (WorkerThread *)arg;

  pthread_mutex_lock(worker->print_mutex);
  printf("Thread %d: Processing graphs %d to %d\n", worker->thread_id,
         worker->start_graph_index, worker->end_graph_index - 1);
  pthread_mutex_unlock(worker->print_mutex);

  int graphs_processed = 0;

  for (int graph_idx = worker->start_graph_index;
       graph_idx < worker->end_graph_index; graph_idx++) {

    char thread_output_file[MAX_FILENAME];
    snprintf(thread_output_file, MAX_FILENAME, "%s/thread_%d_graph_%d.dot",
             worker->output_dir, worker->thread_id, graph_idx);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./hra_expander \"%s\" %d \"%s\" %d %d",
             worker->input_dot_file, graph_idx, thread_output_file,
             worker->start_node_count, worker->target_node_count);

    int result = system(cmd);
    if (result == 0) {
      graphs_processed++;
    } else {
      pthread_mutex_lock(worker->print_mutex);
      fprintf(stderr,
              "Thread %d: Warning - subprocess failed for graph %d (cmd: %s)\n",
              worker->thread_id, graph_idx, cmd);
      pthread_mutex_unlock(worker->print_mutex);
    }
  }

  pthread_mutex_lock(worker->print_mutex);
  printf("Thread %d completed: processed %d graphs\n", worker->thread_id,
         graphs_processed);
  pthread_mutex_unlock(worker->print_mutex);

  return NULL;
}
