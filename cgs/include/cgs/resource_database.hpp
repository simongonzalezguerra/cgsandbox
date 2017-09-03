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
    //! @brief Handle to a cubemap.
    //-----------------------------------------------------------------------------------------------
    typedef std::size_t cubemap_id;

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
    //! @brief Constant representing 'not a cubemap'. Used as a wildcard when iterating the
    //!  cubemaps to indicate the end of the sequence has been reached.
    //-----------------------------------------------------------------------------------------------
    constexpr cubemap_id ncubemap = -1;

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

    void set_resource_meshes(resource_id r, const std::vector<mesh_id>& meshes);
    void set_resource_local_transform(resource_id r, const glm::mat4& local_transform);
    std::vector<mesh_id> get_resource_meshes(resource_id r);
    glm::mat4 get_resource_local_transform(resource_id r);

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

    cubemap_id add_cubemap();
    void set_cubemap_faces(cubemap_id id, const std::vector<std::string>& faces);
    std::vector<std::string> get_cubemap_faces(cubemap_id id);
    cubemap_id get_first_cubemap();
    cubemap_id get_next_cubemap(cubemap_id id);
} // namespace cgs

#endif // RESOURCE_DATABASE_HPP
