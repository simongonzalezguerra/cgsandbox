#! /bin/bash

LOADER_BIN="../build/samples/load_file"
RESOURCE_PATH="../resources/"
FILE="f-14D-super-tomcat/F-14D_SuperTomcat.obj"

${LOADER_BIN} ${RESOURCE_PATH}${FILE}
echo "Logs appended to glsandbox.log"
