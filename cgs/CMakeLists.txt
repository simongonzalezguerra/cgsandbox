add_subdirectory (src)
add_executable(rte main.cpp)
FIND_LIBRARY(X11_LIBRARY X11)
target_link_libraries(rte cgs libglfw.so ${X11_LIBRARY} GL pthread assimp GLEW_1130 freeimage)

