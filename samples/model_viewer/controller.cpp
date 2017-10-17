#include "samples/model_viewer/controller.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "cgs/resource_database.hpp"
#include "cgs/resource_loader.hpp"
#include "glm/gtx/transform.hpp"
#include "cgs/opengl_driver.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cgs/scenegraph.hpp"
#include "cgs/renderer.hpp"
#include "cgs/control.hpp"
#include "cgs/system.hpp"
#include "cgs/utils.hpp"
#include "cgs/log.hpp"
#include "glm/glm.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace samples
{
namespace model_viewer
{
    namespace
    {
        cgs::resource_id make_floor_resource()
        {
            cgs::mesh_id floor_mesh = cgs::add_mesh();
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
            std::vector<cgs::vindex> indices = {0, 2, 1, 0, 3, 2};
            cgs::set_mesh_vertices(floor_mesh, vertices);
            cgs::set_mesh_normals(floor_mesh, normals);
            cgs::set_mesh_texture_coords(floor_mesh, texture_coords);
            cgs::set_mesh_indices(floor_mesh, indices);

            cgs::mat_id mat = cgs::add_material();
            cgs::set_material_diffuse_color(mat, glm::vec3(0.8f, 0.8f, 0.8f));
            cgs::set_material_specular_color(mat, glm::vec3(0.8f, 0.8f, 0.8f));
            cgs::set_material_reflectivity(mat, 0.0f);
            cgs::set_material_translucency(mat, 0.0f);
            cgs::set_material_texture_path(mat, "../../../resources/Wood/wood.png");

            cgs::resource_id floor_resource = cgs::add_resource(cgs::root_resource);
            cgs::set_resource_mesh(floor_resource, floor_mesh);
            cgs::set_resource_material(floor_resource,  mat);

            return floor_resource;
        }

        //-----------------------------------------------------------------------------------------------
        // Internal declarations
        //-----------------------------------------------------------------------------------------------
        cgs::resource_id s_floor_resource             = cgs::nresource;
        cgs::resource_id s_plane_resource             = cgs::nresource;
        cgs::resource_id s_teapot_resource            = cgs::nresource;
        cgs::resource_id s_bunny_resource             = cgs::nresource;
        cgs::resource_id s_dragon_resource            = cgs::nresource;
        cgs::cubemap_id  s_skybox_id                  = cgs::ncubemap;
        cgs::mat_id      s_diffuse_teapot_material    = cgs::nmat;
        cgs::mat_id      s_steel_material             = cgs::nmat;
        cgs::mat_id      s_glass_material             = cgs::nmat;
        bool             s_ok                         = false;
    } // anonymous namespace

    class controller::controller_impl
    {
    public:
        controller_impl() :
            m_view(0U),
            m_layer(cgs::nlayer),
            m_floor_node(cgs::nlayer),
            m_plane_node(cgs::nnode),
            m_diffuse_teapot_node(cgs::nnode),
            m_bunny_node(cgs::nnode),
            m_diffuse_dragon_node(cgs::nnode),
            m_steel_dragon_node(cgs::nnode),
            m_steel_teapot_node(cgs::nnode),
            m_glass_bunny_node(cgs::nnode),
            m_fps_camera_controller(),
            m_framerate_controller(),
            m_perspective_controller(),
            m_last_time(0.0f),
            m_should_continue(true),
            m_sim_rotation_speed(0.0f),
            m_sim_rotation_yaw(0.0f) {}

        void process_events(const std::vector<cgs::event>& events) {
            for (auto it = events.cbegin(); it != events.cend(); it++) {
                if (it->type == cgs::EVENT_KEY_PRESS && it->value == cgs::KEY_ESCAPE) {
                    m_should_continue = false;
                }
            }
        }

        void update_simulation(float dt)
        {
            m_sim_rotation_yaw += m_sim_rotation_speed * dt;
            cgs::set_node_transform(m_layer, m_plane_node, glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));

            cgs::set_node_transform(m_layer, m_steel_dragon_node, glm::translate(glm::vec3(65.0f, -3.0f, -20.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));

            cgs::set_node_transform(m_layer, m_glass_bunny_node, glm::translate(glm::vec3(47.0f, -5.0f, -20.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f})
                                           * glm::scale(glm::vec3(60.0f, 60.0f, 60.0f)));

            cgs::set_node_transform(m_layer, m_steel_teapot_node, glm::translate(glm::vec3(22.0f, -3.0f, 0.0f))
                                           * glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f})
                                           * glm::scale(glm::vec3(8.0f, 8.0f, 8.0f)));
        }

        // Member variables
        cgs::view_id                  m_view;
        cgs::layer_id                 m_layer;
        cgs::node_id                  m_floor_node;
        cgs::node_id                  m_plane_node;
        cgs::node_id                  m_diffuse_teapot_node;
        cgs::node_id                  m_bunny_node;
        cgs::node_id                  m_diffuse_dragon_node;
        cgs::node_id                  m_steel_dragon_node;
        cgs::node_id                  m_steel_teapot_node;
        cgs::node_id                  m_glass_bunny_node;
        cgs::fps_camera_controller    m_fps_camera_controller;
        cgs::framerate_controller     m_framerate_controller;
        cgs::perspective_controller   m_perspective_controller;
        float                         m_last_time;
        bool                          m_should_continue;
        float                         m_sim_rotation_speed;
        float                         m_sim_rotation_yaw;
    }; // class controller::controller_impl


    void controller::initialize()
    {
        cgs::log_init();
        cgs::attach_logstream(cgs::default_logstream_stdout_callback);
        cgs::attach_logstream(cgs::default_logstream_file_callback);
        cgs::resource_database_init();

        s_floor_resource = make_floor_resource();

        std::vector<cgs::mat_id> materials_out;
        std::vector<cgs::mesh_id> meshes_out;
        s_plane_resource = cgs::load_resources("../../../resources/f-14D-super-tomcat/F-14D_SuperTomcatRotated.obj", &materials_out, &meshes_out);
        if (s_plane_resource == cgs::nresource) {
            s_ok = false;
        }

        s_teapot_resource = cgs::load_resources("../../../resources/Teapot/Teapot.obj", &materials_out, &meshes_out);
        if (s_teapot_resource == cgs::nresource) {
            s_ok = false;
        }

        s_diffuse_teapot_material = cgs::add_material();
        cgs::set_material_diffuse_color(s_diffuse_teapot_material, glm::vec3(1.0f, 0.0f, 0.0f));
        cgs::set_material_specular_color(s_diffuse_teapot_material, glm::vec3(1.0f, 1.0f, 1.0f));
        cgs::set_material_smoothness(s_diffuse_teapot_material, 1.0f);

        s_steel_material = cgs::add_material();
        cgs::set_material_diffuse_color(s_steel_material, glm::vec3(0.8f, 0.8f, 0.8f));
        cgs::set_material_specular_color(s_steel_material, glm::vec3(0.8f, 0.8f, 0.8f));
        cgs::set_material_reflectivity(s_steel_material, 1.0f);
        cgs::set_material_translucency(s_steel_material, 0.0f);

        s_glass_material = cgs::add_material();
        cgs::set_material_diffuse_color(s_glass_material, glm::vec3(0.5f, 0.5f, 0.5f));
        cgs::set_material_specular_color(s_glass_material, glm::vec3(0.5f, 0.5f, 0.5f));
        cgs::set_material_reflectivity(s_glass_material, 0.0f);
        cgs::set_material_translucency(s_glass_material, 1.0f);
        cgs::set_material_refractive_index(s_glass_material, 1.52f); // Glass

        s_bunny_resource = cgs::load_resources("../../../resources/stanford-bunny/bun_zipper.ply", &materials_out, &meshes_out);
        if (s_teapot_resource == cgs::nresource) {
            s_ok = false;
        }
        cgs::mat_id bunny_material = cgs::add_material();
        cgs::set_material_diffuse_color(bunny_material, glm::vec3(0.0f, 0.2f, 0.4f));
        cgs::set_material_specular_color(bunny_material, glm::vec3(1.0f, 1.0f, 1.0f));
        cgs::set_material_smoothness(bunny_material, 1.0f);
        cgs::set_resource_material(s_bunny_resource, bunny_material);

        s_dragon_resource = cgs::load_resources("../../../resources/stanford-dragon2/dragon.obj", &materials_out, &meshes_out);
        if (s_dragon_resource == cgs::nresource) {
            s_ok = false;
        }
        cgs::mat_id dragon_material = cgs::add_material();
        cgs::set_material_diffuse_color(dragon_material, glm::vec3(0.05f, 0.5f, 0.0f));
        cgs::set_material_specular_color(dragon_material, glm::vec3(0.5f, 0.5f, 0.5f));
        cgs::set_material_smoothness(dragon_material, 1.0f);
        cgs::set_resource_material(s_dragon_resource, dragon_material);

        s_skybox_id = cgs::add_cubemap();
        std::vector<std::string> skybox_faces
        {
            "../../../resources/skybox_sea/right.jpg",
            "../../../resources/skybox_sea/left.jpg",
            "../../../resources/skybox_sea/top.jpg",
            "../../../resources/skybox_sea/bottom.jpg",
            "../../../resources/skybox_sea/back.jpg",
            "../../../resources/skybox_sea/front.jpg"
        };
        cgs::set_cubemap_faces(s_skybox_id, skybox_faces);

        bool window_ok = cgs::open_window(1920, 1080, false);
        if (!window_ok) {
            s_ok = false;
            return;
        }

        cgs::set_gl_driver(cgs::get_opengl_driver());
        cgs::initialize_renderer();

        s_ok = true;
    }

    void controller::finalize()
    {
        cgs::finalize_renderer();
        cgs::close_window();
        cgs::detach_all_logstreams();
    }

    controller::controller() :
        m_impl(std::make_unique<controller::controller_impl>())
    {
        m_impl->m_view = cgs::add_view();
        m_impl->m_layer = cgs::add_layer(m_impl->m_view);
        cgs::set_layer_skybox(m_impl->m_layer, s_skybox_id);
        cgs::set_directional_light_ambient_color(m_impl->m_layer,  glm::vec3(0.05f, 0.05f, 0.05f));
        cgs::set_directional_light_diffuse_color(m_impl->m_layer,  glm::vec3(0.5f, 0.5f, 0.5f));
        cgs::set_directional_light_specular_color(m_impl->m_layer, glm::vec3(0.5f, 0.5f, 0.5f));
        cgs::set_directional_light_direction(m_impl->m_layer, glm::vec3(0.0f, -1.0f, 0.0f));

        // Create the floor
        m_impl->m_floor_node = cgs::add_node(m_impl->m_layer, cgs::root_node, s_floor_resource);
        cgs::set_node_transform(m_impl->m_layer, m_impl->m_floor_node, glm::translate(glm::vec3(50.0f, -3.0f, -5.0f)) * glm::scale(glm::vec3(120.0f, 120.0f, 120.0f)));

        // Create the plane
        m_impl->m_plane_node = cgs::add_node(m_impl->m_layer, cgs::root_node, s_plane_resource);

        // Create the diffuse teapot
        m_impl->m_diffuse_teapot_node = cgs::add_node(m_impl->m_layer, cgs::root_node, s_teapot_resource);
        cgs::set_node_transform(m_impl->m_layer, m_impl->m_diffuse_teapot_node, glm::translate(glm::vec3(22.0f, -3.0f, -20.0f)) * glm::scale(glm::vec3(8.0f, 8.0f, 8.0f)));
        cgs::set_node_material(m_impl->m_layer, cgs::get_first_child_node(m_impl->m_layer, m_impl->m_diffuse_teapot_node), s_diffuse_teapot_material);

        // Create the bunny
        m_impl->m_bunny_node = cgs::add_node(m_impl->m_layer, cgs::root_node, s_bunny_resource);
        cgs::set_node_transform(m_impl->m_layer, m_impl->m_bunny_node, glm::translate(glm::vec3(47.0f, -5.0f, 0.0f)) * glm::scale(glm::vec3(60.0f, 60.0f, 60.0f)));

        // Create the diffuse dragon
        m_impl->m_diffuse_dragon_node = cgs::add_node(m_impl->m_layer, cgs::root_node, s_dragon_resource);
        cgs::set_node_transform(m_impl->m_layer, m_impl->m_diffuse_dragon_node, glm::translate(glm::vec3(65.0f, -3.0f, 0.0f)));
        
        // Create the steel dragon
        m_impl->m_steel_dragon_node = cgs::add_node(m_impl->m_layer, cgs::root_node, s_dragon_resource);
        cgs::set_node_material(m_impl->m_layer, cgs::get_first_child_node(m_impl->m_layer, m_impl->m_steel_dragon_node), s_steel_material);

        // Create the steel teapot
        m_impl->m_steel_teapot_node = cgs::add_node(m_impl->m_layer, cgs::root_node, s_teapot_resource);
        cgs::set_node_material(m_impl->m_layer, cgs::get_first_child_node(m_impl->m_layer, m_impl->m_steel_teapot_node), s_steel_material);

        // Create the glass bunny
        m_impl->m_glass_bunny_node = cgs::add_node(m_impl->m_layer, cgs::root_node, s_bunny_resource);
        cgs::set_node_material(m_impl->m_layer, m_impl->m_glass_bunny_node, s_glass_material);

        cgs::point_light_id point_light = cgs::add_point_light(m_impl->m_layer);
        cgs::set_point_light_position(m_impl->m_layer, point_light, glm::vec3(4.0f, 4.0f, 4.0f));
        cgs::set_point_light_ambient_color(m_impl->m_layer, point_light, glm::vec3(0.1f, 0.1f, 0.1f));
        cgs::set_point_light_diffuse_color(m_impl->m_layer, point_light, glm::vec3(1.0f, 1.0f, 1.0f));
        cgs::set_point_light_specular_color(m_impl->m_layer, point_light, glm::vec3(0.3f, 0.3f, 0.3f));
        cgs::set_point_light_constant_attenuation(m_impl->m_layer, point_light, 1.0f);
        cgs::set_point_light_linear_attenuation(m_impl->m_layer, point_light, 0.0f); // Be careful with this parameter, it can dim your point light pretty fast
        cgs::set_point_light_quadratic_attenuation(m_impl->m_layer, point_light, 0.0f); // Be careful with this parameter, it can dim your point light pretty fast

        m_impl->m_last_time = cgs::get_time();

        m_impl->m_fps_camera_controller.set_layer(m_impl->m_layer);
        m_impl->m_fps_camera_controller.set_position(glm::vec3(-14.28f, 13.71f, 29.35f));
        m_impl->m_fps_camera_controller.set_yaw(-41.50f);
        m_impl->m_fps_camera_controller.set_pitch(-20.37f);
        m_impl->m_fps_camera_controller.set_speed(40.0f);
        m_impl->m_fps_camera_controller.set_mouse_speed(0.1f);

        m_impl->m_perspective_controller.set_layer(m_impl->m_layer);
        m_impl->m_perspective_controller.set_window_width(1920.0f);
        m_impl->m_perspective_controller.set_window_height(1080.0f);
        m_impl->m_perspective_controller.set_fov_speed(0.5f);
        m_impl->m_perspective_controller.set_fov_radians(glm::radians(70.0f));
        m_impl->m_perspective_controller.set_near(0.1f);
        m_impl->m_perspective_controller.set_far(500.0f);

        m_impl->m_sim_rotation_speed = 1.5f;
        m_impl->m_sim_rotation_yaw = 0.0f;
    }

    controller::~controller()
    {
        m_impl->m_framerate_controller.log_stats();
    }


    bool controller::process()
    {
        if (!s_ok) return true;

        // Delta time for simulation
        float current_time = cgs::get_time();
        float dt = float(current_time - m_impl->m_last_time);
        m_impl->m_last_time = current_time;

        std::vector<cgs::event> events = cgs::poll_events();

        // Update the simulation with time passed since last frame
        m_impl->update_simulation(dt);

        // Check for escape key
        m_impl->process_events(events);

        // Control camera
        m_impl->m_fps_camera_controller.process(dt, events);

        // Control projection (update fov)
        m_impl->m_perspective_controller.process(dt, events);

        // Render the scene
        cgs::render(m_impl->m_view);
        cgs::swap_buffers();

        // Control framerate
        m_impl->m_framerate_controller.process(dt, events);

        return !m_impl->m_should_continue;
    }
} // namespace model_viewer
} // namespace samples
