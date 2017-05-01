#! /bin/bash

LOADER_BIN="../build/samples/load_file"
RESOURCE_PATH="../resources/"
FILE="stanford-buddha/happy_vrip_res4.ply"

${LOADER_BIN} ${RESOURCE_PATH}${FILE}
echo "Logs appended to cgs.log"
