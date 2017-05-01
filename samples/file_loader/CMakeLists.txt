add_executable(file_loader file_loader.cpp)
FIND_LIBRARY(X11_LIBRARY X11)
target_link_libraries(file_loader glsandbox assimp)
