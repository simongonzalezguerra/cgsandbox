#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "math_utils.hpp"
#include "control.hpp"
#include "glm/glm.hpp"
#include "log.hpp"

#include <iomanip>
#include <sstream>

namespace rte
{
    //-----------------------------------------------------------------------------------------------
    // fps_camera_controller
    //-----------------------------------------------------------------------------------------------
    constexpr float max_pitch =  85.0f;
    constexpr float min_pitch = -85.0f;

    class fps_camera_controller::fps_camera_controller_impl
    {
    public:
        fps_camera_controller_impl(view_database& db) :
            m_db(db),
            m_position{0.0f},
            m_yaw(0.0f),
            m_pitch(0.0f),
            m_speed(0.0f),
            m_mouse_speed(0.0f),
            m_moving_forward(false),
            m_moving_backward(false),
            m_moving_right(false),
            m_moving_left(false) {}

        void process(float dt, const std::vector<rte::event>& events)
        {
            // Process events
            for (auto it = events.cbegin(); it != events.cend(); it++) {
                if (it->type == rte::EVENT_MOUSE_MOVE) {
                    // Compute new orientation
                    // Yaw rotates the camera around the Y axis counter-clockwise. Mouse X coordinates increase
                    // to the right, so we use mouse motion to substract from yaw.
                    m_yaw -= m_mouse_speed * it->delta_mouse_x;
                    // Pitch rotates the camera around the X axis counter-clockwise. Mouse Y coordinates
                    // increase down, so we use mouse motion to subtract from yaw
                    m_pitch = glm::clamp(m_pitch - m_mouse_speed * it->delta_mouse_y, min_pitch, max_pitch);
    
                    // std::ostringstream oss;
                    // oss << "fps_camera_controller: updated angles, "<< std::fixed << std::setprecision(2)
                    //     << " yaw: " << m_yaw << ", pitch: " << m_pitch;
                    // rte::log(rte::LOG_LEVEL_DEBUG, oss.str());
                }
    
                if (it->type == rte::EVENT_KEY_PRESS) {
                    if (it->value == rte::KEY_W) {
                        m_moving_forward = true;
                        m_moving_backward = false;
                    } else if (it->value == rte::KEY_S) {
                        m_moving_forward = false;
                        m_moving_backward = true;
                    } else if (it->value == rte::KEY_D) {
                        m_moving_right = true;
                        m_moving_left = false;
                    } else if (it->value == rte::KEY_A) {
                        m_moving_right = false;
                        m_moving_left = true;
                    }
                } else if (it->type == rte::EVENT_KEY_RELEASE) {
                    if (it->value == rte::KEY_W || it->value == rte::KEY_S) {
                        m_moving_forward = false;
                        m_moving_backward = false;
                    } else if (it->value == rte::KEY_D || it->value == rte::KEY_A) {
                        m_moving_right = false;
                        m_moving_left = false;
                    }
                }
            }
    
            // Update view
    
            // Direction : Spherical coordinates to Cartesian coordinates conversion
            // Result of putting the vector (0, 0, -1) through an extrinsic rotation of m_pitch degrees
            // around X and m_yaw around Y
            glm::vec3 direction = glm::vec3(-sinf(glm::radians(m_yaw)), sinf(glm::radians(m_pitch)), -cosf(glm::radians(m_yaw)));
    
            // Result of putting the vector (1, 0, 0) through a rotation of m_yaw degrees around Y
            glm::vec3 right = glm::vec3(-sinf(glm::radians(m_yaw - 90.0f)), 0.0f, -cosf(glm::radians(m_yaw - 90.0f)));
    
            glm::vec3 up = glm::cross(right, direction);
    
            // Compute new m_position based on direction and time
            if (m_moving_forward){
                // Move forward
                m_position += direction * dt * m_speed;
                log_position();
            }
            if (m_moving_backward){
                // Move backward
                m_position -= direction * dt * m_speed;
                log_position();
            }
            if (m_moving_right){
                // Strafe right
                m_position += right * dt * m_speed;
                log_position();
            }
            if (m_moving_left){
                // Strafe left
                m_position -= right * dt * m_speed;
                log_position();
            }
    
            m_db.m_view_transform = glm::lookAt(m_position, m_position + direction, up);
        }

        void log_position()
        {
            // std::ostringstream oss;
            // oss << "fps_camera_controller: updated position, position: " << std::fixed << std::setprecision(2)
            //     << m_position.x << ", " << m_position.y << ", " << m_position.z;
            // rte::log(rte::LOG_LEVEL_DEBUG, oss.str());
        }

