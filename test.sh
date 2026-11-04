#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
"${HOST_CXX:-c++}" -std=c++17 -Wall -Wextra -Werror tests/core_test.cpp -o build/core_test
./build/core_test
