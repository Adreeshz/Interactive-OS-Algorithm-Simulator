#!/usr/bin/env bash

# Lightweight launcher: execute the engine binary directly.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE="$SCRIPT_DIR/sys_rescue_engine"

if [ ! -x "$ENGINE" ]; then
    echo "Engine binary not found or not executable: $ENGINE"
    echo "Please build the project (run 'make' in the project root)"
    exit 1
fi

exec "$ENGINE" "$@"
