#!/usr/bin/env bash

set -e

SOURCES=$(find . -type f \( -name "*.h" -o -name "*.cpp" \) \
        -not \( -path "./build/*" -o -path "./third_party/*" \))                          
echo $(clang-format -version)
clang-format -i ${SOURCES}
DIFF=$(git diff)

if [ -n "${DIFF}" ]; then
    echo "${DIFF}"
    exit 1
fi
