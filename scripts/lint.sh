#!/usr/bin/env bash

set -e

if which clang-tidy
then
    CLANG_FORMAT=clang-tidy
elif which clang-tidy-7
then
    CLANG_FORMAT=clang-tidy-7
else
    echo "clang-tidy not found!"
    exit 1
fi

SCRIPTS_DIR=${BASH_SOURCE%/*}

if [ ! -f "${SCRIPTS_DIR}/../build/compile_commands.json" ]; then
    echo "Build the project in order to run linter."
    exit 1
fi

clang-tidy -p "${SCRIPTS_DIR}/../build/" "${SCRIPTS_DIR}"/../src/*.cpp "$@"
