add_executable(layer_tests layer_tests.cpp)
target_link_libraries(layer_tests glsandbox libgtest.a pthread)

add_executable(material_tests material_tests.cpp)
target_link_libraries(material_tests glsandbox libgtest.a pthread)

add_executable(mesh_tests mesh_tests.cpp)
target_link_libraries(mesh_tests glsandbox libgtest.a pthread)

add_executable(resource_tests resource_tests.cpp)
target_link_libraries(resource_tests glsandbox libgtest.a pthread)

add_executable(log_tests log_tests.cpp)
target_link_libraries(log_tests glsandbox libgtest.a pthread)

add_executable(node_tests node_tests.cpp)
target_link_libraries(node_tests glsandbox libgtest.a pthread)

add_executable(resource_loader_tests resource_loader_tests.cpp)
target_link_libraries(resource_loader_tests glsandbox libgtest.a pthread assimp)
