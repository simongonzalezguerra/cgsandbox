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
            m_materials(),
            m_meshes(),
            m_resources(),
            m_cubemaps(),
            m_root_node(nnode),
            m_floor_node(nnode),
            m_plane_node(nnode),
            m_diffuse_teapot_node(nnode),
            m_bunny_node(nnode),
            m_diffuse_dragon_node(nnode),
            m_steel_dragon_node(nnode),
            m_steel_teapot_node(nnode),
            m_glass_bunny_node(nnode),
            m_max_errors(max_errors),
            m_view(0U),
            m_scene(nscene),
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

            unique_resource floor_resource;
            unique_material floor_material;
            unique_mesh floor_mesh;
            make_floor_resource(&floor_resource, &floor_material, &floor_mesh);

            material_vector materials;
            mesh_vector meshes;

            unique_resource plane_resource;
            load_resources("../../../resources/f-14D-super-tomcat/F-14D_SuperTomcatRotated.obj", &plane_resource, &materials, &meshes);

            unique_resource teapot_resource;
            load_resources("../../../resources/Teapot/Teapot.obj", &teapot_resource, &materials, &meshes);

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

            unique_resource bunny_resource;
            load_resources("../../../resources/stanford-bunny/bun_zipper.ply", &bunny_resource, &materials, &meshes);

            unique_material bunny_material = make_material();
            set_material_diffuse_color(bunny_material.get(), glm::vec3(0.0f, 0.2f, 0.4f));
            set_material_specular_color(bunny_material.get(), glm::vec3(1.0f, 1.0f, 1.0f));
            set_material_smoothness(bunny_material.get(), 1.0f);
            set_resource_material(bunny_resource.get(), bunny_material.get());

            unique_resource dragon_resource;
            load_resources("../../../resources/stanford-dragon2/dragon.obj", &dragon_resource, &materials, &meshes);

            unique_material dragon_material = make_material();
            set_material_diffuse_color(dragon_material.get(), glm::vec3(0.05f, 0.5f, 0.0f));
            set_material_specular_color(dragon_material.get(), glm::vec3(0.5f, 0.5f, 0.5f));
            set_material_smoothness(dragon_material.get(), 1.0f);
            set_resource_material(dragon_resource.get(), dragon_material.get());

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

            open_window(1920, 1080, false);

            set_gl_driver(get_opengl_driver());

            initialize_renderer();

            m_view = add_view();
            m_scene = add_scene(m_view);
            set_scene_skybox(m_scene, skybox.get());
            set_directional_light_ambient_color(m_scene,  glm::vec3(0.05f, 0.05f, 0.05f));
            set_directional_light_diffuse_color(m_scene,  glm::vec3(0.5f, 0.5f, 0.5f));
            set_directional_light_specular_color(m_scene, glm::vec3(0.5f, 0.5f, 0.5f));
            set_directional_light_direction(m_scene, glm::vec3(0.0f, -1.0f, 0.0f));

            // Create the floor
            m_root_node = get_scene_root_node(m_scene);
            m_floor_node = add_node(m_scene, m_root_node, floor_resource.get());
            set_node_transform(m_scene, m_floor_node, glm::translate(glm::vec3(50.0f, -3.0f, -5.0f)) * glm::scale(glm::vec3(120.0f, 120.0f, 120.0f)));

            // Create the plane
            m_plane_node = add_node(m_scene, m_root_node, plane_resource.get());
        
            // Create the diffuse teapot
            m_diffuse_teapot_node = add_node(m_scene, m_root_node, teapot_resource.get());
            set_node_transform(m_scene, m_diffuse_teapot_node, glm::translate(glm::vec3(22.0f, -3.0f, -20.0f)) * glm::scale(glm::vec3(8.0f, 8.0f, 8.0f)));
            set_node_material(m_scene, get_first_child_node(m_scene, m_diffuse_teapot_node), diffuse_teapot_material.get());
        
            // Create the bunny
            m_bunny_node = add_node(m_scene, m_root_node, bunny_resource.get());
            set_node_transform(m_scene, m_bunny_node, glm::translate(glm::vec3(47.0f, -5.0f, 0.0f)) * glm::scale(glm::vec3(60.0f, 60.0f, 60.0f)));
        
            // Create the diffuse dragon
            m_diffuse_dragon_node = add_node(m_scene, m_root_node, dragon_resource.get());
            set_node_transform(m_scene, m_diffuse_dragon_node, glm::translate(glm::vec3(65.0f, -3.0f, 0.0f)));

            // Create the steel dragon
            m_steel_dragon_node = add_node(m_scene, m_root_node, dragon_resource.get());
            set_node_material(m_scene, get_first_child_node(m_scene, m_steel_dragon_node), steel_material.get());

            // Create the steel teapot
            m_steel_teapot_node = add_node(m_scene, m_root_node, teapot_resource.get());
            set_node_material(m_scene, get_first_child_node(m_scene, m_steel_teapot_node), steel_material.get());

            // Create the glass bunny
            m_glass_bunny_node = add_node(m_scene, m_root_node, bunny_resource.get());
            set_node_material(m_scene, m_glass_bunny_node, glass_material.get());

            point_light_id point_light = add_point_light(m_scene);
            set_point_light_position(m_scene, point_light, glm::vec3(4.0f, 4.0f, 4.0f));
            set_point_light_ambient_color(m_scene, point_light, glm::vec3(0.1f, 0.1f, 0.1f));
            set_point_light_diffuse_color(m_scene, point_light, glm::vec3(1.0f, 1.0f, 1.0f));
            set_point_light_specular_color(m_scene, point_light, glm::vec3(0.3f, 0.3f, 0.3f));
            set_point_light_constant_attenuation(m_scene, point_light, 1.0f);
            set_point_light_linear_attenuation(m_scene, point_light, 0.0f); // Be careful with this parameter, it can dim your point light pretty fast
            set_point_light_quadratic_attenuation(m_scene, point_light, 0.0f); // Be careful with this parameter, it can dim your point light pretty fast

            m_last_time = get_time();

            m_fps_camera_controller.set_scene(m_scene);
            m_fps_camera_controller.set_position(glm::vec3(-14.28f, 13.71f, 29.35f));
            m_fps_camera_controller.set_yaw(-41.50f);
            m_fps_camera_controller.set_pitch(-20.37f);
            m_fps_camera_controller.set_speed(40.0f);
            m_fps_camera_controller.set_mouse_speed(0.1f);

            m_perspective_controller.set_scene(m_scene);
            m_perspective_controller.set_window_width(1920.0f);
            m_perspective_controller.set_window_height(1080.0f);
            m_perspective_controller.set_fov_speed(0.5f);
            m_perspective_controller.set_fov_radians(glm::radians(70.0f));
            m_perspective_controller.set_near(0.1f);
            m_perspective_controller.set_far(500.0f);

            m_sim_rotation_speed = 1.5f;
            m_sim_rotation_yaw = 0.0f;

            m_materials.insert(m_materials.end(), make_move_iterator(materials.begin()), make_move_iterator(materials.end()));
            m_materials.push_back(std::move(diffuse_teapot_material));
            m_materials.push_back(std::move(floor_material));
            m_materials.push_back(std::move(steel_material));
            m_materials.push_back(std::move(glass_material));
            m_materials.push_back(std::move(bunny_material));
            m_materials.push_back(std::move(dragon_material));
            m_meshes.insert(m_meshes.end(), make_move_iterator(meshes.begin()), make_move_iterator(meshes.end()));
            m_meshes.push_back(std::move(floor_mesh));
            m_resources.push_back(std::move(floor_resource));
            m_resources.push_back(std::move(plane_resource));
            m_resources.push_back(std::move(teapot_resource ));
            m_resources.push_back(std::move(bunny_resource));
            m_resources.push_back(std::move(dragon_resource ));
            m_cubemaps.push_back(std::move(skybox));
            m_is_initialized = true;
        }

        void finalize()
        {
            if (!m_is_initialized) return;

            try {
                log(LOG_LEVEL_DEBUG, "real_time_engine: finalizing application");
                m_framerate_controller.log_stats();
                finalize_renderer();
                close_window();
                m_cubemaps.clear();
                m_resources.clear();
                m_meshes.clear();
                m_materials.clear();
                m_is_initialized = false;
            } catch(...) {
                log(LOG_LEVEL_DEBUG, "real_time_engine: exception during finalization");
            }            
        }

        void make_floor_resource(unique_resource* res, unique_material* mat, unique_mesh* mesh)
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
            *res = std::move(floor_resource);
            *mat = std::move(floor_mat);
            *mesh = std::move(floor_mesh);
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
            set_node_transform(m_scene, m_plane_node, glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));

            set_node_transform(m_scene, m_steel_dragon_node, glm::translate(glm::vec3(65.0f, -3.0f, -20.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));

            set_node_transform(m_scene, m_glass_bunny_node, glm::translate(glm::vec3(47.0f, -5.0f, -20.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f})
                                           * glm::scale(glm::vec3(60.0f, 60.0f, 60.0f)));

            set_node_transform(m_scene, m_steel_teapot_node, glm::translate(glm::vec3(22.0f, -3.0f, 0.0f))
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

        material_vector        m_materials;
        mesh_vector            m_meshes;
        resource_vector        m_resources;
        cubemap_vector         m_cubemaps;
        node_id                m_root_node;
        node_id                m_floor_node;
        node_id                m_plane_node;
        node_id                m_diffuse_teapot_node;
        node_id                m_bunny_node;
        node_id                m_diffuse_dragon_node;
        node_id                m_steel_dragon_node;
        node_id                m_steel_teapot_node;
        node_id                m_glass_bunny_node;
        unsigned int           m_max_errors;
        view_id                m_view;
        scene_id               m_scene;
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
                n_errors++;
                if (n_errors == 100) {
                    log(LOG_LEVEL_ERROR, "real_time_engine: too many errors, stoping frame loop");
                    throw std::logic_error("");
                }
            }
        } while (!done);
    }

    void real_time_engine::finalize()
    {
        m_impl->finalize();
    }
}
