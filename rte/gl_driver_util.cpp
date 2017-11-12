#include "gl_driver_util.hpp"

namespace cgs
{
    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    unique_default_texture make_default_texture(const gl_driver& driver)
    {
        gl_texture_id handle = 0U;
        driver.new_default_texture(&handle);
        unique_default_texture ret(handle, default_texture_deleter(driver));
        return std::move(ret);
    }

    unique_texture make_texture(const gl_driver& driver,
                                unsigned int width,
                                unsigned int height,
                                image_format format,
                                const unsigned char* data)
    {
        gl_texture_id handle = 0U;
        driver.new_texture(width, height, format, data, &handle);
        unique_texture ret(handle, texture_deleter(driver));
        return std::move(ret);
    }

    unique_buffer make_3d_buffer(const gl_driver& driver, const std::vector<glm::vec3>& data)
    {
        gl_buffer_id handle = 0U;
        driver.new_3d_buffer(data, &handle);
        unique_buffer ret(handle, buffer_deleter(driver));
        return std::move(ret);
    }

    unique_buffer make_2d_buffer(const gl_driver& driver, const std::vector<glm::vec2>& data)
    {
        gl_buffer_id handle = 0U;
        driver.new_2d_buffer(data, &handle);
        unique_buffer ret(handle, buffer_deleter(driver));
        return std::move(ret);
    }

    unique_buffer make_index_buffer(const gl_driver& driver, const std::vector<unsigned short>& data)
    {
        gl_buffer_id handle = 0U;
        driver.new_index_buffer(data, &handle);
        unique_buffer ret(handle, buffer_deleter(driver));
        return std::move(ret);
    }

    unique_gl_cubemap make_gl_cubemap(const gl_driver& driver,
                                unsigned int width,
                                unsigned int height,
                                image_format format,
                                const std::vector<const unsigned char*>& faces_data)
    {
        gl_cubemap_id handle = 0U;
        driver.new_gl_cubemap(width, height, format, faces_data, &handle);
        unique_gl_cubemap ret(handle, gl_cubemap_deleter(driver));
        return std::move(ret);
    }

    unique_program make_program(const gl_driver& driver,
                                program_type type)
    {
        gl_program_id handle = 0U;
        driver.new_program(type, &handle);
        unique_program ret(handle, program_deleter(driver));
        return std::move(ret);
    }
} // namespace cgs
