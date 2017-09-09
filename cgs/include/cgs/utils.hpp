#ifndef GLSANDBOX_UTILS_HPP
#define GLSANDBOX_UTILS_HPP

namespace cgs
{
    // Retuns fovy in radians. This is how glm::perspective expects it
    float fov_to_fovy(float fov_radians, float window_width, float window_height);
}

#endif
