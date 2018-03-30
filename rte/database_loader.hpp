#ifndef DATABASE_LOADER_HPP
#define DATABASE_LOADER_HPP

#include "rte_domain.hpp"

namespace rte
{
    void database_loader_initialize();
    // Any existing data in db is removed
    void load_database(view_database& db);
    void log_database(const view_database& db);
    void database_loader_finalize();
} // namespace rte

#endif // DATABASE_LOADER_HPP
