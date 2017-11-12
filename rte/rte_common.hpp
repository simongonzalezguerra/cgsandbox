#ifndef CGS_TYPES_HPP
#define CGS_TYPES_HPP

namespace rte
{
    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to represent the format of an image in memory.
    //-----------------------------------------------------------------------------------------------
    enum class image_format
    {
        none,
        rgb,
        rgba,
        bgr,
        bgra
    };

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store a handle to a texture. Non-zero.
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int gl_texture_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store a handle to a buffer. Non-zero.
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int gl_buffer_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store a handle to a cubemap. Non-zero.
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int gl_cubemap_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store a handle to a shader program (vertex and fragment shader). Non-zero.
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int gl_program_id;
} // namespace rte

#endif // CGS_TYPES_HPP
