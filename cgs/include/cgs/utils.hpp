#ifndef GLSANDBOX_UTILS_HPP
#define GLSANDBOX_UTILS_HPP

#include "glm/glm.hpp"

namespace cgs
{
    // Retuns fovy in radians. This is how glm::perspective expects it
    float fov_to_fovy(float fov_radians, float window_width, float window_height);
    // Extracts the camera position in worldspace from the view matrix
    // Constraint: this works if the view matrix doesn't involve scaling (tipical situation)
    // For the more general case where the view matrix can include scaling see this page:
    // https://www.opengl.org/discussion_boards/showthread.php/178484-Extracting-camera-position-from-a-ModelView-Matrix
    glm::vec3 camera_position_worldspace_from_view_matrix(const glm::mat4 view_matrix);
}

#endif
