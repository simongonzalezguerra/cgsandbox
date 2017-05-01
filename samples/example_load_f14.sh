#! /bin/bash

LOADER_BIN="../build/samples/load_file"
RESOURCE_PATH="../resources/"
FILE="f-14-tomcat-broken/f14d.obj"

${LOADER_BIN} ${RESOURCE_PATH}${FILE}
echo "Logs appended to cgs.log"
