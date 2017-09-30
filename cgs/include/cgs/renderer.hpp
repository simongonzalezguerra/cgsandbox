#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "cgs/scenegraph.hpp"

namespace cgs
{
    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    bool initialize_renderer();
    void finalize_renderer();
    void render(view_id view);
} // namespace cgs

#endif // RENDERER_HPP
