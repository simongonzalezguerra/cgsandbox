add_executable(model_viewer model_viewer.cpp controller.cpp)
FIND_LIBRARY(X11_LIBRARY X11)
target_link_libraries(model_viewer cgs libglfw.so ${X11_LIBRARY} GL pthread assimp GLEW_1130 freeimage)
