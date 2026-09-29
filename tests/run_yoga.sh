#!/bin/sh
set -eu

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work=${TMPDIR:-/tmp}/tinylayout-yoga-3.2.1
source=$work/source
build=$work/build

if [ "${1:-}" = --fetch-yoga ]; then
    mkdir -p "$work"
    if [ ! -d "$source/.git" ]; then
        git clone --depth 1 --branch v3.2.1 https://github.com/react/yoga.git "$source"
    fi
elif [ "${1:-}" != "" ]; then
    source=$1
elif [ ! -d "$source/yoga" ]; then
    echo "Pass a Yoga v3.2.1 source directory, or --fetch-yoga to download it." >&2
    exit 2
fi

mkdir -p "$work"
cmake -S "$source/yoga" -B "$build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF
cmake --build "$build" -j 4
cc -std=c99 -Wall -Wextra -Werror -pedantic -I"$repo" -c "$repo/tinylayout.c" -o "$work/tinylayout.o"
c++ -std=c++20 -Wall -Wextra -Werror -I"$repo" -I"$source" "$repo/tests/yoga_compare.cpp" "$work/tinylayout.o" "$build/libyogacore.a" -o "$work/yoga_compare"
"$work/yoga_compare"
