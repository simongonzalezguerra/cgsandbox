#include "glsandbox/utils.hpp"
#include "glm/glm.hpp"

namespace glsandbox
{
  float fov_to_fovy(float fov, float width, float height)
  {
    float d = width / (2 * tanf(glm::radians(fov / 2)));
    return 2 * glm::degrees(atan(height / (2 * d)));
  }
}