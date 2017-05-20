#include "cgs/resource_database.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "cgs/renderer.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cgs/log.hpp"
#include "glm/glm.hpp"
#include "FreeImage.h"
#include "GL/glew.h"
#include "glfw3.h"

#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstring>
#include <vector>
#include <string>
#include <queue>
#include <map>

namespace cgs
{
  namespace
  {
    //---------------------------------------------------------------------------------------------
    // Internal declarations
    //---------------------------------------------------------------------------------------------
    struct mesh_context
    {
      GLuint mpositions_vbo_id;
      GLuint muvs_vbo_id;
      GLuint mnormals_vbo_id;
      GLuint mindices_vbo_id;
      GLuint mtexture_id;
      GLuint mnum_indices;
    };

    typedef std::map<mesh_id, mesh_context> mesh_context_map;
    typedef std::map<int, event_type> key_event_type_map;
    typedef key_event_type_map::iterator event_type_it;
    typedef std::map<int, std::string> error_code_map;

    //---------------------------------------------------------------------------------------------
    // Internal data structures
    //---------------------------------------------------------------------------------------------
    const char* vertex_shader = R"glsl(
      #version 330 core

      // Input vertex data, different for all executions of this shader.
      layout(location = 0) in vec3 vertexPosition_modelspace;
      layout(location = 1) in vec2 vertexUV;
      layout(location = 2) in vec3 vertexNormal_modelspace;

      // Output data ; will be interpolated for each fragment.
      out vec2 UV;
      out vec3 Position_worldspace;
      out vec3 Normal_cameraspace;
      out vec3 EyeDirection_cameraspace;
      out vec3 LightDirection_cameraspace;

      // Values that stay constant for the whole mesh.
      uniform mat4 mvp;
      uniform mat4 V;
      uniform mat4 M;
      uniform vec3 LightPosition_worldspace;

      void main(){

              // Output position of the vertex, in clip space : mvp * position
              gl_Position =  mvp * vec4(vertexPosition_modelspace,1);
              
              // Position of the vertex, in worldspace : M * position
              Position_worldspace = (M * vec4(vertexPosition_modelspace,1)).xyz;
              
              // Vector that goes from the vertex to the camera, in camera space.
              // In camera space, the camera is at the origin (0,0,0).
              vec3 vertexPosition_cameraspace = ( V * M * vec4(vertexPosition_modelspace,1)).xyz;
              EyeDirection_cameraspace = vec3(0,0,0) - vertexPosition_cameraspace;

              // Vector that goes from the vertex to the light, in camera space. M is ommited because it's identity.
              vec3 LightPosition_cameraspace = ( V * vec4(LightPosition_worldspace,1)).xyz;
              LightDirection_cameraspace = LightPosition_cameraspace + EyeDirection_cameraspace;
              
              // Normal of the the vertex, in camera space
              Normal_cameraspace = ( V * M * vec4(vertexNormal_modelspace,0)).xyz; // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
              
              // UV of the vertex. No special space for this one.
              UV = vertexUV;
      }
    )glsl";

    const char* fragment_shader = R"glsl(
      #version 330 core

      // Interpolated values from the vertex shaders
      in vec2 UV;
      in vec3 Position_worldspace;
      in vec3 Normal_cameraspace;
      in vec3 EyeDirection_cameraspace;
      in vec3 LightDirection_cameraspace;

      // Ouput data
      out vec3 color;

      // Values that stay constant for the whole mesh.
      uniform sampler2D myTextureSampler;
      uniform mat4 MV;
      uniform vec3 LightPosition_worldspace;

