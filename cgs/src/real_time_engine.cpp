#include "cgs/resource_database.hpp"
#include "cgs/real_time_engine.hpp"
#include "cgs/resource_loader.hpp"
#include "glm/gtx/transform.hpp"
#include "cgs/opengl_driver.hpp"
#include "cgs/scenegraph.hpp"
#include "cgs/renderer.hpp"
#include "cgs/control.hpp"
#include "cgs/system.hpp"
#include "cgs/log.hpp"

#include <stdexcept>
#include <iostream>
#include <sstream>

namespace cgs
{
    //-------------------------------------------------------------------------------------------------
    // real_time_engine
    //-------------------------------------------------------------------------------------------------
    class real_time_engine::real_time_engine_impl
    {
    public:
        real_time_engine_impl(unsigned int max_errors) :
            m_floor_resource(nresource),
            m_plane_resource(nresource),
            m_teapot_resource(nresource),
            m_bunny_resource(nresource),
            m_dragon_resource(nresource),
            m_floor_node(nlayer),
            m_plane_node(nnode),
            m_diffuse_teapot_node(nnode),
            m_bunny_node(nnode),
            m_diffuse_dragon_node(nnode),
            m_steel_dragon_node(nnode),
            m_steel_teapot_node(nnode),
            m_glass_bunny_node(nnode),
            m_diffuse_teapot_material(nmat),
            m_steel_material(nmat),
            m_glass_material(nmat),
            m_skybox_id(ncubemap),
            m_max_errors(max_errors),
            m_view(0U),
            m_layer(nlayer),
            m_fps_camera_controller(),
            m_framerate_controller(),
            m_perspective_controller(),
            m_last_time(0.0f),
            m_should_continue(true),
            m_sim_rotation_speed(0.0f),
            m_sim_rotation_yaw(0.0f)
        {
            log_init();
            log(LOG_LEVEL_DEBUG, "real_time_engine: initializing application");

            attach_logstream(default_logstream_stdout_callback);
            attach_logstream(default_logstream_file_callback);
            resource_database_init();

            m_floor_resource = make_floor_resource();

            std::vector<mat_id> materials_out;
            std::vector<mesh_id> meshes_out;
            m_plane_resource = load_resources("../../../resources/f-14D-super-tomcat/F-14D_SuperTomcatRotated.obj", &materials_out, &meshes_out);

            m_teapot_resource = load_resources("../../../resources/Teapot/Teapot.obj", &materials_out, &meshes_out);

            m_diffuse_teapot_material = add_material();
            set_material_diffuse_color(m_diffuse_teapot_material, glm::vec3(1.0f, 0.0f, 0.0f));
            set_material_specular_color(m_diffuse_teapot_material, glm::vec3(1.0f, 1.0f, 1.0f));
            set_material_smoothness(m_diffuse_teapot_material, 1.0f);

            m_steel_material = add_material();
            set_material_diffuse_color(m_steel_material, glm::vec3(0.8f, 0.8f, 0.8f));
            set_material_specular_color(m_steel_material, glm::vec3(0.8f, 0.8f, 0.8f));
            set_material_reflectivity(m_steel_material, 1.0f);
            set_material_translucency(m_steel_material, 0.0f);

            m_glass_material = add_material();
            set_material_diffuse_color(m_glass_material, glm::vec3(0.5f, 0.5f, 0.5f));
            set_material_specular_color(m_glass_material, glm::vec3(0.5f, 0.5f, 0.5f));
            set_material_reflectivity(m_glass_material, 0.0f);
            set_material_translucency(m_glass_material, 1.0f);
            set_material_refractive_index(m_glass_material, 1.52f); // Glass

            m_bunny_resource = load_resources("../../../resources/stanford-bunny/bun_zipper.ply", &materials_out, &meshes_out);

            mat_id bunny_material = add_material();
            set_material_diffuse_color(bunny_material, glm::vec3(0.0f, 0.2f, 0.4f));
            set_material_specular_color(bunny_material, glm::vec3(1.0f, 1.0f, 1.0f));
            set_material_smoothness(bunny_material, 1.0f);
            set_resource_material(m_bunny_resource, bunny_material);

            m_dragon_resource = load_resources("../../../resources/stanford-dragon2/dragon.obj", &materials_out, &meshes_out);

            mat_id dragon_material = add_material();
            set_material_diffuse_color(dragon_material, glm::vec3(0.05f, 0.5f, 0.0f));
            set_material_specular_color(dragon_material, glm::vec3(0.5f, 0.5f, 0.5f));
            set_material_smoothness(dragon_material, 1.0f);
            set_resource_material(m_dragon_resource, dragon_material);

            m_skybox_id = add_cubemap();
            std::vector<std::string> skybox_faces
            {
                "../../../resources/skybox_sea/right.jpg",
                "../../../resources/skybox_sea/left.jpg",
                "../../../resources/skybox_sea/top.jpg",
                "../../../resources/skybox_sea/bottom.jpg",
                "../../../resources/skybox_sea/back.jpg",
                "../../../resources/skybox_sea/front.jpg"
            };
            set_cubemap_faces(m_skybox_id, skybox_faces);

            open_window(1920, 1080, false);

            set_gl_driver(get_opengl_driver());

            initialize_renderer();

            m_view = add_view();
            m_layer = add_layer(m_view);
            set_layer_skybox(m_layer, m_skybox_id);
            set_directional_light_ambient_color(m_layer,  glm::vec3(0.05f, 0.05f, 0.05f));
            set_directional_light_diffuse_color(m_layer,  glm::vec3(0.5f, 0.5f, 0.5f));
            set_directional_light_specular_color(m_layer, glm::vec3(0.5f, 0.5f, 0.5f));
            set_directional_light_direction(m_layer, glm::vec3(0.0f, -1.0f, 0.0f));

            // Create the floor
            m_floor_node = add_node(m_layer, root_node, m_floor_resource);
            set_node_transform(m_layer, m_floor_node, glm::translate(glm::vec3(50.0f, -3.0f, -5.0f)) * glm::scale(glm::vec3(120.0f, 120.0f, 120.0f)));

            // Create the plane
            m_plane_node = add_node(m_layer, root_node, m_plane_resource);
        
            // Create the diffuse teapot
            m_diffuse_teapot_node = add_node(m_layer, root_node, m_teapot_resource);
            set_node_transform(m_layer, m_diffuse_teapot_node, glm::translate(glm::vec3(22.0f, -3.0f, -20.0f)) * glm::scale(glm::vec3(8.0f, 8.0f, 8.0f)));
            set_node_material(m_layer, get_first_child_node(m_layer, m_diffuse_teapot_node), m_diffuse_teapot_material);
        
            // Create the bunny
            m_bunny_node = add_node(m_layer, root_node, m_bunny_resource);
            set_node_transform(m_layer, m_bunny_node, glm::translate(glm::vec3(47.0f, -5.0f, 0.0f)) * glm::scale(glm::vec3(60.0f, 60.0f, 60.0f)));
        
            // Create the diffuse dragon
            m_diffuse_dragon_node = add_node(m_layer, root_node, m_dragon_resource);
            set_node_transform(m_layer, m_diffuse_dragon_node, glm::translate(glm::vec3(65.0f, -3.0f, 0.0f)));

            // Create the steel dragon
            m_steel_dragon_node = add_node(m_layer, root_node, m_dragon_resource);
            set_node_material(m_layer, get_first_child_node(m_layer, m_steel_dragon_node), m_steel_material);

            // Create the steel teapot
            m_steel_teapot_node = add_node(m_layer, root_node, m_teapot_resource);
            set_node_material(m_layer, get_first_child_node(m_layer, m_steel_teapot_node), m_steel_material);

            // Create the glass bunny
            m_glass_bunny_node = add_node(m_layer, root_node, m_bunny_resource);
            set_node_material(m_layer, m_glass_bunny_node, m_glass_material);

            point_light_id point_light = add_point_light(m_layer);
            set_point_light_position(m_layer, point_light, glm::vec3(4.0f, 4.0f, 4.0f));
            set_point_light_ambient_color(m_layer, point_light, glm::vec3(0.1f, 0.1f, 0.1f));
            set_point_light_diffuse_color(m_layer, point_light, glm::vec3(1.0f, 1.0f, 1.0f));
            set_point_light_specular_color(m_layer, point_light, glm::vec3(0.3f, 0.3f, 0.3f));
            set_point_light_constant_attenuation(m_layer, point_light, 1.0f);
            set_point_light_linear_attenuation(m_layer, point_light, 0.0f); // Be careful with this parameter, it can dim your point light pretty fast
            set_point_light_quadratic_attenuation(m_layer, point_light, 0.0f); // Be careful with this parameter, it can dim your point light pretty fast

            m_last_time = get_time();

            m_fps_camera_controller.set_layer(m_layer);
            m_fps_camera_controller.set_position(glm::vec3(-14.28f, 13.71f, 29.35f));
            m_fps_camera_controller.set_yaw(-41.50f);
            m_fps_camera_controller.set_pitch(-20.37f);
            m_fps_camera_controller.set_speed(40.0f);
            m_fps_camera_controller.set_mouse_speed(0.1f);

            m_perspective_controller.set_layer(m_layer);
            m_perspective_controller.set_window_width(1920.0f);
            m_perspective_controller.set_window_height(1080.0f);
            m_perspective_controller.set_fov_speed(0.5f);
            m_perspective_controller.set_fov_radians(glm::radians(70.0f));
            m_perspective_controller.set_near(0.1f);
            m_perspective_controller.set_far(500.0f);

            m_sim_rotation_speed = 1.5f;
            m_sim_rotation_yaw = 0.0f;
        }

