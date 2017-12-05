#ifndef DATABASE_LOADER_HPP
#define DATABASE_LOADER_HPP

#include "resource_database.hpp"
#include "scenegraph.hpp"

namespace rte
{
    void database_loader_initialize();
    void load_database();
    void log_database();
    void database_loader_finalize();
} // namespace rte

#endif // DATABASE_LOADER_HPP