      void main(){
        // Light emission properties
        // You probably want to put them as uniforms
        vec3 LightColor = vec3(1,1,1);
        float LightPower = 50.0f;

        // Material properties
        vec3 MaterialDiffuseColor = texture( myTextureSampler, UV ).rgb;
        vec3 MaterialAmbientColor = vec3(0.1,0.1,0.1) * MaterialDiffuseColor;
        vec3 MaterialSpecularColor = vec3(0.3,0.3,0.3);

        // Distance to the light
        float distance = length( LightPosition_worldspace - Position_worldspace );

        // Normal of the computed fragment, in camera space
        vec3 n = normalize( Normal_cameraspace );
        // Direction of the light (from the fragment to the light)
        vec3 l = normalize( LightDirection_cameraspace );
        // Cosine of the angle between the normal and the light direction, 
        // clamped above 0
        //  - light is at the vertical of the triangle -> 1
        //  - light is perpendicular to the triangle -> 0
        //  - light is behind the triangle -> 0
        float cosTheta = clamp( dot( n,l ), 0,1 );

        // Eye vector (towards the camera)
        vec3 E = normalize(EyeDirection_cameraspace);
        // Direction in which the triangle reflects the light
        vec3 R = reflect(-l,n);
        // Cosine of the angle between the Eye vector and the Reflect vector,
        // clamped to 0
        //  - Looking into the reflection -> 1
        //  - Looking elsewhere -> < 1
        float cosAlpha = clamp( dot( E,R ), 0,1 );

        color = 
                // Ambient : simulates indirect lighting
                MaterialAmbientColor +
                // Diffuse : "color" of the object
                MaterialDiffuseColor * LightColor * LightPower * cosTheta / (distance*distance) +
                // Specular : reflective highlight, like a mirror
                MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha,5) / (distance*distance);

      }
    )glsl";

    GLFWwindow* window = nullptr;
    bool ok = false;
    mesh_context_map mesh_contexts;
    GLuint vertex_array_id = 0U;
    GLuint vertex_shader_id = 0U;
    GLuint fragment_shader_id = 0U;
    GLuint program_id = 0U;
    GLuint matrix_id = 0U;
    GLuint view_matrix_id = 0U;
    GLuint model_matrix_id = 0U;
    GLuint texture_sampler_id = 0U;
    GLuint light_id = 0U;
    std::vector<event> events;
    key_event_type_map event_types; // map from GLFW key action value to event_type value
    error_code_map error_codes;     // map from GLFW error codes to their names
    float last_mouse_x = 0.0f;
    float last_mouse_y = 0.0f;

    //---------------------------------------------------------------------------------------------
    // Helper functions
    //---------------------------------------------------------------------------------------------
    bool load_shaders(const char* vertex_file_path, const char* fragment_file_path)
    {
      // Create the shaders
      vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
      fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

      GLint result = GL_FALSE;
      int info_log_length = 0;

      // Compile Vertex Shader
      log(LOG_LEVEL_DEBUG, std::string("load_shaders: compiling shader: ") + vertex_file_path);
      glShaderSource(vertex_shader_id, 1, &vertex_shader, nullptr);
      glCompileShader(vertex_shader_id);

      // Check Vertex Shader
      glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
      glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
      if (result == GL_FALSE){
        std::vector<char> vertex_shader_error_message(info_log_length + 1);
        glGetShaderInfoLog(vertex_shader_id, info_log_length, nullptr, &vertex_shader_error_message[0]);
        log(LOG_LEVEL_ERROR, &vertex_shader_error_message[0]);
        return false;
      }
      log(LOG_LEVEL_DEBUG, "vertex shader compiled successfully");

      // Compile Fragment Shader
      log(LOG_LEVEL_DEBUG, std::string("load_shaders: compiling shader: ") + fragment_file_path);
      glShaderSource(fragment_shader_id, 1, &fragment_shader , nullptr);
      glCompileShader(fragment_shader_id);

      // Check Fragment Shader
      glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
      glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
      if (result == GL_FALSE) {
        std::vector<char> fragment_shader_error_message(info_log_length + 1);
        glGetShaderInfoLog(fragment_shader_id, info_log_length, nullptr, &fragment_shader_error_message[0]);
        log(LOG_LEVEL_ERROR, &fragment_shader_error_message[0]);
        return false;
      }
      log(LOG_LEVEL_DEBUG, "fragment shader compiled successfully");

      // Link the program
      log(LOG_LEVEL_DEBUG, "load_shaders: linking program");
      program_id = glCreateProgram();
      glAttachShader(program_id, vertex_shader_id);
      glAttachShader(program_id, fragment_shader_id);
      glLinkProgram(program_id);

      // Check the program
      glGetProgramiv(program_id, GL_LINK_STATUS, &result);
      glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
      if (result == GL_FALSE) {
        std::vector<char> program_error_message(info_log_length + 1);
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &program_error_message[0]);
        log(LOG_LEVEL_ERROR, &program_error_message[0]);
        return false;
      }
      log(LOG_LEVEL_DEBUG, "program linked successfully");
      
      glDetachShader(program_id, vertex_shader_id);
      glDetachShader(program_id, fragment_shader_id);
      
      glDeleteShader(vertex_shader_id);
      glDeleteShader(fragment_shader_id);

      return true;
    }

    void fill_error_code_map()
    {
      if (error_codes.empty()) {
        error_codes[GLFW_NOT_INITIALIZED]     = "GLFW_NOT_INITIALIZED";
        error_codes[GLFW_NO_CURRENT_CONTEXT]  = "GLFW_NO_CURRENT_CONTEXT";
        error_codes[GLFW_INVALID_ENUM]        = "GLFW_INVALID_ENUM";
        error_codes[GLFW_INVALID_VALUE]       = "GLFW_INVALID_VALUE";
        error_codes[GLFW_OUT_OF_MEMORY]       = "GLFW_OUT_OF_MEMORY";
        error_codes[GLFW_API_UNAVAILABLE]     = "GLFW_API_UNAVAILABLE";
        error_codes[GLFW_VERSION_UNAVAILABLE] = "GLFW_VERSION_UNAVAILABLE";
        error_codes[GLFW_PLATFORM_ERROR]      = "GLFW_PLATFORM_ERROR";
        error_codes[GLFW_FORMAT_UNAVAILABLE]  = "GLFW_FORMAT_UNAVAILABLE";
        //error_codes[GLFW_NO_WINDOW_CONTEXT]   = "GLFW_NO_WINDOW_CONTEXT"; Not defined in our version of GLFW
      }
    }

    void fill_event_type_map() {
      if (event_types.empty()) {
        event_types[GLFW_PRESS]   = EVENT_KEY_PRESS;
        event_types[GLFW_REPEAT]  = EVENT_KEY_HOLD;
        event_types[GLFW_RELEASE] = EVENT_KEY_RELEASE;
      }
    }

    void key_callback(GLFWwindow*, int key, int, int action, int)
    {
      if (ok && window) {
        // Our key code is the same value that GLFW gives us. This works because we have defined our
        // keycode constants with the same values as GLFW's constants. If we update GLFW and its
        // key constants change, we need to update our keycode constants accordingly.
        key_code key_code = key;

        // Translate GLFW key action value to an event type
        fill_event_type_map();
        event_type event_type = EVENT_KEY_PRESS;
        event_type_it eit = event_types.find(action);
        if (eit != event_types.end()) {
          event_type = eit->second;
          event e = {event_type, key_code, 0.0f, 0.0f, 0.0f, 0.0f};
          events.push_back(e);
        }
      }
    }

    void mouse_move_callback(GLFWwindow*, double x, double y)
    {
      if (ok && window) {
        event e{EVENT_MOUSE_MOVE, KEY_UNKNOWN, (float) x, (float) y, (float) x - last_mouse_x, (float) y - last_mouse_y};
        events.push_back(e);
        last_mouse_x = x;
        last_mouse_y = y;

        std::ostringstream oss;
        oss << "mouse_move_callback: new delta_mouse_x: " << std::fixed << std::setprecision(2)
            << e.delta_mouse_x << ", new delta_mouse_y: " << e.delta_mouse_y;
        cgs::log(cgs::LOG_LEVEL_DEBUG, oss.str());
      }
    }

    void error_callback(int error_code, const char* description)
    {
      fill_error_code_map();
      log(LOG_LEVEL_ERROR, "GLFW error callback");
      std::ostringstream oss;
      oss << "GLFW error " << error_codes[error_code] << ", " << description;
      log(LOG_LEVEL_ERROR, oss.str());
    }

    // Path must be relative to the curent directory
    GLuint load_texture(const char* path)
    {
      log(LOG_LEVEL_DEBUG, "load_texture: loading texture from path " + std::string(path));

      FREE_IMAGE_FORMAT format = FreeImage_GetFileType(path, 0); // automatocally detects the format(from over 20 formats!)
      FIBITMAP* image = FreeImage_Load(format, path);
      unsigned int width = FreeImage_GetWidth(image);
      unsigned int height = FreeImage_GetHeight(image);
      unsigned int depth = FreeImage_GetBPP(image) / 8;
      unsigned int red_mask = FreeImage_GetRedMask(image);
      unsigned int green_mask = FreeImage_GetGreenMask(image);
      unsigned int blue_mask = FreeImage_GetBlueMask(image);
      char* fi_data = (char*) FreeImage_GetBits(image);
      std::vector<char> data(depth * width * height);
      std::memcpy(&data[0], fi_data, depth * width * height);
      FreeImage_Unload(image);

      std::ostringstream oss;
      oss << "load_texture: loaded image of (width x height) = " << width << " x " << height
          << " pixels, " << depth * width * height << " bytes in total" << ", bytes by pixel: " << depth
          << std::hex << ", red mask: " << red_mask << ", green mask: " << green_mask << ", blue mask: " << blue_mask;
      log(LOG_LEVEL_DEBUG, oss.str());

      // Create one OpenGL texture
      GLuint textureID;
      glGenTextures(1, &textureID);
      // "Bind" the newly created texture : all future texture functions will modify this texture
      glBindTexture(GL_TEXTURE_2D, textureID);

      // Give the image to OpenGL
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
      // OpenGL has now copied the data. Free our own version

      // Trilinear filtering.
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
      glGenerateMipmap(GL_TEXTURE_2D);

      // Return the ID of the texture we just created
      return textureID;
    }

    void render_mesh(mesh_id mid, const glm::vec3 light_pos, const glm::mat4& model_matrix, const glm::mat4& view_matrix, const glm::mat4& projection_matrix)
    {
      // Get buffer ids previously created for the mesh
      auto& context = mesh_contexts[mid];

      glm::mat4 mvp = projection_matrix * view_matrix * model_matrix;

      // Send our transformation to the currently bound shader, 
      // in the "mvp" uniform
      glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);
      glUniformMatrix4fv(model_matrix_id, 1, GL_FALSE, &model_matrix[0][0]);
      glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, &view_matrix[0][0]);
      glUniform3f(light_id, light_pos.x, light_pos.y, light_pos.z);

      // Bind our texture in texture Unit 0
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, context.mtexture_id);
      // Set our "myTextureSampler" sampler to user texture Unit 0
      glUniform1i(texture_sampler_id, 0);

      // 1rst attribute buffer : vertices
      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, context.mpositions_vbo_id);
      glVertexAttribPointer(
        0,                  // attribute
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
      );

      // 2nd attribute buffer : UVs
      glEnableVertexAttribArray(1);
      glBindBuffer(GL_ARRAY_BUFFER, context.muvs_vbo_id);
      glVertexAttribPointer(
        1,                                // attribute
        2,                                // size
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
      );

      // 3rd attribute buffer : normals
      glEnableVertexAttribArray(2);
      glBindBuffer(GL_ARRAY_BUFFER, context.mnormals_vbo_id);
      glVertexAttribPointer(
        2,                                // attribute
        3,                                // size
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
      );

      // Index buffer
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context.mindices_vbo_id);

      // Draw the triangles !
      glDrawElements(
        GL_TRIANGLES,      // mode
        context.mnum_indices, // count
        GL_UNSIGNED_SHORT,   // type
        (void*)0           // element array buffer offset
      );

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
      glDisableVertexAttribArray(2);
    }

    void render_node(layer_id l, node_id n, const glm::vec3& light_pos, const glm::mat4& model_matrix, const glm::mat4& view_matrix, const glm::mat4& projection_matrix)
    {
      const mesh_id* meshes = nullptr;
      std::size_t num_meshes = 0U;
      get_node_meshes(l, n, &meshes, &num_meshes);
      // For each mesh in the node
      for (std::size_t i = 0U; i < num_meshes; ++i) {
        if (mesh_contexts.find(meshes[i]) != mesh_contexts.end()) {
          render_mesh(meshes[i], light_pos, model_matrix, view_matrix, projection_matrix);
        }
      }
    }
  } // anonymous namespace

  //-----------------------------------------------------------------------------------------------
  // Public functions
  //-----------------------------------------------------------------------------------------------
  bool open_window(std::size_t width, std::size_t height, bool fullscreen)
  {
    // Initialise GLFW
    if(!glfwInit())
    {
      log(LOG_LEVEL_ERROR, "open_window: failed to initialize GLFW.");
      return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(width, height, "cgs", fullscreen? glfwGetPrimaryMonitor() : nullptr, nullptr);
    if(window == nullptr){
      log(LOG_LEVEL_ERROR, "open_window: failed to open GLFW window. If you have an Intel GPU prior to HD 4000, they are not OpenGL 3.3 compatible.");
      glfwTerminate();
      return false;
    }
    ok = true;
    glfwMakeContextCurrent(window);

    // Register callbacks
    glfwSetErrorCallback(error_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_move_callback);

    // Disable vertical synchronization (results in a noticeable fps gain). This needs to be done
    // after calling glfwMakeContextCurrent, since it acts on the current context, and the context
    // created with glfwCreateWindow is not current until we make it explicitly so with
    // glfwMakeContextCurrent
    glfwSwapInterval(0);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
      log(LOG_LEVEL_ERROR, "open_window: failed to initialize GLEW");
      glfwTerminate();
      return false;
    }

    last_mouse_x = (float) width / 2.0f;
    last_mouse_y = (float) height / 2.0f;

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      
    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, width / 2, height / 2);

    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS); 

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    // From http://www.opengl-tutorial.org/miscellaneous/faq/ VAOs are wrappers around VBOs. They
    // remember which buffer is bound to which attribute and various other things. This reduces the
    // number of OpenGL calls before glDrawArrays/Elements(). Since OpenGL 3 Core, they are
    // compulsory, but you may use only one and modify it permanently.
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);

    // Create and compile our GLSL program from the shaders
    bool shaders_ok = load_shaders("../../shaders/vertex_shader.glsl", "../../shaders/fragment_shader.glsl");
    if (!shaders_ok) {
      log(LOG_LEVEL_ERROR, "open_window: failed to load shaders");
      glfwTerminate();
      return false;
    }

    // Get a handle for our "mvp" uniform
    matrix_id = glGetUniformLocation(program_id, "mvp");
    view_matrix_id = glGetUniformLocation(program_id, "V");
    model_matrix_id = glGetUniformLocation(program_id, "M");

    for (mesh_id mid = get_first_mesh(); mid != nmesh; mid = get_next_mesh(mid)) {
      std::vector<glm::vec3> vertices = get_mesh_vertices(mid);
      std::vector<glm::vec2> texture_coords = get_mesh_texture_coords(mid);
      std::vector<glm::vec3> normals = get_mesh_normals(mid);
      std::vector<vindex> indices = get_mesh_indices(mid);
      std::string texture_path = get_material_texture_path(get_mesh_material(mid));

      // Load the texture
      GLuint texture_id = load_texture(texture_path.c_str());
      // Get a handle for our "myTextureSampler" uniform
      texture_sampler_id = glGetUniformLocation(program_id, "myTextureSampler");

      // Load it into a VBO
      GLuint positions_vbo_id = 0U;
      glGenBuffers(1, &positions_vbo_id);
      glBindBuffer(GL_ARRAY_BUFFER, positions_vbo_id);
      glBufferData(GL_ARRAY_BUFFER, 3 * vertices.size() * sizeof (float), &vertices[0][0], GL_STATIC_DRAW);

      GLuint uvs_vbo_id = 0U;
      glGenBuffers(1, &uvs_vbo_id);
      glBindBuffer(GL_ARRAY_BUFFER, uvs_vbo_id);
      glBufferData(GL_ARRAY_BUFFER, 2 * texture_coords.size() * sizeof (float), &texture_coords[0][0], GL_STATIC_DRAW);

      GLuint normals_vbo_id = 0U;
      glGenBuffers(1, &normals_vbo_id);
      glBindBuffer(GL_ARRAY_BUFFER, normals_vbo_id);
      glBufferData(GL_ARRAY_BUFFER, 3 * normals.size() * sizeof(float), &normals[0][0], GL_STATIC_DRAW);

      // Generate a buffer for the indices as well
      GLuint indices_vbo_id = 0U;
      glGenBuffers(1, &indices_vbo_id);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(vindex), &indices[0], GL_STATIC_DRAW);

      // Create a mesh context and add it to the map
      mesh_context context;
      context.mpositions_vbo_id = positions_vbo_id;
      context.muvs_vbo_id = uvs_vbo_id;
      context.mnormals_vbo_id = normals_vbo_id;
      context.mindices_vbo_id = indices_vbo_id;
      context.mtexture_id = texture_id;
      context.mnum_indices = indices.size();
      mesh_contexts[mid] = context;
    }

    // Get a handle for our "LightPosition" uniform
    glUseProgram(program_id);
    light_id = glGetUniformLocation(program_id, "LightPosition_worldspace");

    return true;
  }

  void close_window()
  {
    if (window) {
      for (auto it = mesh_contexts.begin(); it != mesh_contexts.end(); it++) {
        glDeleteBuffers(1, &it->second.mpositions_vbo_id);
        glDeleteBuffers(1, &it->second.muvs_vbo_id);
        glDeleteBuffers(1, &it->second.mnormals_vbo_id);
        glDeleteBuffers(1, &it->second.mindices_vbo_id);
        glDeleteTextures(1, &it->second.mtexture_id);
      }

      glDeleteProgram(program_id);
      glDeleteVertexArrays(1, &vertex_array_id);
      glfwTerminate();
      window = nullptr;
      ok = false;
      mesh_contexts.clear();
      vertex_array_id = 0U;
      vertex_shader_id = 0U;
      fragment_shader_id = 0U;
      program_id = 0U;
      matrix_id = 0U;
      view_matrix_id = 0U;
      model_matrix_id = 0U;
      texture_sampler_id = 0U;
      light_id = 0U;
      events.clear();
    }
  }

  void render(view_id v)
  {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!is_view_enabled(v)) {
      log(LOG_LEVEL_ERROR, "render: view is not enabled"); return;
    }
    // Use our shader
    glUseProgram(program_id);

    // For each layer in the view
    for (layer_id l = get_first_layer(v); l != nlayer && is_layer_enabled(l); l = get_next_layer(l)) {
      // Get view and projection matrices for the layer
      glm::mat4 projection_matrix;
      get_layer_projection_transform(l, &projection_matrix);
      glm::mat4 view_matrix;
      get_layer_view_transform(l, &view_matrix);

      // Get light data for the layer (actually only the position is being used)
      glm::vec3 light_position = get_light_position(l);

      struct context{ node_id nid; };
      std::queue<context> pending_nodes;
      pending_nodes.push({root_node});
      // For each node in the layer
      while (!pending_nodes.empty()) {
        auto current = pending_nodes.front();
        pending_nodes.pop();

        if (is_node_enabled(l, current.nid)) {
          glm::mat4 local_transform;
          glm::mat4 accum_transform;
          get_node_transform(l, current.nid, &local_transform, &accum_transform);
          render_node(l, current.nid, light_position, accum_transform, view_matrix, projection_matrix);

          for (node_id child = get_first_child_node(l, current.nid); child != nnode; child = get_next_sibling_node(l, child)) {
            pending_nodes.push({child});
          }
        }
      }
    }

    // Swap buffers
    glfwSwapBuffers(window);
  }

  std::vector<cgs::event> poll_events()
  {
    events.clear();
    glfwPollEvents();

    return events;
  }

  float get_time()
  {
    return (float) glfwGetTime();
  }
} // namespace cgs
