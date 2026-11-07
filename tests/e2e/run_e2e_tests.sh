#!/usr/bin/env bash
# ==============================================================================
# MEMElang E2E Test Suite Shell Wrapper
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Ensure meme binary exists
if [ ! -f "${ROOT_DIR}/meme" ]; then
    echo "Compiling MEMElang binary..."
    make -C "${ROOT_DIR}"
fi

# Run python test runner
python3 "${SCRIPT_DIR}/run_e2e_tests.py" "$@"
