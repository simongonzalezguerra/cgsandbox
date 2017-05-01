file(GLOB GLSANDBOX_SOURCES "*.cpp")
add_library(glsandbox ${GLSANDBOX_SOURCES})

FIND_LIBRARY(X11_LIBRARY X11)
target_link_libraries(glsandbox libglfw.so ${X11_LIBRARY} GL)
