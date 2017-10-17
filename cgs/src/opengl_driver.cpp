#include "cgs/environment_mapping.hpp"
#include "cgs/opengl_driver.hpp"
#include "cgs/skybox.hpp"
#include "cgs/phong.hpp"
#include "cgs/utils.hpp"
#include "cgs/log.hpp"
#include "GL/glew.h"

#include <stdexcept>
#include <sstream>
#include <string>
#include <map>

namespace cgs
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        typedef std::map<image_format, GLenum> opengl_image_format_map;
        typedef std::map<depth_func, GLenum>   opengl_depth_func_map;

        // WARNING: this constant is also defined inside the fragment shaders
        constexpr std::size_t   MAX_POINT_LIGHTS = 10;
        opengl_image_format_map opengl_image_formats;
        opengl_depth_func_map   opengl_depth_funcs;
        GLuint                  vao_id = 0U;
        GLuint                  bound_program = 0U;
        GLuint                  bound_texture_2d = 0U;
        GLuint                  bound_texture_cubemap = 0U;
        GLuint                  bound_element_array_buffer = 0U;
        GLenum                  current_depth_func = 0U;

        void initialize_opengl_image_formats()
        {
            if (opengl_image_formats.empty()) {
                opengl_image_formats[image_format::none] = GL_RGB;
                opengl_image_formats[image_format::rgb]  = GL_RGB;
                opengl_image_formats[image_format::rgba] = GL_RGBA;
                opengl_image_formats[image_format::bgr]  = GL_BGR;
                opengl_image_formats[image_format::bgra] = GL_BGRA;
            }
        }

        void initialize_opengl_depth_func_map()
        {
            if (opengl_depth_funcs.empty()) {
                opengl_depth_funcs[depth_func::never]    = GL_NEVER;
                opengl_depth_funcs[depth_func::less]     = GL_LESS;
                opengl_depth_funcs[depth_func::equal]    = GL_EQUAL;
                opengl_depth_funcs[depth_func::lequal]   = GL_LEQUAL;
                opengl_depth_funcs[depth_func::greater]  = GL_GREATER;
                opengl_depth_funcs[depth_func::notequal] = GL_NOTEQUAL;
                opengl_depth_funcs[depth_func::gequal]   = GL_GEQUAL;
                opengl_depth_funcs[depth_func::always]   = GL_ALWAYS;
            }
        }

        void opengl_driver_init()
        {
            // Black background
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            // Initialize GLEW
            glewExperimental = true; // Needed for core profile
            if (glewInit() != GLEW_OK) {
                throw std::runtime_error("opengl_driver_init: failed to initialize GLEW");
            }
            // Enable depth test
            glEnable(GL_DEPTH_TEST);
            // Accept fragment if it closer to the camera than the former one
            glDepthFunc(GL_LESS); 
            current_depth_func = GL_LESS;
            // Cull triangles which normal is not towards the camera
            glEnable(GL_CULL_FACE);
            // From http://www.opengl-tutorial.org/miscellaneous/faq/ VAOs are wrappers around VBOs. They
            // remember which buffer is bound to which attribute and various other things. This reduces the
            // number of OpenGL calls before glDrawArrays/Elements(). Since OpenGL 3 Core, they are
            // compulsory, but you may use only one and modify it permanently.
            glGenVertexArrays(1, &vao_id);
            glBindVertexArray(vao_id);
            // Since we only use texture unit 0, we bind this unit at initialization and then never bound it again
            glActiveTexture(GL_TEXTURE0);
        }

        void new_default_texture(gl_texture_id* id)
        {
            GLuint texture_id = 0U;
            // Create a default texture to use as diffuse map on objects that don't have a texture
            glGenTextures(1, &texture_id);
            // "Bind" the newly created texture : all future texture functions will modify this texture
            glBindTexture(GL_TEXTURE_2D, texture_id);
            std::vector<unsigned char> default_texture_data = {255U, 255U, 255U};
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_BGR, GL_UNSIGNED_BYTE, &default_texture_data[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            *id = texture_id;
        }

        void delete_default_texture(gl_texture_id id)
        {
            glDeleteTextures(1, &id);  
        }

        void new_texture(unsigned int width,
                        unsigned int height,
                        image_format format,
                        const unsigned char* data,
                        gl_texture_id* id)
        {
            // Create one OpenGL texture
            GLuint texture_id;
            glGenTextures(1, &texture_id);
            // "Bind" the newly created texture : all future texture functions will modify this texture
            glBindTexture(GL_TEXTURE_2D, texture_id);
            bound_texture_2d = texture_id;
            // Give the image to OpenGL
            initialize_opengl_image_formats();
            // The format (7th argument) argument specifies the format of the data we pass in, stored in client memory
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, opengl_image_formats[format], GL_UNSIGNED_BYTE, data);
            // OpenGL has now copied the data, we can delete our image object

            // Trilinear filtering.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Return the ID of the texture we just created
            *id = texture_id;
        }

        void delete_texture(gl_texture_id id)
        {
            glDeleteTextures(1, &id);
        }

        void new_3d_buffer(const std::vector<glm::vec3>& data, gl_buffer_id* buffer_id)
        {
            GLuint vbo_id = 0U;
            glGenBuffers(1, &vbo_id);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
            glBufferData(GL_ARRAY_BUFFER, 3 * data.size() * sizeof (float), &data[0][0], GL_STATIC_DRAW);
            *buffer_id = vbo_id;
        }

        void new_2d_buffer(const std::vector<glm::vec2>& data, gl_buffer_id* buffer_id)
        {
            GLuint vbo_id = 0U;
            glGenBuffers(1, &vbo_id);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
            glBufferData(GL_ARRAY_BUFFER, 2 * data.size() * sizeof (float), &data[0][0], GL_STATIC_DRAW);
            *buffer_id = vbo_id;
        }

        void new_index_buffer(const std::vector<unsigned short>& indices, gl_buffer_id* buffer_id)
        {
            // Generate a buffer for the indices as well
            GLuint vbo_id = 0U;
            glGenBuffers(1, &vbo_id);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_id);
            bound_element_array_buffer = vbo_id;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
            *buffer_id = vbo_id;
        }

        void delete_buffer(gl_buffer_id buffer_id)
        {
            glDeleteBuffers(1, &buffer_id);
        }

        void flip_image_vertically(unsigned int width,
                                   unsigned int height,
                                   image_format format,
                                   const unsigned char* data,
                                   std::vector<unsigned char>* flipped_image)
        {
            unsigned int bpp = (format == image_format::rgb || format == image_format::bgr)? 3U : 4U;
            for (unsigned int i = 0; i < height; i++) {
                unsigned int row = height - 1 - i;
                for (unsigned int j = 0; j < width; j++) {
                    unsigned int pixel_base = row * width * bpp + j * bpp;
                    flipped_image->push_back(data[pixel_base]);
                    flipped_image->push_back(data[pixel_base + 1]);
                    flipped_image->push_back(data[pixel_base + 2]);
                    if (format == image_format::rgba || format == image_format::bgra) {
                        flipped_image->push_back(data[pixel_base + 3]); 
                    }
                }
            }
        }

        void new_cubemap(unsigned int width,
                          unsigned int height,
                          image_format format,
                          const std::vector<const unsigned char*>& faces_data,
                          gl_cubemap_id* id)
        {
            unsigned int texture_id;
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
            bound_texture_cubemap = texture_id;
            for (unsigned int i = 0; i < faces_data.size(); i++)
            {
                // In the gl_driver interface, images are store upside-down in memory. For normal
                // textures this matches what OpenGL expects, but for cubemaps OpenGL follows the
                // RenderMan criteria, which requires flipping the images. You can flip the images
                // on disk, or by software. Here we opted for the second approach. Give the image to
                // OpenGL
                std::vector<unsigned char> flipped_image;
                flip_image_vertically(width, height, format, faces_data[i], &flipped_image);
                initialize_opengl_image_formats();
                // The format (7th argument) argument specifies the format of the data we pass in, stored in client memory
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, opengl_image_formats[format], GL_UNSIGNED_BYTE, &flipped_image[0]);
                // OpenGL has now copied the data, we can delete our image object
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            *id = texture_id;
        }

        void delete_cubemap(gl_cubemap_id id)
        {
            glDeleteTextures(1, &id);
        }

        void load_shaders(const char* vertex_shader_source,
                          const char* fragment_shader_source,
                          gl_program_id* gl_program_id)
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
                glDeleteShader(vertex_shader_id);
                throw std::runtime_error("Error when compiling vertex shader: " + std::string(&error_message[0]));
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
                glDeleteShader(vertex_shader_id);
                glDeleteShader(fragment_shader_id);
                throw std::runtime_error("Error when compiling fragment shader: " + std::string(&fragment_shader_error_message[0]));
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
                glDetachShader(new_program_id, vertex_shader_id);
                glDetachShader(new_program_id, fragment_shader_id);
                glDeleteShader(vertex_shader_id);
                glDeleteShader(fragment_shader_id);
                throw std::runtime_error("Error when linking shaders: " + std::string(&program_error_message[0]));
            }
            log(LOG_LEVEL_DEBUG, "program linked successfully");

            *gl_program_id = new_program_id;

            glDetachShader(new_program_id, vertex_shader_id);
            glDetachShader(new_program_id, fragment_shader_id);
            glDeleteShader(vertex_shader_id);
            glDeleteShader(fragment_shader_id);
        }

        void initialize_frame()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        void draw(const gl_driver_context& context)
        {
            initialize_opengl_depth_func_map();
            GLenum previous_depth_func = 0U;
            bool depth_func_updated = false;
            if (opengl_depth_funcs[context.m_depth_func] != current_depth_func) {
                depth_func_updated = true;
                glDepthFunc(opengl_depth_funcs[context.m_depth_func]);
                previous_depth_func = current_depth_func;
                current_depth_func = opengl_depth_funcs[context.m_depth_func];
            }
            // Bind the program
            if (context.m_program != bound_program) {
                glUseProgram(context.m_program);
                bound_program = context.m_program;
            }
            // Send our transformation to the currently bound shader
            glm::mat4 mvp = context.m_projection * context.m_view * context.m_node.m_model;
            glUniformMatrix4fv(glGetUniformLocation(context.m_program, "model"), 1, GL_FALSE, &context.m_node.m_model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(context.m_program, "view"), 1, GL_FALSE, &context.m_view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(context.m_program, "projection"), 1, GL_FALSE, &context.m_projection[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(context.m_program, "mvp"), 1, GL_FALSE, &mvp[0][0]);
            // Bind our cubemap texture in the GL_TEXTURE_CUBE_MAP target of texture unit 0
            // The texture unit is 0 because we called glActiveTexture(GL_TEXTURE0) at initialization
            if (bound_texture_cubemap != context.m_cubemap) {
                glBindTexture(GL_TEXTURE_CUBE_MAP, context.m_cubemap);
                bound_texture_cubemap = context.m_cubemap;
            }
            // Set the cubemap uniform. The value to set is the texture unit, which is 0 because we
            // called glActiveTexture(GL_TEXTURE0) at initialization. The cubemap sampler knows that
            // is needs to use the GL_TEXTURE_CUBE_MAP target of that unit.
            glUniform1i(glGetUniformLocation(context.m_program, "cubemap"), 0);

            // Bind our texture in the GL_TEXTURE_2D target of texture unit 0
            // The texture unit is 0 because we called glActiveTexture(GL_TEXTURE0) at initialization
            if (bound_texture_2d != context.m_node.m_texture) {
                glBindTexture(GL_TEXTURE_2D, context.m_node.m_texture);
                bound_texture_2d = context.m_node.m_texture;
            }
            // Set the diffuse_sampler uniform (used to sample the texture). The value to set is the
            // texture unit, which is 0 because we called glActiveTexture(GL_TEXTURE0) at
            // initialization. The 2D sampler knows that is needs to use the GL_TEXTURE_2D target of
            // that unit.
            glUniform1i(glGetUniformLocation(context.m_program, "material.diffuse_sampler"), 0);

            // Set material uniform properties from material information
            glUniform3fv(glGetUniformLocation(context.m_program, "material.diffuse_color"), 1, &context.m_node.m_material.m_diffuse_color[0]);
            glUniform3fv(glGetUniformLocation(context.m_program, "material.specular_color"), 1, &context.m_node.m_material.m_specular_color[0]);
            glUniform1f(glGetUniformLocation(context.m_program,  "material.smoothness"), context.m_node.m_material.m_smoothness);
            glUniform1f(glGetUniformLocation(context.m_program,  "material.reflectivity"), context.m_node.m_material.m_reflectivity);
            glUniform1f(glGetUniformLocation(context.m_program,  "material.translucency"), context.m_node.m_material.m_translucency);
            glUniform1f(glGetUniformLocation(context.m_program,  "material.refractive_index"), context.m_node.m_material.m_refractive_index);

            // Set dirlight uniform properties
            glUniform3fv(glGetUniformLocation(context.m_program, "dirlight.ambient_color"),  1, &context.m_dirlight.m_ambient_color[0]);
            glUniform3fv(glGetUniformLocation(context.m_program, "dirlight.diffuse_color"),  1, &context.m_dirlight.m_diffuse_color[0]);
            glUniform3fv(glGetUniformLocation(context.m_program, "dirlight.specular_color"), 1, &context.m_dirlight.m_specular_color[0]);
            glm::vec3 direction_cameraspace(context.m_view * glm::vec4(context.m_dirlight.m_direction_cameraspace, 0.0f));
            glUniform3fv(glGetUniformLocation(context.m_program, "dirlight.direction_cameraspace"), 1, &direction_cameraspace[0]);

            // Set camera position uniform
            glm::vec3 camera_position_worldspace = camera_position_worldspace_from_view_matrix(context.m_view);
            glUniform3fv(glGetUniformLocation(context.m_program, "camera_position_worldspace"), 1, &camera_position_worldspace[0]);

            // Set point light uniform
            std::size_t sent_point_lights = 0U;
            for (std::vector<point_light_data>::const_iterator it = context.m_point_lights.begin(); it != context.m_point_lights.end(); it++) {
                if (sent_point_lights >= MAX_POINT_LIGHTS) { break; }

                std::ostringstream oss;
                oss << "point_lights[" << sent_point_lights << "]";
                std::string uniform_prefix = oss.str();

                glUniform3fv(glGetUniformLocation(context.m_program, (uniform_prefix + ".position_cameraspace").c_str()), 1, &it->m_position_cameraspace[0]);
                glUniform3fv(glGetUniformLocation(context.m_program, (uniform_prefix + ".ambient_color").c_str()), 1, &it->m_ambient_color[0]);
                glUniform3fv(glGetUniformLocation(context.m_program, (uniform_prefix + ".diffuse_color").c_str()), 1, &it->m_diffuse_color[0]);
                glUniform3fv(glGetUniformLocation(context.m_program, (uniform_prefix + ".specular_color").c_str()), 1, &it->m_specular_color[0]);
                glUniform1f(glGetUniformLocation(context.m_program,  (uniform_prefix + ".constant_attenuation").c_str()), it->m_constant_attenuation);
                glUniform1f(glGetUniformLocation(context.m_program,  (uniform_prefix + ".linear_attenuation").c_str()), it->m_linear_attenuation);
                glUniform1f(glGetUniformLocation(context.m_program,  (uniform_prefix + ".quadratic_attenuation").c_str()), it->m_quadratic_attenuation);

                sent_point_lights++;
            }
            glUniform1ui(glGetUniformLocation(context.m_program, "npoint_lights"), sent_point_lights);

            // 1rst attribute buffer : vertices
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, context.m_node.m_position_buffer);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

            // 2nd attribute buffer : UVs
            if (context.m_node.m_texture_coords_buffer) {
                glEnableVertexAttribArray(1);
                glBindBuffer(GL_ARRAY_BUFFER, context.m_node.m_texture_coords_buffer);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);
            }

            // 3rd attribute buffer : normals
            if (context.m_node.m_normal_buffer) {
                glEnableVertexAttribArray(2);
                glBindBuffer(GL_ARRAY_BUFFER, context.m_node.m_normal_buffer);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
            }

            // Index buffer
            if (bound_element_array_buffer != context.m_node.m_index_buffer) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context.m_node.m_index_buffer);
                bound_element_array_buffer = context.m_node.m_index_buffer;
            }

            // Draw the triangles !
            glDrawElements(GL_TRIANGLES, context.m_node.m_num_indices, GL_UNSIGNED_SHORT, (void*) 0);

            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(2);

            // Restore the previous depth func
            if (depth_func_updated) {
                glDepthFunc(previous_depth_func);
                current_depth_func = previous_depth_func;
            }
        }

        void new_program(program_type type, gl_program_id* gl_program_id)
        {
            // The strings phong_vertex_shader and phong_fragment_shader and so forth are defined in
            // header file phong.hpp. These strings contain GLSL code embedded into our application
            // as raw string literals. Same goes for the other shaders.
            if (type == program_type::phong) {
                load_shaders(phong_vertex_shader, phong_fragment_shader, gl_program_id);
            } else if (type == program_type::environment_mapping) {
                load_shaders(environment_mapping_vertex_shader, environment_mapping_fragment_shader, gl_program_id);
            } else if (type == program_type::skybox) {
                load_shaders(skybox_vertex_shader, skybox_fragment_shader, gl_program_id);
            }
        }

        void delete_program(gl_program_id id)
        {
            glDeleteProgram(id);
        }
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    gl_driver get_opengl_driver()
    {
        gl_driver driver;
        driver.gl_driver_init = opengl_driver_init;
        driver.new_default_texture = new_default_texture;
        driver.delete_default_texture = delete_default_texture;
        driver.new_texture = new_texture;
        driver.delete_texture = delete_texture;
        driver.new_3d_buffer = new_3d_buffer;
        driver.new_2d_buffer = new_2d_buffer;
        driver.new_index_buffer = new_index_buffer;
        driver.delete_buffer = delete_buffer;
        driver.new_cubemap = new_cubemap;
        driver.delete_cubemap = delete_cubemap;
        driver.new_program = new_program;
        driver.delete_program = delete_program;
        driver.initialize_frame = initialize_frame;
        driver.draw = draw;

        return driver;
    }
} // namespace cgs
