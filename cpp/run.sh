#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

if [ ! -d "$BUILD_DIR" ]; then
  echo "Build directory '$BUILD_DIR' does not exist. Run 'make all' from cpp first."
  exit 1
fi

mapfile -t executables < <(find "$BUILD_DIR" -maxdepth 1 -type f -name 'day*' -executable | sort)

if [ ${#executables[@]} -eq 0 ]; then
  echo "No day executables found in '$BUILD_DIR'. Run 'make all' from cpp first."
  exit 1
fi

for exe in "${executables[@]}"; do
  name="$(basename "$exe")"
  echo "===== $name ====="
  "$exe"
  echo
done
