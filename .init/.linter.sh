#!/usr/bin/env bash
# Lightweight static analysis for C/C++ files in this container workspace.
# Designed to be tolerant (skip if clang/clang++ is not available) and never fail CI.

set -euo pipefail

# Resolve repository root based on this script's location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

SRC_DIR="${REPO_ROOT}/app-gateway"

# Prefer clang++ for C++ files; fall back to clang for C files
if ! command -v clang++ >/dev/null 2>&1 && ! command -v clang >/dev/null 2>&1; then
  echo "clang/clang++ not found, skipping static analysis."
  exit 0
fi

# Include paths for Thunder sources (sibling repository)
THUNDER_INC_BASE="${REPO_ROOT}/Thunder/Source"

# Find and analyze files; never fail CI due to analyzer findings
find "${SRC_DIR}" -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) -print0 \
| while IFS= read -r -d '' file; do
  echo "Analyzing ${file}"
  ext="${file##*.}"
  if [[ "${ext}" == "c" ]]; then
    # C source
    clang --analyze -I "${THUNDER_INC_BASE}" "${file}" || true
  else
    # C++ source
    clang++ --analyze -std=c++17 -I "${THUNDER_INC_BASE}" -I "${THUNDER_INC_BASE}/core" -I "${THUNDER_INC_BASE}/plugins" "${file}" || true
  fi
done

echo "Static analysis completed."
