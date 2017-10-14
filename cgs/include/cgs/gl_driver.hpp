#ifndef GL_DRIVER_HPP
#define GL_DRIVER_HPP

#include "cgs/cgs_common.hpp"
#include "glm/glm.hpp"

#include <string>
#include <vector>

namespace cgs
{
    struct material_data
    {
        glm::vec3 m_diffuse_color;
        glm::vec3 m_specular_color; 
        float     m_smoothness;
        float     m_reflectivity;
        float     m_translucency;
        float     m_refractive_index;
    };

    struct dirlight_data
    {
        glm::vec3 m_ambient_color;
        glm::vec3 m_diffuse_color;
        glm::vec3 m_specular_color;
        glm::vec3 m_direction_cameraspace;
    };

    struct point_light_data
    {
        glm::vec3 m_position_cameraspace;
        glm::vec3 m_ambient_color;
        glm::vec3 m_diffuse_color;
        glm::vec3 m_specular_color;
        float     m_constant_attenuation;
        float     m_linear_attenuation;
        float     m_quadratic_attenuation;
    };

    typedef std::vector<point_light_data> point_light_vector;

    enum class program_type
    {
        phong,
        environment_mapping,
        skybox
    };

    enum class depth_func
    {
        never,    // Never passes.
        less,     // Passes if the incoming depth value is less than the stored depth value. This is the default behavior.
        equal,    // Passes if the incoming depth value is equal to the stored depth value.
        lequal,   // Passes if the incoming depth value is less than or equal to the stored depth value.
        greater,  // Passes if the incoming depth value is greater than the stored depth value.
        notequal, // Passes if the incoming depth value is not equal to the stored depth value.
        gequal,   // Passes if the incoming depth value is greater than or equal to the stored depth value.
        always    // Always passes.
    };

    struct gl_node_context
    {
        gl_node_context() :
            m_material(),
            m_texture(0U),
            m_position_buffer(0U),
            m_texture_coords_buffer(0U),
            m_normal_buffer(0U),
            m_index_buffer(0U),
            m_num_indices(0U),
            m_model(1.0f) {}

        material_data m_material;
        gl_texture_id m_texture;
        gl_buffer_id  m_position_buffer;
        gl_buffer_id  m_texture_coords_buffer;
        gl_buffer_id  m_normal_buffer;
        gl_buffer_id  m_index_buffer;
        unsigned int  m_num_indices;
        glm::mat4     m_model;
    };

    struct gl_driver_context
    {
        gl_driver_context() :
            m_node(),
            m_cubemap(0U),
            m_program(0U),
            m_view(1.0f),
            m_projection(1.0f),
            m_dirlight(),
            m_point_lights(),
            m_depth_func(depth_func::less) {}

        gl_node_context    m_node;
        gl_cubemap_id      m_cubemap;
        gl_program_id      m_program;
        glm::mat4          m_view;
        glm::mat4          m_projection;
        dirlight_data      m_dirlight;
        point_light_vector m_point_lights;
        depth_func         m_depth_func;
    };

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to initialize the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*gl_driver_init_func)();

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to create a default texture in the graphics API.
    //! @remark The default texture is a placeholder white texture used to render objects tha
    //!  don't have a texture themselves.
    //-----------------------------------------------------------------------------------------------
    typedef void (*new_default_texture_func)(gl_texture_id* id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to delete the default texture from the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*delete_default_texture_func)(gl_texture_id id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to create a texture in the graphics API.
    //! @remark The data must be layed out in memory in the following way:
    //!  row-major (one scanline, then another, and so on)
    //!  vertically flipped (the first scanline in memory is the bottom of the image)
    //-----------------------------------------------------------------------------------------------
    typedef void (*new_texture_func)(unsigned int width,
                                      unsigned int height,
                                      image_format format,
                                      const unsigned char* data,
                                      gl_texture_id* id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to delete a texture from the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*delete_texture_func)(gl_texture_id id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to create a 3d buffer in the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*new_3d_buffer_func)(const std::vector<glm::vec3>& data,
                                       gl_buffer_id* buffer_id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to create a 2d buffer in the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*new_2d_buffer_func)(const std::vector<glm::vec2>& data,
                                       gl_buffer_id* buffer_id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to create an index buffer in the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*new_index_buffer_func)(const std::vector<unsigned short>& data,
                                          gl_buffer_id* buffer_id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to delete a buffer from the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*delete_buffer_func)(gl_buffer_id buffer_id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to create a cubemap in the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*new_cubemap_func)(unsigned int width,
                                      unsigned int height,
                                      image_format format,
                                      const std::vector<const unsigned char*>& faces_data,
                                      gl_cubemap_id* id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to delete a cubemap from the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*delete_cubemap_func)(gl_cubemap_id id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to create a pair of shaders in the graphics API as a program.
    //-----------------------------------------------------------------------------------------------
    typedef void (*new_program_func)(program_type type,
                                     gl_program_id* gl_program_id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to delete a program from the graphics API.
    //-----------------------------------------------------------------------------------------------
    typedef void (*delete_program_func)(gl_program_id gl_program_id);

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to start a new frame.
    //-----------------------------------------------------------------------------------------------
    typedef void (*initialize_frame_func)();

    //-----------------------------------------------------------------------------------------------
    //! @brief Function type used to do a draw call.
    //-----------------------------------------------------------------------------------------------
    typedef void (*draw_func)(const gl_driver_context& context);

    struct gl_driver
    {
        gl_driver() :
            gl_driver_init(nullptr),
            new_default_texture(nullptr),
            delete_default_texture(nullptr),
            new_texture(nullptr),
            delete_texture(nullptr),
            new_3d_buffer(nullptr),
            new_2d_buffer(nullptr),
            new_index_buffer(nullptr),
            delete_buffer(nullptr),
            new_cubemap(nullptr),
            delete_cubemap(nullptr),
            new_program(nullptr),
            delete_program(nullptr),
            initialize_frame(nullptr),
            draw(nullptr) {}

        gl_driver_init_func         gl_driver_init;
        new_default_texture_func    new_default_texture;
        delete_default_texture_func delete_default_texture;
        new_texture_func            new_texture;
        delete_texture_func         delete_texture;
        new_3d_buffer_func          new_3d_buffer;
        new_2d_buffer_func          new_2d_buffer;
        new_index_buffer_func       new_index_buffer;
        delete_buffer_func          delete_buffer;
        new_cubemap_func            new_cubemap;
        delete_cubemap_func         delete_cubemap;
        new_program_func            new_program;
        delete_program_func         delete_program;
        initialize_frame_func       initialize_frame;
        draw_func                   draw;
    };
} // namespace cgs

#endif // GL_DRIVER_HPP
