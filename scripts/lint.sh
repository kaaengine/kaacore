#!/usr/bin/env bash

set -e

if which clang-tidy
then
    CLANG_TIDY=clang-tidy
elif which clang-tidy-7
then
    CLANG_TIDY=clang-tidy-7
else
    echo "clang-tidy not found!"
    exit 1
fi

SCRIPTS_DIR=${BASH_SOURCE%/*}

if [ ! -f "${SCRIPTS_DIR}/../build/compile_commands.json" ]; then
    echo "Build the project with CMAKE_EXPORT_COMPILE_COMMANDS flag on in order to run linter."
    exit 1
fi

${CLANG_TIDY} -p "${SCRIPTS_DIR}/../build/" "${SCRIPTS_DIR}"/../src/*.cpp "$@"
