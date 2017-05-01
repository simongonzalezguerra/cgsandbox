#! /bin/bash

LOADER_BIN="../build/samples/simple_view"
RESOURCE_PATH="../resources/"
MODEL_FILE="suzanne/suzanne.obj"
TEXTURE_FILE="suzanne/uvmap.DDS"

${LOADER_BIN} ${RESOURCE_PATH}${MODEL_FILE} ${RESOURCE_PATH}${TEXTURE_FILE}
echo "Logs appended to cgs.log
