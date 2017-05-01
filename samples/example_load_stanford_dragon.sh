#! /bin/bash

LOADER_BIN="../build/samples/load_file"
RESOURCE_PATH="../resources/"
FILE="stanford-dragon/dragon_vrip_res3.ply"

${LOADER_BIN} ${RESOURCE_PATH}${FILE}
echo "Logs appended to cgs.log"
