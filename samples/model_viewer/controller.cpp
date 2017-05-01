#include "samples/model_viewer/controller.hpp"
#include "glsandbox/resource_database.hpp"
#include "glsandbox/resource_loader.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glsandbox/scenegraph.hpp"
#include "glsandbox/renderer.hpp"
#include "glm/gtx/transform.hpp"
#include "glsandbox/control.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glsandbox/utils.hpp"
#include "glsandbox/log.hpp"
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
    glsandbox::resource_id s_resource_root = glsandbox::nresource;
    bool s_ok = false;
  } // anonymous namespace

  class controller::controller_impl
  {
  public:
    controller_impl() :
      m_view(0U),
      m_layer(glsandbox::nlayer),
      m_object_root(glsandbox::nnode),
      m_fps_camera_controller(),
      m_framerate_controller(),
      m_perspective_controller(),
      m_last_time(0.0f),
      m_should_continue(true),
      m_sim_rotation_speed(0.0f),
      m_sim_rotation_yaw(0.0f) {}

    void process_events(const std::vector<glsandbox::event>& events) {
      for (auto it = events.cbegin(); it != events.cend(); it++) {
        if (it->type == glsandbox::EVENT_KEY_PRESS && it->value == glsandbox::KEY_ESCAPE) {
          m_should_continue = false;
        }
      }
    }

    void update_simulation(float dt)
    {
      m_sim_rotation_yaw += m_sim_rotation_speed * dt;
      glm::mat4 rot = glm::rotate(m_sim_rotation_yaw, glm::vec3{0.0f, 1.0f, 0.0f});
      glsandbox::set_node_transform(m_layer, m_object_root, glm::value_ptr(rot));
    }

    // Member variables
    glsandbox::view_id                  m_view;
    glsandbox::layer_id                 m_layer;
    glsandbox::node_id                  m_object_root;
    glsandbox::fps_camera_controller    m_fps_camera_controller;
    glsandbox::framerate_controller     m_framerate_controller;
    glsandbox::perspective_controller   m_perspective_controller;
    float                               m_last_time;
    bool                                m_should_continue;
    float                               m_sim_rotation_speed;
    float                               m_sim_rotation_yaw;
  }; // class controller::controller_impl

  void controller::initialize()
  {
    glsandbox::log_init();
    glsandbox::attach_logstream(glsandbox::default_logstream_stdout_callback);
    glsandbox::attach_logstream(glsandbox::default_logstream_file_callback);
    glsandbox::resource_database_init();

    const glsandbox::mat_id* materials_out = nullptr;
    std::size_t num_materials_out = 0U;
    const glsandbox::mesh_id* meshes_out = nullptr;
    std::size_t num_meshes_out = 0U;
    // glsandbox::resource_id s_resource_root = glsandbox::load_resources("../../resources/sponza/sponza.obj", &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
    s_resource_root = glsandbox::load_resources("../../../resources/f-14D-super-tomcat/F-14D_SuperTomcatRotated.obj", &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
    // The suzzane model doesn't have the texture path in its material information so we need to insert it manually
    // if (num_materials_out) {
    //   float color_diffuse[] = {0.0f, 0.0f, 0.0f};
    //   float color_spec[] = {0.0f, 0.0f, 0.0f};
    //   float smoothness = 0.0f;
    //   const char* texture_path = nullptr;
    //   glsandbox::get_material_properties(materials_out[0], color_diffuse, color_spec, smoothness, &texture_path);
    //  glsandbox::set_material_properties(materials_out[0], color_diffuse, color_spec, smoothness, "../../resources/suzanne/uvmap.png");
    // }
    if (s_resource_root == glsandbox::nresource) {
      s_ok = false;
    }

    bool window_ok = glsandbox::open_window(1920, 1080, true);
    if (!window_ok) {
      s_ok = false;
    }

    s_ok = true;
  }

  void controller::finalize()
  {
    glsandbox::close_window();
    glsandbox::detach_all_logstreams();
  }

  controller::controller() :
    m_impl(std::make_unique<controller::controller_impl>())
  {
    m_impl->m_view = glsandbox::add_view();
    m_impl->m_layer = glsandbox::add_layer(m_impl->m_view);
    // glsandbox::set_layer_projection_transform(m_impl->m_layer, glm::perspective(glsandbox::fov_to_fovy(fov, 1920.0f, 1080.0f), 1920.0f / 1080.0f, 0.1f, 100.0f));
    m_impl->m_object_root = glsandbox::add_node(m_impl->m_layer, glsandbox::root_node, s_resource_root);
    glsandbox::light_data ld;
    ld.mposition[0] = 4.0f;
    ld.mposition[1] = 4.0f;
    ld.mposition[2] = 4.0f;
    glsandbox::set_light_data(m_impl->m_layer, &ld);
    m_impl->m_last_time = glsandbox::get_time();


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

  controller::~controller() {}


  bool controller::process()
  {
    if (!s_ok) return true;

    // Delta time for simulation
    float current_time = glsandbox::get_time();
    float dt = float(current_time - m_impl->m_last_time);
    m_impl->m_last_time = current_time;

    std::vector<glsandbox::event> events = glsandbox::poll_events();

    // Update the simulation with time passed since last frame
    m_impl->update_simulation(dt);

    // Check for escape key
    m_impl->process_events(events);

    // Control camera
    m_impl->m_fps_camera_controller.process(dt, events);

    // Control projection (update fov)
    m_impl->m_perspective_controller.process(dt, events);

    // Render the scene
    glsandbox::render(m_impl->m_view);

    // Control framerate
    m_impl->m_framerate_controller.process(dt, events);

    if (!m_impl->m_should_continue) {
      m_impl->m_framerate_controller.log_stats();
    }
    return !m_impl->m_should_continue;
  }
} // namespace model_viewer
} // namespace samples