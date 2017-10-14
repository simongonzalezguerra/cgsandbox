#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "cgs/scenegraph.hpp"
#include "cgs/gl_driver.hpp"

namespace cgs
{
    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void set_gl_driver(const gl_driver& driver);
    bool initialize_renderer_old();
    void finalize_renderer_old();
    void render_old(view_id view);
    bool initialize_renderer();
    void finalize_renderer();
    void render(view_id view);
} // namespace cgs

#endif // RENDERER_HPP