        view_database&      m_db;
        glm::vec3           m_position;
        float               m_yaw;
        float               m_pitch;
        float               m_speed;
        float               m_mouse_speed;
        bool                m_moving_forward;
        bool                m_moving_backward;
        bool                m_moving_right;
        bool                m_moving_left;
    };

    fps_camera_controller::fps_camera_controller(view_database& db) :
        m_impl(std::make_unique<fps_camera_controller_impl>(db)) {}

    fps_camera_controller::~fps_camera_controller() {}

    void fps_camera_controller::set_position(glm::vec3 position)
    {
        m_impl->m_position = position;
    }

    void fps_camera_controller::set_yaw(float yaw)
    {
        m_impl->m_yaw = yaw;
    }

    void fps_camera_controller::set_pitch(float pitch)
    {
        m_impl->m_pitch = pitch;
    }

    void fps_camera_controller::set_speed(float speed)
    {
        m_impl->m_speed = speed;
    }

    void fps_camera_controller::set_mouse_speed(float m_mouse_speed)
    {
        m_impl->m_mouse_speed = m_mouse_speed;
    }

    glm::vec3 fps_camera_controller::get_position()
    {
        return m_impl->m_position;
    }

    float fps_camera_controller::get_yaw()
    {
        return m_impl->m_yaw;
    }

    float fps_camera_controller::get_pitch()
    {
        return m_impl->m_pitch;
    }

    float fps_camera_controller::get_speed()
    {
        return m_impl->m_speed;
    }

    float fps_camera_controller::get_mouse_speed()
    {
        return m_impl->m_mouse_speed;
    }

    void fps_camera_controller::process(float dt, const std::vector<rte::event>& events)
    {
        m_impl->process(dt, events);
    }

    //-----------------------------------------------------------------------------------------------
    // perspective_controller
    //-----------------------------------------------------------------------------------------------
    constexpr float max_fov_radians = glm::radians(120.0f);
    constexpr float min_fov_radians = glm::radians(60.0f);

    class perspective_controller::perspective_controller_impl
    {
    public:
        perspective_controller_impl(view_database& db) :
            m_db(db),
            m_window_width(0.0f),
            m_window_height(0.0f),
            m_increasing_fov(false),
            m_decreasing_fov(false),
            m_fov_speed(0.0f),
            m_fov_radians(0.0f),
            m_near(0.1f),
            m_far(100.0f) {}

        void set_fov_radians(float fov_radians)
        {
            m_fov_radians = glm::clamp(fov_radians, min_fov_radians, max_fov_radians);

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "perspective_controller, fov degrees: "
                << glm::degrees(m_fov_radians)
                << ", fovy degrees: "
                << glm::degrees(rte::fov_to_fovy(m_fov_radians, m_window_width, m_window_height));
            rte::log(rte::LOG_LEVEL_DEBUG, oss.str());
        }

        void process(float dt, const std::vector<rte::event>& events)
        {
            for (auto it = events.cbegin(); it != events.cend(); it++) {
                if (it->type == rte::EVENT_KEY_PRESS) {
                    if (it->value == rte::KEY_9) {
                        m_increasing_fov = false;
                        m_decreasing_fov = true;
                    } else if (it->value == rte::KEY_0) {
                        m_increasing_fov = true;
                        m_decreasing_fov = false;
                    }
                } else if (it->type == rte::EVENT_KEY_RELEASE) {
                    if (it->value == rte::KEY_9 || it->value == rte::KEY_0) {
                        m_increasing_fov = false;
                        m_decreasing_fov = false; 
                    }
                }
            }

            if (m_increasing_fov) {
                set_fov_radians(m_fov_radians + m_fov_speed * dt);
            } else if (m_decreasing_fov) {
                set_fov_radians(m_fov_radians - m_fov_speed * dt);
            }

            // The fovy parameter to glm::perspective is the full vertical fov, not the half! the reason
            // they usually use 45 is that 90.0 would look weird. 90 would be ok for horizontal fov, not
            // vertical
            // https://www.opengl.org/discussion_boards/showthread.php/171227-glm-perspective-fovy-question
            // Also, the fovy parameter is in radians, not degrees! there are some old versions
            // of the glm documentation that state this depends on a preprocessor symbol, but
            // version 0.9.7 states explicitly that it's in radians:
            // http://glm.g-truc.net/0.9.7/api/a00174.html#gac3613dcb6c6916465ad5b7ad5a786175
            // This is why our utility function fov_to_fovy takes radians and returns radians
            float fovy_radians = rte::fov_to_fovy(m_fov_radians, m_window_width, m_window_height);
            m_db.m_projection_transform = glm::perspective(fovy_radians, m_window_width / m_window_height, m_near, m_far);
        }

