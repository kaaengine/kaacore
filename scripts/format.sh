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

SCRIPTS_DIR=${BASH_SOURCE%/*}
SOURCES=$(find "${SCRIPTS_DIR}/.." -type f \( -name "*.h" -o -name "*.cpp" \) \
        -not \( -path "${SCRIPTS_DIR}/../build/*" -o -path "${SCRIPTS_DIR}/../third_party/*" \))   
${CLANG_FORMAT} -version
${CLANG_FORMAT} -i ${SOURCES}
DIFF=$(git diff)

if [ -n "${DIFF}" ]; then
    echo "${DIFF}"
    exit 1
fi
