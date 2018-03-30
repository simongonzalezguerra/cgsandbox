#ifndef RESOURCE_LOADER_HPP
#define RESOURCE_LOADER_HPP

#include "rte_domain.hpp"

#include <cstddef> // for size_t
#include <vector>
#include <string>

namespace rte
{
    void load_resources(const std::string& file_name, index_type& root_index_out, view_database& db);
} // namespace rte

#endif // RESOURCE_LOADER_HPP
