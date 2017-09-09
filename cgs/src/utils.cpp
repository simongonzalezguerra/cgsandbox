#include "cgs/utils.hpp"
#include "glm/glm.hpp"

namespace cgs
{
    float fov_to_fovy(float fov, float width, float height)
    {
        return fov * (height / width); 
    }
}