        // Member variables
        view_database&      m_db;
        float               m_window_width;
        float               m_window_height;
        bool                m_increasing_fov;
        bool                m_decreasing_fov;
        float               m_fov_speed;
        float               m_fov_radians;
        float               m_near;
        float               m_far;
    };

    perspective_controller::perspective_controller(view_database& db) :
        m_impl(std::make_unique<perspective_controller_impl>(db)) {}

    perspective_controller::~perspective_controller() {}

    void perspective_controller::set_window_width(float window_width)
    {
        m_impl->m_window_width = window_width;
    }

    void perspective_controller::set_window_height(float window_height)
    {
        m_impl->m_window_height = window_height;
    }

    void  perspective_controller::set_fov_speed(float fov_speed)
    {
        m_impl->m_fov_speed = fov_speed;
    }

    void  perspective_controller::set_fov_radians(float fov)
    {
        m_impl->set_fov_radians(fov);
    }

    void perspective_controller::set_near(float near)
    {
        m_impl->m_near = near;
    }

    void perspective_controller::set_far(float far)
    {
        m_impl->m_far = far;
    }

    float perspective_controller::get_fov_speed()
    {
        return m_impl->m_fov_speed;
    }

    float perspective_controller::get_fov_radians()
    {
        return m_impl->m_fov_radians;
    }

    float perspective_controller::get_window_width()
    {
        return m_impl->m_window_width;
    }

    float perspective_controller::get_window_height()
    {
        return m_impl->m_window_height;
    }

    float perspective_controller::get_near()
    {
        return m_impl->m_near;
    }

    float perspective_controller::get_far()
    {
        return m_impl->m_far;
    }

    void perspective_controller::process(float dt, const std::vector<rte::event>& events)
    {
        m_impl->process(dt, events);
    }

    //-----------------------------------------------------------------------------------------------
    // framerate_controller
    //-----------------------------------------------------------------------------------------------
    class framerate_controller::framerate_controller_impl
    {
    public:
        framerate_controller_impl() :
            m_n_frames(0U),
            m_framerate_sample_time(0.0f),
            m_minimum_framerate(0.0f),
            m_maximum_framerate(0.0f),
            m_average_framerate(0.0f) {}

        void process(float dt)
        {
            // Framerate calculation
            m_n_frames++;
            m_framerate_sample_time += dt;
            if (m_framerate_sample_time >= 1.0) {
                float framerate = ((float) m_n_frames / m_framerate_sample_time);
                m_minimum_framerate = (framerate < m_minimum_framerate || m_minimum_framerate == 0.0f)? framerate : m_minimum_framerate;
                m_maximum_framerate = (framerate > m_maximum_framerate || m_maximum_framerate == 0.0f)? framerate : m_maximum_framerate;
                m_average_framerate = (m_average_framerate == 0.0f)?  framerate : (0.5f * m_average_framerate + 0.5 * framerate);
                m_n_frames = 0U;
                m_framerate_sample_time = 0.0f;
            }
        }

        // Member variables
        unsigned int      m_n_frames;
        float             m_framerate_sample_time;
        float             m_minimum_framerate;
        float             m_maximum_framerate;
        float             m_average_framerate;
    };

    framerate_controller::framerate_controller() :
        m_impl(std::make_unique<framerate_controller_impl>()) {}

    framerate_controller::~framerate_controller() {}

    float framerate_controller::get_minimum_framerate()
    {
        return m_impl->m_minimum_framerate;
    }

    float framerate_controller::get_maximum_framerate()
    {
        return m_impl->m_maximum_framerate;
    }

    float framerate_controller::get_average_framerate()
    {
        return m_impl->m_average_framerate;
    }

    void framerate_controller::log_stats()
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "framerate_controller, framerate: "
            << "min: " << m_impl->m_minimum_framerate
            << ", max: " << m_impl->m_maximum_framerate
            << ", avg: " << m_impl->m_average_framerate;
        rte::log(rte::LOG_LEVEL_DEBUG, oss.str());
    }

    void framerate_controller::process(float dt, const std::vector<rte::event>&)
    {
        m_impl->process(dt);
    }
}
