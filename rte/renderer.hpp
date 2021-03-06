#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "gl_driver_util.hpp"
#include "rte_domain.hpp"

namespace rte
{
    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void set_gl_driver(const gl_driver& driver);
    void initialize_renderer(view_database& db);
    void finalize_renderer();
    void render(const view_database& db);
} // namespace rte

#endif // RENDERER_HPP
