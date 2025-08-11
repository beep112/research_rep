# Top-level Makefile to consolidate builds, linting, testing, and packaging

.PHONY: all build clean lint format test index help

all: build

# Build native tools and generate indexes
build:
	$(MAKE) -C graph_parser all

clean:
	$(MAKE) -C graph_parser clean

# Python linters/formatters (install via requirements-dev.txt)
lint:
	ruff check .
	black --check .

format:
	black .
	ruff check --fix .

# Optional tests: runs only if tests/ exists
test:
	@if [ -d tests ] || ls -1 test_*.py 2>/dev/null | grep -q . ; then \
		pytest -q ; \
	else \
		echo "No tests found. Skipping." ; \
	fi

# Refresh index.json files for graph viewer
index:
	$(MAKE) -C graph_parser index

help:
	@echo "Targets:"
	@echo "  build   - Build C tools and generate graph indexes"
	@echo "  clean   - Clean build artifacts"
	@echo "  lint    - Run Ruff and Black checks"
	@echo "  format  - Auto-format with Black and Ruff"
	@echo "  test    - Run tests if present (pytest)"
	@echo "  index   - Regenerate graph index JSON files"
