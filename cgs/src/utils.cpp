#include "cgs/utils.hpp"
#include "glm/glm.hpp"

namespace cgs
{
    float fov_to_fovy(float fov_radians, float width, float height)
    {
        float d = width / (2.0f * tanf(fov_radians / 2.0f));
        return 2.0f * atan(height / (2.0f * d));
    }
}