        ~real_time_engine_impl()
        {
            try {
                log(LOG_LEVEL_DEBUG, "real_time_engine: finalizing application");
                m_framerate_controller.log_stats();
                finalize_renderer();
                close_window();
                detach_all_logstreams();
            } catch(...) {
                std::cout << "real_time_engine: exception during finalization";
            }
        }

        resource_id make_floor_resource()
        {
            mesh_id floor_mesh = add_mesh();
            std::vector<glm::vec3> vertices =
            {
                { 1.0f, 0.0f,  1.0f},
                {-1.0f, 0.0f,  1.0f},
                {-1.0f, 0.0f, -1.0f},
                { 1.0f, 0.0f, -1.0f}
            };
            std::vector<glm::vec3> normals =
            {
                {0.0f, 1.0f, 0.0f},
                {0.0f, 1.0f, 0.0f},
                {0.0f, 1.0f, 0.0f},
                {0.0f, 1.0f, 0.0f}
            };
            std::vector<glm::vec2> texture_coords =
            {
                {20.0f,  0.0f},
                { 0.0f,  0.0f},
                { 0.0f, 20.0f},
                {20.0f, 20.0f}
            };
            std::vector<vindex> indices = {0, 2, 1, 0, 3, 2};
            set_mesh_vertices(floor_mesh, vertices);
            set_mesh_normals(floor_mesh, normals);
            set_mesh_texture_coords(floor_mesh, texture_coords);
            set_mesh_indices(floor_mesh, indices);

            mat_id mat = add_material();
            set_material_diffuse_color(mat, glm::vec3(0.8f, 0.8f, 0.8f));
            set_material_specular_color(mat, glm::vec3(0.8f, 0.8f, 0.8f));
            set_material_reflectivity(mat, 0.0f);
            set_material_translucency(mat, 0.0f);
            set_material_texture_path(mat, "../../../resources/Wood/wood.png");

            resource_id floor_resource = add_resource(root_resource);
            set_resource_mesh(floor_resource, floor_mesh);
            set_resource_material(floor_resource,  mat);

            return floor_resource;
        }

