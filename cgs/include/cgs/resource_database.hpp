#ifndef RESOURCE_DATABASE_HPP
#define RESOURCE_DATABASE_HPP

#include "glm/glm.hpp"

#include <cstddef> // for size_t
#include <vector>
#include <string>

namespace cgs
{
  //-----------------------------------------------------------------------------------------------
  // Types
  //-----------------------------------------------------------------------------------------------

  //-----------------------------------------------------------------------------------------------
  //! @brief Handle to a material.
  //-----------------------------------------------------------------------------------------------
  typedef std::size_t mat_id;

  //-----------------------------------------------------------------------------------------------
  //! @brief Handle to a mesh.
  //-----------------------------------------------------------------------------------------------
  typedef std::size_t mesh_id;

  //-----------------------------------------------------------------------------------------------
  //! @brief Integral type used to represent indexes in an array.
  //-----------------------------------------------------------------------------------------------
  typedef unsigned short vindex;

  //-----------------------------------------------------------------------------------------------
  //! @brief Handle to a resource.
  //-----------------------------------------------------------------------------------------------
  typedef std::size_t resource_id;

  //-----------------------------------------------------------------------------------------------
  // Constants
  //-----------------------------------------------------------------------------------------------

  //-----------------------------------------------------------------------------------------------
  //! @brief Constant representing 'not a material'. Used as a wildcard when iterating materials
  //!  to indicate the end of the sequence has been reached.
  //-----------------------------------------------------------------------------------------------
  constexpr mat_id nmat = -1;

  //-----------------------------------------------------------------------------------------------
  //! @brief Constant representing 'not a mesh'. Used as a wildcard when iterating meshes to
  //!  indicate the end of the sequence has been reached.
  //-----------------------------------------------------------------------------------------------
  constexpr mesh_id nmesh = -1;

  //-----------------------------------------------------------------------------------------------
  //! @brief Constant representing 'not a resource'. Used as a wildcard when iterating resources to
  //!  indicate the end of the sequence has been reached.
  //-----------------------------------------------------------------------------------------------
  constexpr resource_id nresource = -1;

  //-----------------------------------------------------------------------------------------------
  //! @brief Id of the root resource. This resource is the root of the resource hierarchy.
  //!  It always exists.
  //-----------------------------------------------------------------------------------------------
  constexpr resource_id root_resource = 0;

  //-----------------------------------------------------------------------------------------------
  //! @brief Constant indicating the maximum number of meshes a resource can hold.
  //-----------------------------------------------------------------------------------------------
  constexpr std::size_t max_meshes_by_resource = 10;

  //-----------------------------------------------------------------------------------------------
  //! @brief Initializes the resource database.
  //! @remarks This function can be called any number of times during program execution.
  //!  Calling it erases all materials, meshes and resources that might exist.
  //-----------------------------------------------------------------------------------------------
  void resource_database_init();

  //-----------------------------------------------------------------------------------------------
  //! @brief Creates a new material.
  //! @return The id of the new material.
  //! @remarks The new material has color diffuse = 1, 1, 1, color specular = 1, 1, 1
  //!  and smoothness = 1.
  //-----------------------------------------------------------------------------------------------
  mat_id add_material();

  void set_material_diffuse_color(mat_id mat, glm::vec3 diffuse_color);
  void set_material_specular_color(mat_id mat, glm::vec3 specular_color);
  void set_material_smoothness(mat_id mat, float smoothness);
  void set_material_texture_path(mat_id mat, const std::string& texture_path);
  glm::vec3 get_material_diffuse_color(mat_id mat);
  glm::vec3 get_material_specular_color(mat_id mat);
  float get_material_smoothness(mat_id mat);
  std::string get_material_texture_path(mat_id mat);

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns the id of the first material in the sequence of existing materials.
  //! @return The id of the material.
  //-----------------------------------------------------------------------------------------------
  mat_id get_first_material();

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns the id of the material that comes next to a given material in the sequence.
  //! @param material The material to query.
  //! @return The id of the next material, of nmaterial of material is the last material.
  //-----------------------------------------------------------------------------------------------
  mat_id get_next_material(mat_id material);

