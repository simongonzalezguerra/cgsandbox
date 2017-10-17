#ifndef GLSANDBOX_UTILS_HPP
#define GLSANDBOX_UTILS_HPP

#include "glm/glm.hpp"
#include "cgs/log.hpp"

#include <stdexcept>
#include <sstream>

namespace cgs
{
    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to call an arbitrary function without arguments.
    //-----------------------------------------------------------------------------------------------
    typedef void (*process_func)();

    struct no_throw_block
    {
        no_throw_block(process_func func) : m_process_func(func) {}

        void operator()() const
        {
            try
            {
                m_process_func();
            }
            catch(std::exception& ex)
            {
                std::ostringstream oss;
                oss << "Unexpected exception, aborting block. Message: " << ex.what();
                log(LOG_LEVEL_ERROR, oss.str());
            }
        }

        process_func m_process_func;
    };

    struct error_counting_block
    {
        error_counting_block(process_func func, unsigned int max_errors) :
            m_process_func(func),
            m_num_errors(0U),
            m_max_errors(max_errors) {}

        void operator()()
        {
            try
            {
                m_process_func();
            }
            catch(std::exception& ex)
            {
                m_num_errors++;
                if (m_num_errors > m_max_errors)
                {
                    m_num_errors = 0;
                    throw std::runtime_error("Too many errors");
                }
            }
        }

        process_func m_process_func;
        unsigned int m_num_errors;
        unsigned int m_max_errors;
    };

    // Retuns fovy in radians. This is how glm::perspective expects it
    float fov_to_fovy(float fov_radians, float window_width, float window_height);
    // Extracts the camera position in worldspace from the view matrix
    // Constraint: this works if the view matrix doesn't involve scaling (tipical situation)
    // For the more general case where the view matrix can include scaling see this page:
    // https://www.opengl.org/discussion_boards/showthread.php/178484-Extracting-camera-position-from-a-ModelView-Matrix
    glm::vec3 camera_position_worldspace_from_view_matrix(const glm::mat4 view_matrix);
}

#endif
