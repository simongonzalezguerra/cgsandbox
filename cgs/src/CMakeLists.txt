file(GLOB GLSANDBOX_SOURCES "*.cpp")
add_library(cgs ${GLSANDBOX_SOURCES})

FIND_LIBRARY(X11_LIBRARY X11)
target_link_libraries(cgs libglfw.so ${X11_LIBRARY} GL)
