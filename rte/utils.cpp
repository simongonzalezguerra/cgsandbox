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

    glm::vec4 direction_to_homogenous_coords(const glm::vec3& v)
    {
        return glm::vec4(v, 0.0f);
    }

    glm::vec4 position_to_homogenous_coords(const glm::vec3& v)
    {
        return glm::vec4(v, 1.0f);
    }

    glm::vec3 from_homogenous_coords(const glm::vec4& v)
    {
        glm::vec4 cartesian = v / v.w;         // divide by w to convert back from homogenous coordinates
        return glm::vec3(cartesian); // the vec3 constructor implicitly discards the w component
    }
}
