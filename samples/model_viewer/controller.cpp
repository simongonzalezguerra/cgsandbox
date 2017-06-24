#include "samples/model_viewer/controller.hpp"
#include "cgs/resource_database.hpp"
#include "cgs/resource_loader.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "cgs/scenegraph.hpp"
#include "cgs/renderer.hpp"
#include "glm/gtx/transform.hpp"
#include "cgs/control.hpp"
#include "glm/gtc/type_ptr.hpp"
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
  namespace {
    //-----------------------------------------------------------------------------------------------
    // Internal declarations
    //-----------------------------------------------------------------------------------------------
    cgs::resource_id s_resource_root = cgs::nresource;
    bool s_ok = false;
  } // anonymous namespace

  class controller::controller_impl
  {
  public:
    controller_impl() :
      m_view(0U),
      m_layer(cgs::nlayer),
      m_object_root(cgs::nnode),
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
      cgs::set_node_transform(m_layer, m_object_root, glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f}));
    }

    // Member variables
    cgs::view_id                  m_view;
    cgs::layer_id                 m_layer;
    cgs::node_id                  m_object_root;
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

    std::vector<cgs::mat_id> materials_out;
    std::vector<cgs::mesh_id> meshes_out;
    // cgs::resource_id s_resource_root = cgs::load_resources("../../resources/sponza/sponza.obj", &materials_out, &meshes_out);
    s_resource_root = cgs::load_resources("../../../resources/f-14D-super-tomcat/F-14D_SuperTomcatRotated.obj", &materials_out, &meshes_out);
    // The suzzane model doesn't have the texture path in its material information so we need to insert it manually
    // if (num_materials_out) {
    //   float color_diffuse[] = {0.0f, 0.0f, 0.0f};
    //   float color_spec[] = {0.0f, 0.0f, 0.0f};
    //   float smoothness = 0.0f;
    //   const char* texture_path = nullptr;
    //   cgs::get_material_properties(materials_out[0], color_diffuse, color_spec, smoothness, &texture_path);
    //  cgs::set_material_properties(materials_out[0], color_diffuse, color_spec, smoothness, "../../resources/suzanne/uvmap.png");
    // }
    if (s_resource_root == cgs::nresource) {
      s_ok = false;
    }

    bool window_ok = cgs::open_window(1920, 1080, true);
    if (!window_ok) {
      s_ok = false;
    }

    s_ok = true;
  }

  void controller::finalize()
  {
    cgs::close_window();
    cgs::detach_all_logstreams();
  }

  controller::controller() :
    m_impl(std::make_unique<controller::controller_impl>())
  {
    m_impl->m_view = cgs::add_view();
    m_impl->m_layer = cgs::add_layer(m_impl->m_view);
    // cgs::set_layer_projection_transform(m_impl->m_layer, glm::perspective(cgs::fov_to_fovy(fov, 1920.0f, 1080.0f), 1920.0f / 1080.0f, 0.1f, 100.0f));
    m_impl->m_object_root = cgs::add_node(m_impl->m_layer, cgs::root_node, s_resource_root);
    cgs::set_light_position(m_impl->m_layer, glm::vec3(4.0f, 4.0f, 4.0f));
    m_impl->m_last_time = cgs::get_time();


    m_impl->m_fps_camera_controller.set_layer(m_impl->m_layer);
    m_impl->m_fps_camera_controller.set_position(glm::vec3(-2.91f, 15.35f, 26.09f));
    m_impl->m_fps_camera_controller.set_yaw(-7.07f);
    m_impl->m_fps_camera_controller.set_pitch(-38.87f);
    m_impl->m_fps_camera_controller.set_speed(12.0f);
    m_impl->m_fps_camera_controller.set_mouse_speed(0.1f);

    m_impl->m_perspective_controller.set_layer(m_impl->m_layer);
    m_impl->m_perspective_controller.set_window_width(1920.0f);
    m_impl->m_perspective_controller.set_window_height(1080.0f);
    m_impl->m_perspective_controller.set_fov_speed(0.5f);
    m_impl->m_perspective_controller.set_fov(120.0f);

    m_impl->m_sim_rotation_speed = 2.0f;
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

    // Control framerate
    m_impl->m_framerate_controller.process(dt, events);

    return !m_impl->m_should_continue;
  }
} // namespace model_viewer
} // namespace samples
