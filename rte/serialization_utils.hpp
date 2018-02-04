#ifndef SERIALIZATION_UTILS_HPP
#define SERIALIZATION_UTILS_HPP

#include "rte_common.hpp"
#include "glm/glm.hpp"

#include <iostream>
#include <string>

namespace rte
{
    std::string format_user_id(user_id uid);
    std::ostream& operator<<(std::ostream&, const glm::mat4& mat);
    std::ostream& operator<<(std::ostream&, const glm::vec3& vec);
}

#endif
