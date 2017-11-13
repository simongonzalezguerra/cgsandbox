#include "resource_database.hpp"
#include "real_time_engine.hpp"
#include "resource_loader.hpp"
#include "glm/gtx/transform.hpp"
#include "opengl_driver.hpp"
#include "scenegraph.hpp"
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
            m_materials(),
            m_meshes(),
            m_resources(),
            m_cubemaps(),
            m_nodes(),
            m_plane_node(nnode),
            m_steel_dragon_node(nnode),
            m_steel_teapot_node(nnode),
            m_glass_bunny_node(nnode),
            m_max_errors(max_errors),
            m_scene(),
            m_fps_camera_controller(),
            m_framerate_controller(),
            m_perspective_controller(),
            m_last_time(0.0f),
            m_should_continue(true),
            m_sim_rotation_speed(0.0f),
            m_sim_rotation_yaw(0.0f),
            m_is_initialized(false)
        {
        }

        ~real_time_engine_impl()
        {
            finalize();
        }

        void initialize()
        {
            if (m_is_initialized) return;

            log(LOG_LEVEL_DEBUG, "real_time_engine: initializing application");

            attach_logstream(default_logstream_stdout_callback);
            attach_logstream(default_logstream_file_callback);

            resource_database_init();

            system_initialize();

            material_vector added_materials;
            mesh_vector added_meshes;
            resource_vector added_resources;

            resource_id floor_resource = nresource;
            make_floor_resource(&floor_resource, &added_resources, &added_materials, &added_meshes);

            resource_id plane_resource = nresource;
            load_resources("../../../resources/f-14D-super-tomcat/F-14D_SuperTomcatRotated.obj", &plane_resource, &added_resources, &added_materials, &added_meshes);

            resource_id teapot_resource = nresource;
            load_resources("../../../resources/Teapot/Teapot.obj", &teapot_resource, &added_resources, &added_materials, &added_meshes);

            unique_material diffuse_teapot_material = make_material();
            set_material_diffuse_color(diffuse_teapot_material.get(), glm::vec3(1.0f, 0.0f, 0.0f));
            set_material_specular_color(diffuse_teapot_material.get(), glm::vec3(1.0f, 1.0f, 1.0f));
            set_material_smoothness(diffuse_teapot_material.get(), 1.0f);

            unique_material steel_material = make_material();
            set_material_diffuse_color(steel_material.get(), glm::vec3(0.8f, 0.8f, 0.8f));
            set_material_specular_color(steel_material.get(), glm::vec3(0.8f, 0.8f, 0.8f));
            set_material_reflectivity(steel_material.get(), 1.0f);
            set_material_translucency(steel_material.get(), 0.0f);

            unique_material glass_material = make_material();
            set_material_diffuse_color(glass_material.get(), glm::vec3(0.5f, 0.5f, 0.5f));
            set_material_specular_color(glass_material.get(), glm::vec3(0.5f, 0.5f, 0.5f));
            set_material_reflectivity(glass_material.get(), 0.0f);
            set_material_translucency(glass_material.get(), 1.0f);
            set_material_refractive_index(glass_material.get(), 1.52f); // Glass

            resource_id bunny_resource = nresource;
            load_resources("../../../resources/stanford-bunny/bun_zipper.ply", &bunny_resource, &added_resources, &added_materials, &added_meshes);

            unique_material bunny_material = make_material();
            set_material_diffuse_color(bunny_material.get(), glm::vec3(0.0f, 0.2f, 0.4f));
            set_material_specular_color(bunny_material.get(), glm::vec3(1.0f, 1.0f, 1.0f));
            set_material_smoothness(bunny_material.get(), 1.0f);
            set_resource_material(bunny_resource, bunny_material.get());

            resource_id dragon_resource = nresource;
            load_resources("../../../resources/stanford-dragon2/dragon.obj", &dragon_resource, &added_resources, &added_materials, &added_meshes);

            unique_material dragon_material = make_material();
            set_material_diffuse_color(dragon_material.get(), glm::vec3(0.05f, 0.5f, 0.0f));
            set_material_specular_color(dragon_material.get(), glm::vec3(0.5f, 0.5f, 0.5f));
            set_material_smoothness(dragon_material.get(), 1.0f);
            set_resource_material(dragon_resource, dragon_material.get());

            unique_cubemap skybox = make_cubemap();
            std::vector<std::string> skybox_faces
            {
                "../../../resources/skybox_sea/right.jpg",
                "../../../resources/skybox_sea/left.jpg",
                "../../../resources/skybox_sea/top.jpg",
                "../../../resources/skybox_sea/bottom.jpg",
                "../../../resources/skybox_sea/back.jpg",
                "../../../resources/skybox_sea/front.jpg"
            };
            set_cubemap_faces(skybox.get(), skybox_faces);

            unique_window window = make_window(1920, 1080, false);

            set_gl_driver(get_opengl_driver());

            initialize_renderer();

            m_scene = make_scene();
            set_scene_skybox(m_scene.get(), skybox.get());
            set_directional_light_ambient_color(m_scene.get(),  glm::vec3(0.05f, 0.05f, 0.05f));
            set_directional_light_diffuse_color(m_scene.get(),  glm::vec3(0.5f, 0.5f, 0.5f));
            set_directional_light_specular_color(m_scene.get(), glm::vec3(0.5f, 0.5f, 0.5f));
            set_directional_light_direction(m_scene.get(), glm::vec3(0.0f, -1.0f, 0.0f));

            node_vector added_nodes;
            // Create the floor
            node_id root_node = get_scene_root_node(m_scene.get());
            node_id floor_node = nnode;
            make_node(root_node, floor_resource, &floor_node, &added_nodes);
            set_node_transform(floor_node, glm::translate(glm::vec3(50.0f, -3.0f, -5.0f)) * glm::scale(glm::vec3(120.0f, 120.0f, 120.0f)));

            // Create the plane
            node_id plane_node = nnode;
            make_node(root_node, plane_resource, &plane_node, &added_nodes);
        
            // Create the diffuse teapot
            node_id diffuse_teapot_node = nnode;
            make_node(root_node, teapot_resource, &diffuse_teapot_node, &added_nodes);
            set_node_transform(diffuse_teapot_node, glm::translate(glm::vec3(22.0f, -3.0f, -20.0f)) * glm::scale(glm::vec3(8.0f, 8.0f, 8.0f)));
            set_node_material(get_first_child_node(diffuse_teapot_node), diffuse_teapot_material.get());
        
            // Create the bunny
            node_id bunny_node = nnode;
            make_node(root_node, bunny_resource, &bunny_node, &added_nodes);
            set_node_transform(bunny_node, glm::translate(glm::vec3(47.0f, -5.0f, 0.0f)) * glm::scale(glm::vec3(60.0f, 60.0f, 60.0f)));
        
            // Create the diffuse dragon
            node_id dragon_node = nnode;
            make_node(root_node, dragon_resource, &dragon_node, &added_nodes);
            set_node_transform(dragon_node, glm::translate(glm::vec3(65.0f, -3.0f, 0.0f)));

            // Create the steel dragon
            node_id steel_dragon_node = nnode;
            make_node(root_node, dragon_resource, &steel_dragon_node, &added_nodes);;
            set_node_material(get_first_child_node(steel_dragon_node), steel_material.get());

            // Create the steel teapot
            node_id steel_teapot_node = nnode;
            make_node(root_node, teapot_resource, &steel_teapot_node, &added_nodes);
            set_node_material(get_first_child_node(steel_teapot_node), steel_material.get());

            // Create the glass bunny
            node_id glass_bunny_node = nnode;
            make_node(root_node, bunny_resource, &glass_bunny_node, &added_nodes);
            set_node_material(glass_bunny_node, glass_material.get());

            unique_point_light pl = make_point_light(m_scene.get());
            set_point_light_position(pl.get(), glm::vec3(34.5f, 15.0f, 10.0f));
            set_point_light_ambient_color(pl.get(), glm::vec3(0.1f, 0.1f, 0.1f));
            set_point_light_diffuse_color(pl.get(), glm::vec3(1.0f, 1.0f, 1.0f));
            set_point_light_specular_color(pl.get(), glm::vec3(0.3f, 0.3f, 0.3f));
            set_point_light_constant_attenuation(pl.get(), 1.0f);
            set_point_light_linear_attenuation(pl.get(), 0.0f); // Be careful with this parameter, it can dim your point light pretty fast
            set_point_light_quadratic_attenuation(pl.get(), 0.0f); // Be careful with this parameter, it can dim your point light pretty fast

            m_last_time = get_time();

            m_fps_camera_controller.set_scene(m_scene.get());
            m_fps_camera_controller.set_position(glm::vec3(-14.28f, 13.71f, 29.35f));
            m_fps_camera_controller.set_yaw(-41.50f);
            m_fps_camera_controller.set_pitch(-20.37f);
            m_fps_camera_controller.set_speed(40.0f);
            m_fps_camera_controller.set_mouse_speed(0.1f);

            m_perspective_controller.set_scene(m_scene.get());
            m_perspective_controller.set_window_width(1920.0f);
            m_perspective_controller.set_window_height(1080.0f);
            m_perspective_controller.set_fov_speed(0.5f);
            m_perspective_controller.set_fov_radians(glm::radians(70.0f));
            m_perspective_controller.set_near(0.1f);
            m_perspective_controller.set_far(500.0f);

            m_sim_rotation_speed = 1.5f;
            m_sim_rotation_yaw = 0.0f;

            m_window = std::move(window);
            m_materials.insert(m_materials.end(), make_move_iterator(added_materials.begin()), make_move_iterator(added_materials.end()));
            m_materials.push_back(std::move(diffuse_teapot_material));
            m_materials.push_back(std::move(steel_material));
            m_materials.push_back(std::move(glass_material));
            m_materials.push_back(std::move(bunny_material));
            m_materials.push_back(std::move(dragon_material));
            m_meshes.insert(m_meshes.end(), make_move_iterator(added_meshes.begin()), make_move_iterator(added_meshes.end()));
            m_resources.insert(m_resources.end(), make_move_iterator(added_resources.begin()), make_move_iterator(added_resources.end()));
            m_cubemaps.push_back(std::move(skybox));
            m_nodes.insert(m_nodes.end(), make_move_iterator(added_nodes.begin()), make_move_iterator(added_nodes.end()));
            m_point_lights.push_back(std::move(pl));
            m_plane_node = plane_node;
            m_steel_dragon_node = steel_dragon_node;
            m_steel_teapot_node = steel_teapot_node;
            m_glass_bunny_node = glass_bunny_node;
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
                m_cubemaps.clear();
                m_resources.clear();
                m_meshes.clear();
                m_materials.clear();
                m_is_initialized = false;
            } catch(...) {
                log(LOG_LEVEL_DEBUG, "real_time_engine: exception during finalization");
            }            
        }

        void make_floor_resource(resource_id* root_out, resource_vector* resources_out, material_vector* materials_out, mesh_vector* meshes_out)
        {
            unique_mesh floor_mesh = make_mesh();
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
            set_mesh_vertices(floor_mesh.get(), vertices);
            set_mesh_normals(floor_mesh.get(), normals);
            set_mesh_texture_coords(floor_mesh.get(), texture_coords);
            set_mesh_indices(floor_mesh.get(), indices);

            unique_material floor_mat = make_material();
            set_material_diffuse_color(floor_mat.get(), glm::vec3(0.8f, 0.8f, 0.8f));
            set_material_specular_color(floor_mat.get(), glm::vec3(0.8f, 0.8f, 0.8f));
            set_material_reflectivity(floor_mat.get(), 0.0f);
            set_material_translucency(floor_mat.get(), 0.0f);
            set_material_texture_path(floor_mat.get(), "../../../resources/Wood/wood.png");

            auto floor_resource = make_resource();
            set_resource_mesh(floor_resource.get(), floor_mesh.get());
            set_resource_material(floor_resource.get(),  floor_mat.get());
            *root_out = floor_resource.get();
            resources_out->push_back(std::move(floor_resource));
            materials_out->push_back(std::move(floor_mat));
            meshes_out->push_back(std::move(floor_mesh));
        }

       void process_events(const std::vector<event>& m_events) {
            for (auto it = m_events.cbegin(); it != m_events.cend(); it++) {
                if (it->type == EVENT_KEY_PRESS && it->value == KEY_ESCAPE) {
                    m_should_continue = false;
                }
            }
        }

        void update_simulation(float dt)
        {
            m_sim_rotation_yaw += m_sim_rotation_speed * dt;
            set_node_transform(m_plane_node, glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));

            set_node_transform(m_steel_dragon_node, glm::translate(glm::vec3(65.0f, -3.0f, -20.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));

            set_node_transform(m_glass_bunny_node, glm::translate(glm::vec3(47.0f, -5.0f, -20.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f})
                                           * glm::scale(glm::vec3(60.0f, 60.0f, 60.0f)));

            set_node_transform(m_steel_teapot_node, glm::translate(glm::vec3(22.0f, -3.0f, 0.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f})
                                           * glm::scale(glm::vec3(8.0f, 8.0f, 8.0f)));
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

            // Update the simulation with time passed since last frame
            update_simulation(dt);
      
            // Check for escape key
            process_events(m_events);
        
            // Control camera
            m_fps_camera_controller.process(dt, m_events);
        
            // Control projection (update fov)
            m_perspective_controller.process(dt, m_events);

            // Render the scene
            render();
            swap_buffers(m_window.get());

            // Control framerate
            m_framerate_controller.process(dt, m_events);

            return !m_should_continue;            
        }

        std::vector<event>     m_events;
        unique_window          m_window;
        material_vector        m_materials;
        mesh_vector            m_meshes;
        resource_vector        m_resources;
        cubemap_vector         m_cubemaps;
        node_vector            m_nodes;
        point_light_vector     m_point_lights;
        node_id                m_plane_node;
        node_id                m_steel_dragon_node;
        node_id                m_steel_teapot_node;
        node_id                m_glass_bunny_node;
        unsigned int           m_max_errors;
        unique_scene           m_scene;
        fps_camera_controller  m_fps_camera_controller;
        framerate_controller   m_framerate_controller;
        perspective_controller m_perspective_controller;
        float                  m_last_time;
        bool                   m_should_continue;
        float                  m_sim_rotation_speed;
        float                  m_sim_rotation_yaw;
        bool                   m_is_initialized;
    };

    real_time_engine::real_time_engine(unsigned int max_errors) :
        m_impl(std::make_unique<real_time_engine_impl>(max_errors)) {}

    real_time_engine::~real_time_engine() {}

    void real_time_engine::initialize()
    {
        m_impl->initialize();
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
