#ifndef RESOURCE_DATABASE_HPP
#define RESOURCE_DATABASE_HPP

#include <cstddef> // for size_t

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

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the properties of a material.
  //! @param material Id of a material.
  //! @param color_diffuse Object to store the color diffuse of the material.
  //! @param color_spec Object to store the color specular of the material.
  //! @param smoothness Object to store the smoothness parameter of the material.
  //! @param texture_path Address of a pointer in which to store the address of a internal array
  //!  containing the texture path of the material, as a null-terminated string. Can't be nullptr.
  //! @remarks The value that this function stores in texture_path can be null if the material
  //!   doesn't have a texture associated with it.
  //-----------------------------------------------------------------------------------------------
  void get_material_properties(mat_id material,
                               float color_diffuse[3],
                               float color_spec[3],
                               float& smoothness,
                               const char** texture_path);

  //-----------------------------------------------------------------------------------------------
  //! @brief Sets the properties of a material.
  //! @param material Id of a material.
  //! @param color_diffuse Color to set as diffuse color of the material.
  //! @param color_spec Color to set as specular color of the material.
  //! @param smoothness Value to set as smoothness parameter of the material.
  //! @param texture_path The path of the texture associated to this material, if any.
  //!  Can be nullptr to indicate there is no texture.
  //-----------------------------------------------------------------------------------------------
  void set_material_properties(mat_id material,
                               float color_diffuse[3],
                               float color_spec[3],
                               float smoothness,
                               const char* texture_path);

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

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the properties of a mesh.
  //! @param mesh Id of the mesh to query.
  //! @param vertex_base Address of a pointer in which to store the address of an internal array
  //!  containing vertex coordinates. Can't be nulltptr.
  //! @param vertex_stride Address of an object in which to store the vertex stride. Can't be
  //!   nullptr.
  //! @param texture_coords Address of a pointer in which to store the address of an internal
  //!  array containing texture coordinates. Can't be nulltptr.
  //! @param texture_coords_stride Address of an object in which to store the texture coordinate
  //!  stride. Can't be nullptr.
  //! @param num_vertices Address of an object in which to store the number of vertices. Can't
  //!  be nullptr.
  //! @param faces Address of a pointer in which to store the address of an internal array
  //!  containing face indices. Can't be nulltptr.
  //! @param faces_stride Address of an object in which to store the faces stride. Can't be
  //!  nullptr.
  //! @param num_faces Address of an object in which to store the number of faces. Can't be
  //!  nullptr.
  //! @param normals Address of a pointer in which to store the address of an internal array
  //!  containing normals. Can't be nulltptr.
  //! @param material Address of an object in which to store the id of the material associated
  //!  to the mesh. Can't be nullptr.
  //! @remarks See set_mesh_properties for the meaning of all the properties.
  //! @remarks The value stored in the texture_coords pointer can be nullptr, even when the mesh
  //! has vertices. This is used to indicate the mesh has no texture coordinates.
  //! @remarks The value stored in the normals pointer can be nullptr, even when the mesh
  //! has vertices. This is used to indicate the mesh has no normal data.
  //-----------------------------------------------------------------------------------------------
  void get_mesh_properties(mesh_id mesh,
                          const float** vertex_base,
                          std::size_t* vertex_stride,
                          const float** texture_coords,
                          std::size_t* texture_coords_stride,
                          std::size_t* num_vertices,
                          const vindex** faces,
                          std::size_t* faces_stride,
                          std::size_t* num_faces,
                          const float** normals,
                          mat_id* material);

  //-----------------------------------------------------------------------------------------------
  //! @brief Sets the properties of a mesh.
  //! @param mesh Id of the mesh to set.
  //! @param vertex_base Array containing vertex coordinates. Can't be nulltptr.
  //! @param vertex_stride Number of elements of vertex_base representing each vertex (usually 3,
  //!  but can be 2 for 2D meshes). Must be a positive number.
  //! @param texture_coords Array containing the texture coordinates of each vertex, in the same
  //!  order as the vertex_base array. Can be nullptr, in which case the texture_coords_stride
  //!  parameter is ignored and the texture coordinates of the mesh are left unchanged.
  //! @param texture_coords_stride Number of elements of texture_coords coorresponding to each
  //!  vertex (usually 2, but can be higher for some specific texture data channels). If
  //!  texture_coords is not nullptr then texture_coord_stride must be a positive number.
  //! @param num_vertices Number of vertices. Must be a positive number.
  //! @param faces Array containing the faces, as indexes over the logical vertex array formed
  //!  by splitting vertex base in blocks of vertex_stride elements. Can't be nullptr.
  //! @param faces_stride Number of elements in faces corresponding to each face. Usually 3
  //!  which means the mesh is made of triangles. Must be a positive number.
  //! @param num_faces Number of faces. Must be a positive number.
  //! @param normals Array containing vertex normals. Can't be nulltptr.
  //! @param material Id of the material associated to the mesh. Can be nmaterial.
  //-----------------------------------------------------------------------------------------------
  void set_mesh_properties(mesh_id mesh,
                          const float* vertex_base,
                          std::size_t vertex_stride,
                          float* texture_coords,
                          std::size_t texture_coords_stride,
                          std::size_t num_vertices,
                          const vindex* faces,
                          std::size_t faces_stride,
                          std::size_t num_faces,
                          const float* normals,
                          mat_id material);

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
