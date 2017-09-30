#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "cgs/scenegraph.hpp"

#include <cstddef>  // for std::size_t
#include <vector>

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
