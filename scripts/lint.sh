#!/usr/bin/env bash

set -e

if which clang-format
then
    CLANG_FORMAT=clang-format
elif which clang-format-7
then
    CLANG_FORMAT=clang-format-7
else
    echo "clang-format not found!"
    exit 1
fi

SOURCES=$(find . -type f \( -name "*.h" -o -name "*.cpp" \) \
        -not \( -path "./build/*" -o -path "./third_party/*" \))                          
${CLANG_FORMAT} -version
${CLANG_FORMAT} -i ${SOURCES}
DIFF=$(git diff)

if [ -n "${DIFF}" ]; then
    echo "${DIFF}"
    exit 1
fi
