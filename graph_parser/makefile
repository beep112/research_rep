CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -D_GNU_SOURCE
LDFLAGS = -lpthread -lm

# Source files
SAMPLER_SOURCES = hra_sampler.c hra_runner.c
EXPANDER_SOURCES = hra_expander.c hra_sampler.c

# Executables
SAMPLER_EXEC = hra_runner
EXPANDER_EXEC = hra_expander

# Default target
all: $(SAMPLER_EXEC) $(EXPANDER_EXEC)

# Main runner program
$(SAMPLER_EXEC): hra_runner.c hra_sampler.c hra_sampler.h
	$(CC) $(CFLAGS) -o $@ hra_runner.c hra_sampler.c $(LDFLAGS)

# Graph expander subprocess (called by runner)
$(EXPANDER_EXEC): hra_expander.c hra_sampler.c hra_sampler.h
	$(CC) $(CFLAGS) -o $@ hra_expander.c hra_sampler.c $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(SAMPLER_EXEC) $(EXPANDER_EXEC)
	rm -rf hra_evolution_results
	rm -f *.o

# Run full pipeline (assuming you have n=3 graphs)
run: $(SAMPLER_EXEC) $(EXPANDER_EXEC)
	./$(SAMPLER_EXEC) hras_dot_files/hras_n3.dot 4 0

# Build JSON indexes for GitHub Pages graphs.html viewer
index:
	python3 build_graph_index.py

# Debug version
debug: CFLAGS += -g -DDEBUG
debug: all

.PHONY: all clean run debug
