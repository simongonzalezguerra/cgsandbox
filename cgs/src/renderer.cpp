#include "glm/gtc/matrix_transform.hpp"
#include "cgs/resource_database.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cgs/renderer.hpp"
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
        // WARNING: this constant is also defined inside the object fragment shader
        constexpr std::size_t MAX_POINT_LIGHTS = 10;

        struct mesh_context
        {
            GLuint  mpositions_vbo_id;
            GLuint  muvs_vbo_id;
            GLuint  mnormals_vbo_id;
            GLuint  mindices_vbo_id;
            GLuint  mnum_indices;
        };

        struct material_context
        {
            GLuint mtexture_id;
        };

        struct skybox_context
        {
            GLuint mvao_id;
            GLuint mvbo_id;
            GLuint mtexture_id;
        };

        struct dirlight_data
        {
            glm::vec3 mambient_color;
            glm::vec3 mdiffuse_color;
            glm::vec3 mspecular_color;
            glm::vec3 mdirection;
        };

        typedef std::map<mesh_id, mesh_context>      mesh_context_map;
        typedef std::map<cubemap_id, skybox_context> cubemap_context_map;
        typedef std::map<int, event_type>            key_event_type_map;
        typedef key_event_type_map::iterator         event_type_it;
        typedef std::map<int, std::string>           error_code_map;
        typedef std::map<mat_id, material_context>   material_context_map;

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        // Based on the shaders from
        //     http://www.opengl-tutorial.org/beginners-tutorials/tutorial-8-basic-shading/
        //     https://learnopengl.com/#!Lighting/Multiple-lights
        const char* object_vertex_shader = R"glsl(
            #version 330 core

            // Input vertex data, different for all executions of this shader.
            layout(location = 0) in vec3 vertex_position_modelspace;
            layout(location = 1) in vec2 vertex_tex_coords;
            layout(location = 2) in vec3 vertex_direction_n_modelspace;

            // Output data ; will be interpolated for each fragment.
            out vec2 tex_coords;
            out vec3 position_worldspace;
            out vec3 position_cameraspace;
            out vec3 direction_n_cameraspace;
            out vec3 direction_v_cameraspace;

            // Values that stay constant for the whole mesh.
            uniform mat4 mvp;
            uniform mat4 view;
            uniform mat4 model;

            void main(){
                // Output position of the vertex, in clip space : mvp * position
                gl_Position =  mvp * vec4(vertex_position_modelspace,1);

                // Position of the vertex, in worldspace : model * position
                position_worldspace = (model * vec4(vertex_position_modelspace, 1)).xyz;

                // Vector that goes from the vertex to the camera, in camera space.
                // In camera space, the camera is at the origin (0,0,0).
                position_cameraspace = (view * model * vec4(vertex_position_modelspace, 1)).xyz;
                direction_v_cameraspace = vec3(0,0,0) - position_cameraspace;

                // Normal of the the vertex, in camera space. Note this is only correct if the model
                // transform does not scale the model in a way that is non-uniform accross all axes! If not
                // you can use its inverse transpose, but keep in mind that computing the inverse is expensive
                // (direction_n_cameraspace = mat3(transpose(inverse(model))) * vertex_direction_n_modelspace;)
                direction_n_cameraspace = ( view * model * vec4(vertex_direction_n_modelspace,0)).xyz;

                // Texture coordinates of the vertex. No special space for this one.
                tex_coords = vertex_tex_coords;
            }
        )glsl";

        const char* object_fragment_shader = R"glsl(
            // Lighting computations are performed in camera space. This can also be done in worlspace with the same
            // result, what matters is that all vectors are expressed in the same coordinate system. Regardless of this,
            // there are precision advantages to computing in view space (worldspace can have coordinates with large
            // values that introduce precision issues).
            // Source:
            // https://www.opengl.org/discussion_boards/showthread.php/168104-lighting-in-eye-space-or-in-world-space

            #version 330 core

            #define MAX_POINT_LIGHTS 10

            struct material_data
            {
                sampler2D diffuse_sampler;
                vec3      diffuse_color;
                vec3      specular_color; 
                float     smoothness;
            };

            struct dirlight_data
            {
                vec3 ambient_color;
                vec3 diffuse_color;
                vec3 specular_color;
                vec3 direction_cameraspace;
            };

            struct point_light_data
            {
                vec3  position_cameraspace;
                vec3  ambient_color;
                vec3  diffuse_color;
                vec3  specular_color;
                float constant_attenuation;
                float linear_attenuation;
                float quadratic_attenuation;
            };


            // Interpolated values from the vertex shaders
            in vec2 tex_coords;
            in vec3 position_worldspace;
            in vec3 position_cameraspace;
            in vec3 direction_n_cameraspace;
            in vec3 direction_v_cameraspace;

            // Ouput data
            out vec3 color;

            // Values that stay constant for the whole mesh.
            uniform material_data     material;
            uniform dirlight_data     dirlight;
            uniform point_light_data  point_lights[MAX_POINT_LIGHTS];
            uniform uint              npoint_lights;

            // Calculates the contribution of the directional light
            vec3 calc_dirlight(dirlight_data dirlight,
                                vec3 n_cameraspace,
                                vec3 v_cameraspace,
                                material_data material,
                                vec2 tex_coords)
            {
                vec3 l_cameraspace = normalize(-dirlight.direction_cameraspace);
                // diffuse shading
                float cos_theta_diff = clamp(dot(n_cameraspace, l_cameraspace), 0, 1);
                // specular shading
                vec3 r_cameraspace = reflect(-l_cameraspace, n_cameraspace);
                float cos_alpha_spec = clamp(dot(v_cameraspace, r_cameraspace), 0, 1);
                // combine results
                vec3 ambient = dirlight.ambient_color * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
                vec3 diffuse = dirlight.diffuse_color * cos_theta_diff * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
                vec3 specular = dirlight.specular_color * pow(cos_alpha_spec, material.smoothness) * material.specular_color;
                return (ambient + diffuse + specular);
            }

            vec3 calc_point_light(point_light_data point_light,
                                vec3 n_cameraspace,
                                vec3 position_cameraspace,
                                vec3 v_cameraspace,
                                material_data material,
                                vec2 tex_coords)
            {
                vec3 l_cameraspace = normalize(point_light.position_cameraspace - position_cameraspace);
                // diffuse shading
                float cos_theta_diff = clamp(dot(n_cameraspace, l_cameraspace), 0, 1);
                // specular shading
                vec3 r_cameraspace = reflect(-l_cameraspace, n_cameraspace);
                float cos_alpha_spec = clamp(dot(v_cameraspace, r_cameraspace), 0, 1);
                // attenuation
                float distance = length(point_light.position_cameraspace - position_cameraspace);
                float attenuation = 1.0 / (point_light.constant_attenuation
                                           + point_light.linear_attenuation * distance
                                           + point_light.quadratic_attenuation * (distance * distance));    
                // combine results
                vec3 ambient  = point_light.ambient_color * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
                vec3 diffuse  = point_light.diffuse_color * cos_theta_diff * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
                vec3 specular = point_light.specular_color * pow(cos_alpha_spec, material.smoothness) * material.specular_color;
                ambient *= attenuation;
                diffuse *= attenuation;
                specular *= attenuation;
                return (ambient + diffuse + specular);
            }

            void main()
            {
                // Normal of the computed fragment, in camera space
                vec3 n_cameraspace = normalize(direction_n_cameraspace);
                // Eye vector (towards the camera)
                vec3 v_cameraspace = normalize(direction_v_cameraspace);
                // Phase 1: directional lighting
                color = calc_dirlight(dirlight, n_cameraspace, v_cameraspace, material, tex_coords);
                // Phase 2: point lights
                for (uint i = 0U; i < npoint_lights; i++) {
                    color += calc_point_light(point_lights[i], n_cameraspace, position_cameraspace, v_cameraspace, material, tex_coords); 
                }
            }
        )glsl";

        const char* skybox_vertex_shader = R"glsl(
            #version 330 core
            layout (location = 0) in vec3 a_pos;

            out vec3 tex_coords;

            uniform mat4 projection;
            uniform mat4 view;

            void main()
            {
                tex_coords = a_pos;
                vec4 pos = projection * view * vec4(a_pos, 1.0);
                gl_Position = pos.xyww;
            }
        )glsl";

        const char* skybox_fragment_shader = R"glsl(
            #version 330 core
            out vec4 FragColor;

            in vec3 tex_coords;

            uniform samplerCube skybox;

            void main()
            {    
                FragColor = texture(skybox, tex_coords);
            }
        )glsl";

        float skybox_vertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
        };

        GLFWwindow*          window = nullptr;
        bool                 ok = false;
        mesh_context_map     mesh_contexts;
        cubemap_context_map  skybox_contexts;
        GLuint               object_vao_id = 0U;
        GLuint               object_program_id = 0U;
        GLuint               skybox_program_id = 0U;
        GLuint               matrix_id = 0U;
        GLuint               view_matrix_id = 0U;
        GLuint               model_matrix_id = 0U;
        std::vector<event>   events;
        key_event_type_map   event_types;     // map from GLFW key action value to event_type value
        error_code_map       error_codes;     // map from GLFW error codes to their names
        float                last_mouse_x = 0.0f;
        float                last_mouse_y = 0.0f;
        dirlight_data        dirlight;
        layer_id             current_layer;
        material_context_map material_contexts;
        GLuint               default_texture_id = 0U;

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        bool load_shaders(const char* vertex_shader_source, const char* fragment_shader_source, GLuint* program_id)
        {
            // Create the shaders
            GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER); 
            GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

            GLint result = GL_FALSE;
            int info_log_length = 0;

            // Compile Vertex Shader
            log(LOG_LEVEL_DEBUG, std::string("load_shaders: compiling vertex shader: "));
            glShaderSource(vertex_shader_id, 1, &vertex_shader_source, nullptr);
            glCompileShader(vertex_shader_id);

            // Check Vertex Shader
            glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
            glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
            if (result == GL_FALSE){
                std::vector<char> error_message(info_log_length + 1);
                glGetShaderInfoLog(vertex_shader_id, info_log_length, nullptr, &error_message[0]);
                log(LOG_LEVEL_ERROR, &error_message[0]);
                return false;
            }
            log(LOG_LEVEL_DEBUG, "vertex shader compiled successfully");

            // Compile Fragment Shader
            log(LOG_LEVEL_DEBUG, std::string("load_shaders: compiling fragment shader: "));
            glShaderSource(fragment_shader_id, 1, &fragment_shader_source, nullptr);
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
            GLuint new_program_id = glCreateProgram();
            glAttachShader(new_program_id, vertex_shader_id);
            glAttachShader(new_program_id, fragment_shader_id);
            glLinkProgram(new_program_id);

            // Check the program
            glGetProgramiv(new_program_id, GL_LINK_STATUS, &result);
            glGetProgramiv(new_program_id, GL_INFO_LOG_LENGTH, &info_log_length);
            if (result == GL_FALSE) {
                std::vector<char> program_error_message(info_log_length + 1);
                glGetProgramInfoLog(new_program_id, info_log_length, nullptr, &program_error_message[0]);
                log(LOG_LEVEL_ERROR, &program_error_message[0]);
                return false;
            }
            log(LOG_LEVEL_DEBUG, "program linked successfully");

            *program_id = new_program_id;

            glDetachShader(new_program_id, vertex_shader_id);
            glDetachShader(new_program_id, fragment_shader_id);
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

            FREE_IMAGE_FORMAT format = FreeImage_GetFileType(path, 0); // automatically detects the format(from over 20 formats!)
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
            GLuint texture_id;
            glGenTextures(1, &texture_id);
            // "Bind" the newly created texture : all future texture functions will modify this texture
            glBindTexture(GL_TEXTURE_2D, texture_id);

            // Give the image to OpenGL
            // Note about image format: when running in another OS you may need to change RGB to BGR for
            // the raw data to be interpreted correctly. From the FreeImage doc: However, the pixel layout
            // used by this model is OS dependant. Using a byte by byte memory order to label the pixel
            // layout, then FreeImage uses a BGR[A] pixel layout under a Little Endian processor (Windows,
            // Linux) and uses a RGB[A] pixel layout under a Big Endian processor (Mac OS X or any Big
            // Endian Linux / Unix). This choice was made to ease the use of FreeImage with graphics API.
            // When runing on Linux, FreeImage stores data in BGRA format.
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, &data[0]);
            // OpenGL has now copied the data. Free our own version

            // Trilinear filtering.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Return the ID of the texture we just created
            return texture_id;
        }

        void render_node(layer_id l, node_id n, const glm::mat4& model_matrix, const glm::mat4& view_matrix, const glm::mat4& projection_matrix)
        {
            // The root node itself has no content to render
            if (get_node_material(l, n) == nmat) return;
            std::vector<mesh_id> meshes = get_node_meshes(l, n);
            // Get buffer ids previously created for the mesh
            auto& context = mesh_contexts[get_node_mesh(l, n)];

            glm::mat4 mvp = projection_matrix * view_matrix * model_matrix;

            // Send our transformation to the currently bound shader, 
            // in the "mvp" uniform
            glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);
            glUniformMatrix4fv(model_matrix_id, 1, GL_FALSE, &model_matrix[0][0]);
            glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, &view_matrix[0][0]);

            // Bind our texture in texture Unit 0
            glActiveTexture(GL_TEXTURE0);
            mat_id node_material = get_node_material(l, n);
            GLuint texture_id = default_texture_id;
            if (material_contexts.find(node_material) != material_contexts.end()) {
                texture_id = material_contexts[node_material].mtexture_id;
            }
            glBindTexture(GL_TEXTURE_2D, texture_id);
            // Set material unform properties from material information
            glUniform1i(glGetUniformLocation(object_program_id, "material.diffuse_sampler"), 0);
            glm::vec3 diffuse_color = get_material_diffuse_color(node_material);
            glUniform3fv(glGetUniformLocation(object_program_id, "material.diffuse_color"), 1, &diffuse_color[0]);
            glm::vec3 specular_color = get_material_specular_color(node_material);
            glUniform3fv(glGetUniformLocation(object_program_id, "material.specular_color"), 1, &specular_color[0]);
            glUniform1f(glGetUniformLocation(object_program_id, "material.smoothness"), get_material_smoothness(get_node_material(l, n)));
            // Set dirlight uniform
            glUniform3fv(glGetUniformLocation(object_program_id, "dirlight.ambient_color"),  1, &dirlight.mambient_color[0]);
            glUniform3fv(glGetUniformLocation(object_program_id, "dirlight.diffuse_color"),  1, &dirlight.mdiffuse_color[0]);
            glUniform3fv(glGetUniformLocation(object_program_id, "dirlight.specular_color"), 1, &dirlight.mspecular_color[0]);
            glm::vec3 direction_cameraspace(view_matrix * glm::vec4(dirlight.mdirection, 0.0f));
            glUniform3fv(glGetUniformLocation(object_program_id, "dirlight.direction_cameraspace"), 1, &direction_cameraspace[0]);

            // Set point light uniform
            std::size_t sent_point_lights = 0U;
            for (point_light_id point_light = get_first_point_light(current_layer); point_light != npoint_light; point_light = get_next_point_light(current_layer, point_light)) {
                if (sent_point_lights >= MAX_POINT_LIGHTS) { break; }

                glm::vec3 position_worldspace = get_point_light_position(current_layer, point_light);
                glm::vec3 position_cameraspace(view_matrix * glm::vec4(position_worldspace, 0.0f));
                glm::vec3 ambient_color = get_point_light_ambient_color(current_layer, point_light);
                glm::vec3 diffuse_color = get_point_light_diffuse_color(current_layer, point_light);
                glm::vec3 specular_color = get_point_light_specular_color(current_layer, point_light);
                float constant_attenuation = get_point_light_constant_attenuation(current_layer, point_light);
                float linear_attenuation = get_point_light_linear_attenuation(current_layer, point_light);
                float quadratic_attenuation = get_point_light_quadratic_attenuation(current_layer, point_light);

                std::ostringstream oss;
                oss << "point_lights[" << sent_point_lights << "]";
                std::string uniform_prefix = oss.str();
                glUniform3fv(glGetUniformLocation(object_program_id, (uniform_prefix + ".position_cameraspace").c_str()), 1, &position_cameraspace[0]);
                glUniform3fv(glGetUniformLocation(object_program_id, (uniform_prefix + ".ambient_color").c_str()), 1, &ambient_color[0]);
                glUniform3fv(glGetUniformLocation(object_program_id, (uniform_prefix + ".diffuse_color").c_str()), 1, &diffuse_color[0]);
                glUniform3fv(glGetUniformLocation(object_program_id, (uniform_prefix + ".specular_color").c_str()), 1, &specular_color[0]);
                glUniform1f(glGetUniformLocation(object_program_id,  (uniform_prefix + ".constant_attenuation").c_str()), constant_attenuation);
                glUniform1f(glGetUniformLocation(object_program_id,  (uniform_prefix + ".linear_attenuation").c_str()), linear_attenuation);
                glUniform1f(glGetUniformLocation(object_program_id,  (uniform_prefix + ".quadratic_attenuation").c_str()), quadratic_attenuation);

                sent_point_lights++;
            }
            glUniform1ui(glGetUniformLocation(object_program_id, "npoint_lights"), sent_point_lights);

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

        // loads a cubemap texture from 6 individual texture faces
        // order:
        // +X (right)
        // -X (left)
        // +Y (top)
        // -Y (bottom)
        // +Z (front) 
        // -Z (back)
        // -------------------------------------------------------
        bool load_cubemap(const std::vector<std::string>& faces, GLuint* cubemap_texture)
        {
            unsigned int texture_id;
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

            for (unsigned int i = 0; i < faces.size(); i++)
            {
                log(LOG_LEVEL_DEBUG, "load_cubemap: loading image from path " + faces[i]);

                FREE_IMAGE_FORMAT format = FreeImage_GetFileType(faces[i].c_str(), 0); // automatically detects the format(from over 20 formats!)
                FIBITMAP* image = FreeImage_Load(format, faces[i].c_str());
                if (image == nullptr) {
                    log(LOG_LEVEL_ERROR, "load_cubemap: could not find image in path " + faces[i]);
                    FreeImage_Unload(image);
                    return false;
                }
                // In FreeImage, images are store upside-down in memory (see the PDF doc about pixel
                // access). For normal textures this is matches what OpenGL expects, but for cubemaps
                // OpenGL follows the RenderMan criteria, which requires flipping the images. You can flip
                // the images on disk, or by software. Here we opted for the second approach.
                FreeImage_FlipVertical(image);
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
                oss << "load_cubemap: loaded image of (width x height) = " << width << " x " << height
                    << " pixels, " << depth * width * height << " bytes in total" << ", bytes by pixel: " << depth
                    << std::hex << ", red mask: " << red_mask << ", green mask: " << green_mask << ", blue mask: " << blue_mask;
                log(LOG_LEVEL_DEBUG, oss.str());

                // Note about image format: when running in another OS you may need to change RGB to BGR for
                // the raw data to be interpreted correctly. From the FreeImage doc: However, the pixel layout
                // used by this model is OS dependant. Using a byte by byte memory order to label the pixel
                // layout, then FreeImage uses a BGR[A] pixel layout under a Little Endian processor (Windows,
                // Linux) and uses a RGB[A] pixel layout under a Big Endian processor (Mac OS X or any Big
                // Endian Linux / Unix). This choice was made to ease the use of FreeImage with graphics API.
                // When runing on Linux, FreeImage stores data in BGR format.
                // The format (7th argument) argument specifies the format of the data we pass in, stored in client memory
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, &data[0]);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            *cubemap_texture = texture_id;
            return true;
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
        glfwSwapInterval(1);

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
        glGenVertexArrays(1, &object_vao_id);

        // Create and compile our GLSL program from the shaders
        bool ok = load_shaders(object_vertex_shader, object_fragment_shader, &object_program_id);
        if (!ok) {
            log(LOG_LEVEL_ERROR, "open_window: failed to load object shaders");
            glfwTerminate();
            return false;
        }

        for (mat_id mat = get_first_material(); mat != nmat; mat = get_next_material(mat)) {
            // Load the texture
            if (!get_material_texture_path(mat).empty()) {
                GLuint texture_id = load_texture(get_material_texture_path(mat).c_str());
                material_contexts[mat] = material_context{texture_id};
            }
        }

        // Get a handle for our "mvp" uniform
        matrix_id = glGetUniformLocation(object_program_id, "mvp");
        view_matrix_id = glGetUniformLocation(object_program_id, "view");
        model_matrix_id = glGetUniformLocation(object_program_id, "model");

        for (mesh_id mid = get_first_mesh(); mid != nmesh; mid = get_next_mesh(mid)) {
            std::vector<glm::vec3> vertices = get_mesh_vertices(mid);
            std::vector<glm::vec2> texture_coords = get_mesh_texture_coords(mid);
            std::vector<glm::vec3> normals = get_mesh_normals(mid);
            std::vector<vindex> indices = get_mesh_indices(mid);

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
            context.mnum_indices = indices.size();
            mesh_contexts[mid] = context;
        }

        // Get a handle for our "LightPosition" uniform
        glUseProgram(object_program_id);

        ////////////////////////////////////////////////////////////////////////////////////
        // Skybox initialization
        ////////////////////////////////////////////////////////////////////////////////////

        // Create and compile our GLSL program from the shaders
        ok = load_shaders(skybox_vertex_shader, skybox_fragment_shader, &skybox_program_id);
        if (!ok) {
            log(LOG_LEVEL_ERROR, "open_window: failed to load skybox shaders");
            glfwTerminate();
            return false;
        }

        for (cubemap_id cid = get_first_cubemap(); cid != ncubemap; cid = get_next_cubemap(cid)) {
            // skybox VAO and VBO
            skybox_context context;
            glGenVertexArrays(1, &context.mvao_id);
            glGenBuffers(1, &context.mvbo_id);
            glBindVertexArray(context.mvao_id);
            glBindBuffer(GL_ARRAY_BUFFER, context.mvbo_id);
            glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), &skybox_vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

            std::vector<std::string> faces = get_cubemap_faces(cid);
            ok = load_cubemap(faces, &context.mtexture_id);
            if (!ok) {
                log(LOG_LEVEL_ERROR, "open_window: failed to load skybox textures");
                glfwTerminate();
                return false;
            }

            skybox_contexts[cid] = context;
        }

        // Create a default texture to use as diffuse map on objects that don't have a texture
        glGenTextures(1, &default_texture_id);
        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, default_texture_id);
        std::vector<unsigned char> default_texture_data = {255U, 255U, 255U};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_BGR, GL_UNSIGNED_BYTE, &default_texture_data[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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
            }

            glDeleteProgram(object_program_id);

            for (auto it = material_contexts.begin(); it != material_contexts.end(); it++) {
                glDeleteTextures(1, &it->second.mtexture_id);
            }

            glDeleteProgram(skybox_program_id);
            for (auto it = skybox_contexts.begin(); it != skybox_contexts.end(); it++) {
                glDeleteBuffers(1, &it->second.mvbo_id);
                glDeleteVertexArrays(1, &it->second.mvao_id);
                glDeleteTextures(1, &it->second.mtexture_id);
            }

            glfwTerminate();
            window = nullptr;
            ok = false;
            mesh_contexts.clear();
            object_vao_id = 0U;
            object_program_id = 0U;
            skybox_program_id = 0U;
            matrix_id = 0U;
            view_matrix_id = 0U;
            model_matrix_id = 0U;
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

        // For each layer in the view
        for (layer_id l = get_first_layer(v); l != nlayer && is_layer_enabled(l); l = get_next_layer(l)) {
            current_layer = l;
            dirlight.mambient_color = get_directional_light_ambient_color(l);
            dirlight.mdiffuse_color = get_directional_light_diffuse_color(l);
            dirlight.mspecular_color = get_directional_light_specular_color(l);
            dirlight.mdirection = get_directional_light_direction(l);

            // Use our shader
            glUseProgram(object_program_id);
            glBindVertexArray(object_vao_id);

            // Get view and projection matrices for the layer
            glm::mat4 projection_matrix;
            get_layer_projection_transform(l, &projection_matrix);
            glm::mat4 view_matrix;
            get_layer_view_transform(l, &view_matrix);

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
                    render_node(l, current.nid, accum_transform, view_matrix, projection_matrix);

                    for (node_id child = get_first_child_node(l, current.nid); child != nnode; child = get_next_sibling_node(l, child)) {
                        pending_nodes.push({child});
                    }
                }
            }

            cubemap_id skybox_id = get_layer_skybox(l);
            if (skybox_id != ncubemap) {
                // Render the skybox
                glUseProgram(skybox_program_id);
                glUniform1i(glGetUniformLocation(skybox_program_id, "skybox"), 0);
                glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
                glm::mat4 skybox_view = glm::mat4(glm::mat3(view_matrix)); // remove translation from the view matrix
                glUniformMatrix4fv(glGetUniformLocation(skybox_program_id, "view"), 1, GL_FALSE, &skybox_view[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(skybox_program_id, "projection"), 1, GL_FALSE, &projection_matrix[0][0]);
                // skybox cube
                cubemap_context_map::iterator it = skybox_contexts.find(l);
                glBindVertexArray(it->second.mvao_id);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, it->second.mtexture_id);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glBindVertexArray(0);
                glDepthFunc(GL_LESS); // set depth function back to default
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
