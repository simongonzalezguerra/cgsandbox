file(GLOB RTE_SOURCES "*.cpp")
add_executable(rte main.cpp ${RTE_SOURCES})
FIND_LIBRARY(X11_LIBRARY X11)
target_link_libraries(rte libglfw.so ${X11_LIBRARY} GL pthread assimp GLEW_1130 freeimage)

