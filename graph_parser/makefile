# Makefile for HRA Generator

CC = gcc
CFLAGS = -O3 -march=native -Wall -Wextra -std=c99
LDFLAGS = -lm
TARGET = hra_test 
SOURCE = hra_test.c
HEADER = hra_test.h

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)

# Optimized build for maximum performance
optimized: CFLAGS += -funroll-loops -ffast-math -DNDEBUG
optimized: $(TARGET)

# Debug build
debug: CFLAGS = -g -O0 -Wall -Wextra -std=c99 -DDEBUG
debug: $(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Profile build (requires gprof)
profile: CFLAGS += -pg
profile: LDFLAGS += -pg
profile: $(TARGET)

.PHONY: all optimized debug clean run profile

# Usage instructions as comments:
#
# To compile and run:
#   make && ./hra_generator
#
# For maximum performance:
#   make optimized && ./hra_generator
#
# For debugging:
#   make debug && gdb ./hra_generator
#
# To profile performance:
#   make profile && ./hra_generator && gprof ./hra_generator gmon.out
#
# Expected performance improvements over Python:
# - 10-50x faster for small n (n=1-4)
# - 50-200x faster for larger n (n=5-6)
# - Much lower memory usage
# - Better cache efficiency
#
# Note: The C implementation focuses on the core algorithm.
# Plotting functionality would require additional libraries like:
# - matplotlib-cpp (C++ wrapper for matplotlib)
# - or export data to file and plot with separate Python script
