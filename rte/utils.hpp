#ifndef GLSANDBOX_UTILS_HPP
#define GLSANDBOX_UTILS_HPP

#include "glm/glm.hpp"
#include "log.hpp"

#include <stdexcept>
#include <sstream>

namespace rte
{
    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to call an arbitrary function without arguments.
    //-----------------------------------------------------------------------------------------------
    typedef void (*process_func)();

    // Retuns fovy in radians. This is how glm::perspective expects it
    float fov_to_fovy(float fov_radians, float window_width, float window_height);
    // Extracts the camera position in worldspace from the view matrix
    // Constraint: this works if the view matrix doesn't involve scaling (tipical situation)
    // For the more general case where the view matrix can include scaling see this page:
    // https://www.opengl.org/discussion_boards/showthread.php/178484-Extracting-camera-position-from-a-ModelView-Matrix
    glm::vec3 camera_position_worldspace_from_view_matrix(const glm::mat4 view_matrix);

    glm::vec4 direction_to_homogenous_coords(const glm::vec3&);

    glm::vec4 position_to_homogenous_coords(const glm::vec3&);

    glm::vec3 from_homogenous_coords(const glm::vec4&);

    template<typename T>
    void print_sequence(T* a, std::size_t num_elems, std::ostringstream& oss)
    {
        oss << "[";
        if (num_elems) {
            oss << " " << a[0];
        }
        for (std::size_t i = 1U; i < num_elems; i++) {
            oss << ", " << a[i];
        }
        oss << " ]";
    }
}

#endif
