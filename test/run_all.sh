#! /bin/bash

cd ../build/test/unit/cgs/
./layer_tests &&\
./log_tests &&\
./material_tests &&\
./mesh_tests &&\
./node_tests &&\
./resource_loader_tests &&\
./resource_tests
cd -
