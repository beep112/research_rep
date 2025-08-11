###############################################################################
# Cross-platform Makefile for HRA tools and graph index generation
# Builds:
#   - hra_runner (main program)
#   - hra_expander (subprocess)
#   - hra_test3 (auxiliary test tool)
# Also runs build_graph_index.py to refresh index.json files.
###############################################################################

# Compiler / Tools
CC      ?= gcc
UNAME_S := $(shell uname -s 2>/dev/null)

# Python command (python on Windows, python3 elsewhere)
ifeq ($(OS),Windows_NT)
	PYTHON ?= python
else
	PYTHON ?= python3
endif

# Base C flags
CFLAGS  := -Wall -Wextra -O3 -std=c99 $(CFLAGS)

# GNU extensions available on Linux; avoid on macOS/Windows for portability
ifeq ($(UNAME_S),Linux)
	CFLAGS += -D_GNU_SOURCE
endif

# Linker flags
LDFLAGS := $(LDFLAGS)
ifeq ($(UNAME_S),Linux)
	LDFLAGS += -lpthread -lm
endif
ifeq ($(UNAME_S),Darwin)
	# pthread + libm are part of system toolchain on macOS; still allow -lm
	LDFLAGS += -lpthread -lm
endif
# On Windows (MSYS2/MinGW), pthread and m are typically available; keep defaults
ifeq ($(OS),Windows_NT)
	# MSYS2/MinGW-w64 typically uses winpthread; -pthread will map appropriately
	CFLAGS  += -pthread
	LDFLAGS += -pthread -lm
endif

# Source files
SAMPLER_SOURCES = hra_sampler.c hra_runner.c
EXPANDER_SOURCES = hra_expander.c hra_sampler.c
TEST3_SOURCES   = hra_test3.c

# Executables
SAMPLER_EXEC = hra_runner
EXPANDER_EXEC = hra_expander
TEST3_EXEC   = hra_test3

# Build only C executables
binaries: $(SAMPLER_EXEC) $(EXPANDER_EXEC) $(TEST3_EXEC)

# Default target builds executables and refreshes indexes
all: binaries index

# Main runner program
$(SAMPLER_EXEC): hra_runner.c hra_sampler.c hra_sampler.h
	$(CC) $(CFLAGS) -o $@ hra_runner.c hra_sampler.c $(LDFLAGS)

# Graph expander subprocess (called by runner)
$(EXPANDER_EXEC): hra_expander.c hra_sampler.c hra_sampler.h
	$(CC) $(CFLAGS) -o $@ hra_expander.c hra_sampler.c $(LDFLAGS)

# Auxiliary test program 3
$(TEST3_EXEC): $(TEST3_SOURCES) hra_test3.h
	$(CC) $(CFLAGS) -o $@ $(TEST3_SOURCES) $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(SAMPLER_EXEC) $(EXPANDER_EXEC) $(TEST3_EXEC)
	rm -f *.o
	@echo "Preserved hra_evolution_results/ (use 'make clean-results' to remove)."

# Remove generated result graphs (use with caution)
clean-results:
	rm -rf hra_evolution_results

# Deep clean: binaries + results
distclean: clean clean-results

# Backup and restore results
backup-results:
	@if [ -d hra_evolution_results ]; then \
		zip -r hra_evolution_results_backup.zip hra_evolution_results >/dev/null; \
		echo "Backed up hra_evolution_results to hra_evolution_results_backup.zip"; \
	else \
		echo "No hra_evolution_results to back up."; \
	fi

restore-results:
	@if [ -f hra_evolution_results_backup.zip ]; then \
		unzip -o hra_evolution_results_backup.zip >/dev/null; \
		echo "Restored hra_evolution_results from backup."; \
	else \
		echo "No backup archive found: hra_evolution_results_backup.zip"; \
	fi

# Run full pipeline (assuming you have n=3 graphs)
run: $(SAMPLER_EXEC) $(EXPANDER_EXEC)
	./$(SAMPLER_EXEC) hras_dot_files/hras_n3.dot 4 0

# Build JSON indexes for GitHub Pages graphs.html viewer
index:
	$(PYTHON) build_graph_index.py

# Debug version
debug: CFLAGS += -g -DDEBUG
debug: all

.PHONY: all clean run debug clean-results distclean backup-results restore-results
