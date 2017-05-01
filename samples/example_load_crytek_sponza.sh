#! /bin/bash

LOADER_BIN="../build/samples/load_file"
RESOURCE_PATH="../resources/"
FILE="crytek-sponza/crytek-sponza.dae"

${LOADER_BIN} ${RESOURCE_PATH}${FILE}
echo "Logs appended to glsandbox.log"