  //-----------------------------------------------------------------------------------------------
  //! @brief Creates a new mesh.
  //! @return The id of the new mesh.
  //! @remarks The new mesh hast the following properties:
  //!  It has 0 vertices
  //!  Its vertext stride is 3
  //!  It has no texture coordinates
  //!  Its texture coordinate stride is 2
  //!  It has no faces
  //!  Its faces stride is 3
  //!  It has no material associated to it
  //-----------------------------------------------------------------------------------------------
  mesh_id add_mesh();

  void set_mesh_vertices(mesh_id mesh, const std::vector<glm::vec3>& vertices);
  void set_mesh_texture_coords(mesh_id mesh, const std::vector<glm::vec2>& texture_coords);
  void set_mesh_normals(mesh_id mesh, const std::vector<glm::vec3>& normals);
  void set_mesh_indices(mesh_id mesh, const std::vector<vindex>& indices);
  void set_mesh_material(mesh_id mesh, mat_id material);
  std::vector<glm::vec3> get_mesh_vertices(mesh_id mesh);
  std::vector<glm::vec2> get_mesh_texture_coords(mesh_id mesh);
  std::vector<glm::vec3> get_mesh_normals(mesh_id mesh);
  std::vector<vindex> get_mesh_indices(mesh_id mesh);
  mat_id get_mesh_material(mesh_id mesh);

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns the id of the first mesh in the sequence of all meshes.
  //! @return The id of the mesh, or nmesh if there are no meshes.
  //-----------------------------------------------------------------------------------------------
  mesh_id get_first_mesh();

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns the id of the mesh following a given mesh in the sequence of all meshes.
  //! @param mesh The mesh to query.
  //! @return The id of the next mesh, or nmesh if mesh is the last mesh.
  //-----------------------------------------------------------------------------------------------
  mesh_id get_next_mesh(mesh_id mesh);

  //-----------------------------------------------------------------------------------------------
  //! @brief Creates a new resource node.
  //! @param parent Id of the resource node to which the new resource node should be added as child.
  //! @return Id of the new node.
  //! @remarks The new resource node has the following properties:
  //!  its local transform is the identity.
  //!  it contains has no meshes
  //-----------------------------------------------------------------------------------------------
  resource_id add_resource(resource_id parent);

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the properties of a resource node.
  //! @param r Id of the resource node to query.
  //! @param meshes Address of a pointer to store the address of an internal array containing the
  //!  list of meshes in the resource node. Can't be nullptr.
  //! @param num_meshes Address of an object to store the number of meshes contained in the
  //!  resource node. Can't be nullptr.
  //! @param local_transform Address of a pointer to store the address of an internal array
  //!  representing the resource's local transform as a matrix in column major format (that is,
  //!  one column, then the other, and so on). Can't be nullptr.
  //! @remarks See set_resource_properties for the meaning of all these properties.
  //-----------------------------------------------------------------------------------------------
  void get_resource_properties(resource_id r, const mesh_id** meshes, std::size_t* num_meshes, const float** local_transform);

  //-----------------------------------------------------------------------------------------------
  //! @brief Sets the properties of a resource node.
  //! @param r Id of the resource node to set.
  //! @param meshes An array of num_meshes elements containing the list of meshes that should
  //!  be contained in the resource node. Can't be nullptr.
  //! @param num_meshes The number of elements of the meshes array. Can be zero to indicate an
  //!  empty set of meshes.
  //! @param local_transform An array containing the local transform that should be assigned to
  //!  the resource node, in column-major format (that is, one column, then the other, and so on),
  //! expressed in the coordinate system of the  parent resource node.
  //-----------------------------------------------------------------------------------------------
  void set_resource_properties(resource_id r, mesh_id* meshes, std::size_t num_meshes, float* local_transform);

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns the id of the first resource node in the children of a given resource node.
  //! @param resource Id of the resource to query.
  //! @return Id of the resource's first child resource, or nresource if there are no children.
  //-----------------------------------------------------------------------------------------------
  resource_id get_first_child_resource(resource_id resource);

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns the id of the next sibling of a given resource node.
  //! @param resource Id of the resource node to query.
  //! @return Id of the next sibling, or nresource if there are no siblings.
  //-----------------------------------------------------------------------------------------------
  resource_id get_next_sibling_resource(resource_id resource);
} // namespace cgs

#endif // RESOURCE_DATABASE_HPP
