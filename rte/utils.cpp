#include "utils.hpp"
#include "glm/glm.hpp"

namespace rte
{
    float fov_to_fovy(float fov_radians, float width, float height)
    {
        float d = width / (2.0f * tanf(fov_radians / 2.0f));
        return 2.0f * atan(height / (2.0f * d));
    }

    glm::vec3 camera_position_worldspace_from_view_matrix(const glm::mat4 view_matrix)
    {
        // See this page for explanation:
        // https://gamedev.stackexchange.com/questions/138208/extract-eye-camera-position-from-a-view-matrix
        glm::mat3 rot_mat(view_matrix);
        glm::vec3 d(view_matrix[3]); 
        return -d * rot_mat;
    }
}
