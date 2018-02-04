#include "serialization_utils.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <sstream>

namespace rte
{
    std::string format_user_id(user_id uid)
    {
        std::ostringstream oss;
        if (uid != nuser_id) {
            oss << uid;
        } else {
            oss << "nuser_id";
        }

        return oss.str();
    }

    std::ostream& operator<<(std::ostream& os, const glm::mat4& mat)
    {
        os << "[";
        os << " " << glm::value_ptr(mat)[0];
        for (std::size_t i = 1; i < 16; i++) {
            os << ", " << glm::value_ptr(mat)[i];
        }
        os << " ]";

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const glm::vec3& vec)
    {
        os << "[";
        os << " " << glm::value_ptr(vec)[0];
        for (std::size_t i = 1; i < 3; i++) {
            os << ", " << glm::value_ptr(vec)[i];
        }
        os << " ]";

        return os;   
    }
}
