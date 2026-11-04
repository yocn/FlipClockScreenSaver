#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build dist
CXX="${CXX:-x86_64-w64-mingw32-g++}"
WINDRES="${WINDRES:-x86_64-w64-mingw32-windres}"
"$WINDRES" -I src src/app.rc -O coff -o build/resource.o
"$CXX" -std=c++17 -O2 -Wall -Wextra -Werror -municode -mwindows -static \
    src/main.cpp build/resource.o -o dist/ClassicFlipClock.scr \
    -lgdiplus -lgdi32 -luser32 -ladvapi32 -Wl,--dynamicbase,--nxcompat -s
echo "Built dist/ClassicFlipClock.scr"
