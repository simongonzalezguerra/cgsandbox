#include "glm/gtx/transform.hpp"
#include "real_time_engine.hpp"
#include "database_loader.hpp"
#include "resource_loader.hpp"
#include "cmd_line_args.hpp"
#include "opengl_driver.hpp"
#include "sparse_list.hpp"
#include "rte_domain.hpp"
#include "renderer.hpp"
#include "control.hpp"
#include "system.hpp"
#include "log.hpp"

#include <stdexcept>
#include <iostream>
#include <sstream>

namespace rte
{
    //-------------------------------------------------------------------------------------------------
    // real_time_engine
    //-------------------------------------------------------------------------------------------------
    class real_time_engine::real_time_engine_impl
    {
    public:
        real_time_engine_impl(unsigned int max_errors) :
            m_events(),
            m_max_errors(max_errors),
            m_last_time(0.0f),
            m_should_continue(true),
            m_sim_rotation_speed(0.0f),
            m_sim_rotation_yaw(0.0f),
            m_is_initialized(false),
            m_view_db(),
            m_fps_camera_controller(m_view_db),
            m_framerate_controller(),
            m_perspective_controller(m_view_db)
        {
        }

        ~real_time_engine_impl()
        {
            finalize();
        }

        void initialize(unsigned int argc, char** argv)
        {
            if (m_is_initialized) return;

            log(LOG_LEVEL_DEBUG, "real_time_engine: initializing application");

            attach_logstream(default_logstream_stdout_callback);
            attach_logstream(default_logstream_file_callback);

            cmd_line_args_initialize();
            cmd_line_args_set_args(argc, argv);

            if (!cmd_line_args_has_option("-config")) {
                throw std::logic_error("Usage: ./real_time_engine -config <config_file>");
            }

            system_initialize();

            database_loader_initialize();
            load_database(m_view_db);
            log_database(m_view_db);

            unique_window window = make_window(896, 504, false);

            set_gl_driver(get_opengl_driver());

            initialize_renderer(m_view_db);

            // After all mesh buffers have been loaded in the graphics API, free the buffers
            m_view_db.m_mesh_buffers.clear();

            m_last_time = get_time();

            m_fps_camera_controller.set_position(glm::vec3(-14.28f, 13.71f, 29.35f));
            m_fps_camera_controller.set_yaw(-41.50f);
            m_fps_camera_controller.set_pitch(-20.37f);
            m_fps_camera_controller.set_speed(40.0f);
            m_fps_camera_controller.set_mouse_speed(0.1f);

            m_perspective_controller.set_window_width(1920.0f);
            m_perspective_controller.set_window_height(1080.0f);
            m_perspective_controller.set_fov_speed(0.5f);
            m_perspective_controller.set_fov_radians(glm::radians(70.0f));
            m_perspective_controller.set_near(0.1f);
            m_perspective_controller.set_far(500.0f);

            m_sim_rotation_speed = 1.5f;
            m_sim_rotation_yaw = 0.0f;

            m_window = std::move(window);
            m_is_initialized = true;
        }

        void finalize()
        {
            if (!m_is_initialized) return;

            try {
                log(LOG_LEVEL_DEBUG, "real_time_engine: finalizing application");
                m_framerate_controller.log_stats();
                finalize_renderer();
                m_window.reset();
                system_finalize();
                database_loader_finalize();
                cmd_line_args_finalize();
            } catch(...) {
                log(LOG_LEVEL_DEBUG, "real_time_engine: exception during finalization");
            }            
        }

        void process_events(const std::vector<event>& m_events) {
            for (auto it = m_events.cbegin(); it != m_events.cend(); it++) {
                if (it->type == EVENT_KEY_PRESS && it->value == KEY_ESCAPE) {
                    m_should_continue = false;
                }
            }
        }

        void compute_accum_transforms(view_database& db)
        {
            struct context { index_type node_index; };
            std::vector<context> pending_nodes;
            pending_nodes.push_back({db.m_root_node});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.back();
                pending_nodes.pop_back();

                auto& current_node = db.m_nodes.at(current.node_index);
                glm::mat4 previous_transform(1.0f);
                if (current_node.m_parent != npos) {
                    previous_transform = db.m_nodes.at(current_node.m_parent).m_accum_transform;
                }
                current_node.m_accum_transform = current_node.m_local_transform * previous_transform;

                // process current
                for (auto it = tree_rbegin(db.m_nodes, current.node_index); it != tree_rend(db.m_nodes, current.node_index); ++it) {
                    pending_nodes.push_back({index(it)});
                }
            }
        }

        bool frame()
        {
            // Delta time for simulation
            float current_time = get_time();
            float dt = float(current_time - m_last_time);
            m_last_time = current_time;

            poll_window_events();
            m_events.clear();
            get_window_events(m_window.get(), &m_events);

            // Check for escape key
            process_events(m_events);
        
            // Control camera
            m_fps_camera_controller.process(dt, m_events);
        
            // Control projection (update fov)
            m_perspective_controller.process(dt, m_events);

            compute_accum_transforms(m_view_db);

            // Render the frame
            render(m_view_db);
            swap_buffers(m_window.get());

            // Control framerate
            m_framerate_controller.process(dt, m_events);

            return !m_should_continue;            
        }

        std::vector<event>     m_events;
        unique_window          m_window;
        unsigned int           m_max_errors;
        float                  m_last_time;
        bool                   m_should_continue;
        float                  m_sim_rotation_speed;
        float                  m_sim_rotation_yaw;
        bool                   m_is_initialized;
        view_database          m_view_db;
        fps_camera_controller  m_fps_camera_controller;
        framerate_controller   m_framerate_controller;
        perspective_controller m_perspective_controller;
    };

    real_time_engine::real_time_engine(unsigned int max_errors) :
        m_impl(std::make_unique<real_time_engine_impl>(max_errors)) {}

    real_time_engine::~real_time_engine() {}

    void real_time_engine::initialize(unsigned int argc, char** argv)
    {
        m_impl->initialize(argc, argv);
    }

    void real_time_engine::process()
    {
        bool done = false;
        unsigned int n_errors = 0U;
        do {
            try {
                done = m_impl->frame();
            } catch(std::exception& ex) {
                log(LOG_LEVEL_ERROR, ex.what());
                n_errors++;
                if (n_errors == 100) {
                    throw std::logic_error("real_time_engine: too many errors, stoping frame loop");
                }
            }
        } while (!done);
    }

    void real_time_engine::finalize()
    {
        m_impl->finalize();
    }
}
