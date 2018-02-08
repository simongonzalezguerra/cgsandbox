#ifndef RTE_COMMON_HPP
#define RTE_COMMON_HPP

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

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store an id that can be set by the user. Non-zero, optional, but unique
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int user_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Constant representing 'not a user id'. This is the default user id assiged to
    //!  objects that the user hasn't assigned an id to.
    //-----------------------------------------------------------------------------------------------
    constexpr user_id nuser_id = -1;
} // namespace rte

#endif // RTE_COMMON_HPP
