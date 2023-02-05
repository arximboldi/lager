#!/usr/bin/env bash

# Short form: set -e
set -o errexit

# Print a helpful message if a pipeline with non-zero exit code causes the
# script to exit as described above.
trap 'echo "Aborting due to errexit on line $LINENO. Exit code: $?" >&2' ERR

# Allow the above trap be inherited by all functions in the script.
#
# Short form: set -E
set -o errtrace

# Return value of a pipeline is the value of the last (rightmost) command to
# exit with a non-zero status, or zero if all commands in the pipeline exit
# successfully.
set -o pipefail

# Set $IFS to only newline and tab.
#
# http://www.dwheeler.com/essays/filenames-in-shell.html
IFS=$'\n\t'

MEASUREMENTS=200
BENCHMARK_RUNNER=$1
GIT_ROOT_DIR=$(git rev-parse --show-toplevel)

function _run_benchmark() {
  local N=${1}
  local FILTER_ARG=${2}

  OUTPUT_FILE="${GIT_ROOT_DIR}/reports/N:${N}_s${MEASUREMENTS}.out"
  HTML_FILE="N:${N}_s${MEASUREMENTS}.html"

  ${GIT_ROOT_DIR}/tools/with-tee.bash \
    ${OUTPUT_FILE} \
    ${BENCHMARK_RUNNER} -v \
    -t "Traversal_${N}" \
    -r html \
    -s "${MEASUREMENTS}" \
    -p "N:${N}" \
    -o ${HTML_FILE} \
    "${FILTER_ARG}"
}

for N in 5 7 9 11 13 15
do
  _run_benchmark ${N} 
done

for N in 1024 2048 4096 16384 32768
do
  _run_benchmark ${N} "-f ^(?!DFS-DC).*"
done
