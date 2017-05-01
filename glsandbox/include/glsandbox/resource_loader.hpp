#ifndef RESOURCE_LOADER_HPP
#define RESOURCE_LOADER_HPP

#include "glsandbox/resource_database.hpp"

#include <cstddef> // for size_t
#include <string>

namespace glsandbox
{
  //-----------------------------------------------------------------------------------------------
  //! @brief Loads resources from a file (materials, meshes and resource nodes).
  //! @param file_name Path to the file to load from relative to the current directory.
  //! @param materials out Address of a pointer in which to store the address of an internal
  //!  array containing the ids of the materials which were created from the file. The contents
  //!  of this internal array will remain valid until the next call to this function.
  //!  Can't ne nullptr.
  //! @param num_materials_out Address of a variable in which to store the number of materials
  //!  which were created from the file. Can't be nullptr.
  //! @param meshes_out Address of a variable in which to store the address of an internal
  //!  array containing the ids of the meshes which were created from the file. The contents
  //!  of this internal array will remain valid until the next call to this function.
  //!  Can't be nullptr.
  //! @param num_meshes_out Address of a variable in which to store the number of meshes
  //!  which were created from the file. Can't be nullptr.
  //! @return The id of the root resource created from the file.
  //-----------------------------------------------------------------------------------------------
  resource_id load_resources(const std::string& file_name,
                             const mat_id** materials_out,
                             std::size_t* num_materials_out,
                             const mesh_id** meshes_out,
                             std::size_t* num_meshes_out);
} // namespace glsandbox

#endif // RESOURCE_LOADER_HPP
