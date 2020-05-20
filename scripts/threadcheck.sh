#!/bin/bash

set -e

if [[ -z $1 || $1 == "-h" || $1 == "--help" ]]
then
    echo "Usage: $0 [extra valgrind flags] <binary> [binary-params]"
    exit 1
fi

[[ -z ${VALGRIND_BIN} ]] && VALGRIND_BIN="valgrind"

${VALGRIND_BIN} --version

SCRIPTS_DIR=`dirname $0`
VALGRIND_DIR="${SCRIPTS_DIR}/valgrind"

${VALGRIND_BIN} --tool=helgrind \
    --suppressions="${VALGRIND_DIR}/threading.supp" \
    $@
