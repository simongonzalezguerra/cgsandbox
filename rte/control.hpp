#ifndef CONTROL_HPP
#define CONTROL_HPP

#include "rte_domain.hpp"
#include "renderer.hpp"
#include "system.hpp"

#include <vector>
#include <memory>

namespace rte
{
    class fps_camera_controller
    {
    public:
        fps_camera_controller(view_database& db);
        ~fps_camera_controller();
        void set_scene(index_type scene);
        void set_position(glm::vec3 position);
        void set_yaw(float yaw);
        void set_pitch(float pitch);
        void set_speed(float speed);
        void set_mouse_speed(float mouse_speed);
        index_type get_scene();
        glm::vec3 get_position();
        float get_yaw();
        float get_pitch();
        float get_speed();
        float get_mouse_speed();
        void process(float dt, const std::vector<rte::event>& events);

    private:
        class fps_camera_controller_impl;
        std::unique_ptr<fps_camera_controller_impl> m_impl;
    };

    class perspective_controller
    {
    public:
        perspective_controller(view_database& db);
        ~perspective_controller();
        void set_scene(index_type scene);
        void set_window_width(float window_width);
        void set_window_height(float window_height);
        void set_fov_speed(float fov_speed);
        void set_fov_radians(float fov_radians);
        void set_near(float near);
        void set_far(float far);
        float get_fov_speed();
        float get_fov_radians();
        index_type get_scene();
        float get_window_width();
        float get_window_height();
        float get_near();
        float get_far();
        void process(float dt, const std::vector<rte::event>& events);

    private:
        class perspective_controller_impl;
        std::unique_ptr<perspective_controller_impl> m_impl;
    };

    class framerate_controller
    {
    public:
        framerate_controller();
        ~framerate_controller();
        float get_minimum_framerate();
        float get_maximum_framerate();
        float get_average_framerate();
        void log_stats();
        void process(float dt, const std::vector<rte::event>& events);

    private:
        class framerate_controller_impl;
        std::unique_ptr<framerate_controller_impl> m_impl;
    };
}

#endif
