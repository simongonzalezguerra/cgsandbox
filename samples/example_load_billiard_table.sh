#! /bin/bash

LOADER_BIN="../build/samples/load_file"
RESOURCE_PATH="../resources/"
FILE="billiard-table/Table_billiard_N150210.3DS"

${LOADER_BIN} ${RESOURCE_PATH}${FILE}
echo "Logs appended to cgs.log"
