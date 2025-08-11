# Release process

1. Ensure CI is green on main.
2. Update version/tag (use semantic versioning), e.g., v0.1.0.
3. Create a tag and push:
   - git tag v0.1.0
   - git push origin v0.1.0
4. The Release workflow will:
   - Build binaries on Linux/macOS/Windows
   - Package per-OS zips
   - Create a GitHub Release attaching the artifacts
5. Alternatively, run the Release workflow manually from the Actions tab (workflow_dispatch).

Artifacts include:
- hra_runner, hra_expander, hra_test3
- hras_dot_files/index.json

Note: The Windows build uses MSYS2/MinGW toolchain provided by actions.