       void process_events(const std::vector<event>& events) {
            for (auto it = events.cbegin(); it != events.cend(); it++) {
                if (it->type == EVENT_KEY_PRESS && it->value == KEY_ESCAPE) {
                    m_should_continue = false;
                }
            }
        }

        void update_simulation(float dt)
        {
            m_sim_rotation_yaw += m_sim_rotation_speed * dt;
            set_node_transform(m_layer, m_plane_node, glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));

            set_node_transform(m_layer, m_steel_dragon_node, glm::translate(glm::vec3(65.0f, -3.0f, -20.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));

            set_node_transform(m_layer, m_glass_bunny_node, glm::translate(glm::vec3(47.0f, -5.0f, -20.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f})
                                           * glm::scale(glm::vec3(60.0f, 60.0f, 60.0f)));

            set_node_transform(m_layer, m_steel_teapot_node, glm::translate(glm::vec3(22.0f, -3.0f, 0.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f})
                                           * glm::scale(glm::vec3(8.0f, 8.0f, 8.0f)));
        }

        bool frame()
        {
            // Delta time for simulation
            float current_time = get_time();
            float dt = float(current_time - m_last_time);
            m_last_time = current_time;

            std::vector<event> events = poll_events();

            // Update the simulation with time passed since last frame
            update_simulation(dt);
      
            // Check for escape key
            process_events(events);
        
            // Control camera
            m_fps_camera_controller.process(dt, events);
        
            // Control projection (update fov)
            m_perspective_controller.process(dt, events);

            // Render the scene
            render(m_view);
            swap_buffers();

            // Control framerate
            m_framerate_controller.process(dt, events);

            return !m_should_continue;            
        }

        resource_id            m_floor_resource;
        resource_id            m_plane_resource;
        resource_id            m_teapot_resource;
        resource_id            m_bunny_resource;
        resource_id            m_dragon_resource;
        node_id                m_floor_node;
        node_id                m_plane_node;
        node_id                m_diffuse_teapot_node;
        node_id                m_bunny_node;
        node_id                m_diffuse_dragon_node;
        node_id                m_steel_dragon_node;
        node_id                m_steel_teapot_node;
        node_id                m_glass_bunny_node;
        mat_id                 m_diffuse_teapot_material;
        mat_id                 m_steel_material;
        mat_id                 m_glass_material;
        cubemap_id             m_skybox_id;
        unsigned int           m_max_errors;
        view_id                m_view;
        layer_id               m_layer;
        fps_camera_controller  m_fps_camera_controller;
        framerate_controller   m_framerate_controller;
        perspective_controller m_perspective_controller;
        float                  m_last_time;
        bool                   m_should_continue;
        float                  m_sim_rotation_speed;
        float                  m_sim_rotation_yaw;
    };

    real_time_engine::real_time_engine(unsigned int max_errors) :
        m_impl(std::make_unique<real_time_engine_impl>(max_errors)) {}

    real_time_engine::~real_time_engine() {}

    void real_time_engine::process()
    {
        bool done = false;
        unsigned int n_errors = 0U;
        do {
            try {
                done = m_impl->frame();
            } catch(std::exception& ex) {
                n_errors++;
                if (n_errors == 100) {
                    log(LOG_LEVEL_ERROR, "real_time_engine: too many errors, stoping frame loop");
                    throw std::logic_error("");
                }
            }
        } while (!done);
    }
}
