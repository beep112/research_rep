#include "hra_sampler.h"

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

extern void *worker_thread(void *arg);
extern int count_graphs_in_dot_file(const char *filename);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <dot_file> [num_threads] [verbose]\n", argv[0]);
    return 1;
  }

  const char *dot_file = argv[1];
  int num_threads = (argc > 2) ? atoi(argv[2]) : 4;
  bool verbose = (argc > 3);

  printf("HRA Evolutionary Sampler\n========================\n");
  printf("Input file: %s\nThreads: %d\nVerbose: %s\n\n", dot_file, num_threads,
         verbose ? "Yes" : "No");

  int total_graphs = count_graphs_in_dot_file(dot_file);
  if (total_graphs <= 0)
    return 1;
  printf("Found %d graphs in input file\n\n", total_graphs);

  const char *output_dir = "hra_evolution_results";
  mkdir(output_dir, 0755);

  pthread_t threads[num_threads];
  WorkerThread args[num_threads];
  int per = (total_graphs + num_threads - 1) / num_threads;
  clock_t start = clock();

  for (int i = 0; i < num_threads; i++) {
    args[i].thread_id = i;
    strncpy(args[i].input_dot_file, dot_file, MAX_FILENAME - 1);
    args[i].start_graph_index = i * per;
    args[i].end_graph_index =
        ((i + 1) * per > total_graphs) ? total_graphs : (i + 1) * per;
    strncpy(args[i].output_dir, output_dir, MAX_FILENAME - 1);
    args[i].start_node_count = 3;
    args[i].target_node_count = 4;
    args[i].print_mutex = &print_mutex;
    args[i].total_graphs = total_graphs;
    pthread_create(&threads[i], NULL, worker_thread, &args[i]);
  }

  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  double elapsed = (clock() - start) / (double)CLOCKS_PER_SEC;
  printf("\nAll threads completed in %.2f seconds\n", elapsed);
  printf("Analyzing results...\n");
  analyze_results(output_dir, 3, 4, dot_file, verbose);

  pthread_mutex_destroy(&print_mutex);
  return 0;
}

// Worker thread function
void *worker_thread(void *arg) {
  WorkerThread *worker = (WorkerThread *)arg;

  pthread_mutex_lock(worker->print_mutex);
  printf("Thread %d: Processing graphs %d to %d", worker->thread_id,
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
      printf("Thread %d: Warning - subprocess failed for graph %d",
             worker->thread_id, graph_idx);
      pthread_mutex_unlock(worker->print_mutex);
    }
  }

  pthread_mutex_lock(worker->print_mutex);
  printf("Thread %d completed: processed %d graphs", worker->thread_id,
         graphs_processed);
  pthread_mutex_unlock(worker->print_mutex);

  return NULL;
}
