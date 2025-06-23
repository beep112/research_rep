CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -D_GNU_SOURCE
LDFLAGS = -lpthread -lm

# Source files
ORIGINAL_SOURCES = hra_test3.c
SAMPLER_SOURCES = hra_sampler.c hra_runner.c
HEADERS = hra_test3.h hra_sampler.h

# Executables
ORIGINAL_EXEC = hra_test3
SAMPLER_EXEC = hra_runner
EXPANDER_EXEC = hra_expander

# Default target
all: $(ORIGINAL_EXEC) $(SAMPLER_EXEC) $(EXPANDER_EXEC)

# Original HRA generator (to create initial .dot files)
$(ORIGINAL_EXEC): $(ORIGINAL_SOURCES) hra_test3.h
	$(CC) $(CFLAGS) -o $@ $(ORIGINAL_SOURCES) $(LDFLAGS)

# Main runner program
$(SAMPLER_EXEC): hra_runner.c hra_sampler.c hra_sampler.h
	$(CC) $(CFLAGS) -o $@ hra_runner.c hra_sampler.c $(LDFLAGS)

# Graph expander subprocess (called by runner)
$(EXPANDER_EXEC): hra_expander.c hra_sampler.c hra_sampler.h
	$(CC) $(CFLAGS) -o $@ hra_expander.c hra_sampler.c $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(ORIGINAL_EXEC) $(SAMPLER_EXEC) $(EXPANDER_EXEC)
	rm -rf hras_dot_files hra_evolution_results
	rm -f *.o

# Create test data
test-data: $(ORIGINAL_EXEC)
	./$(ORIGINAL_EXEC) 3

# Run full pipeline
run-pipeline: $(ORIGINAL_EXEC) $(SAMPLER_EXEC) $(EXPANDER_EXEC) test-data
	./$(SAMPLER_EXEC) hras_dot_files/hras_n3.dot 4

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install build-essential

# Debug version
debug: CFLAGS += -g -DDEBUG
debug: all

.PHONY: all clean test-data run-pipeline install-deps debug
