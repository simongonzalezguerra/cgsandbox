#ifndef RESOURCE_DATABASE_HPP
#define RESOURCE_DATABASE_HPP

#include "rte_common.hpp"
#include "glm/glm.hpp"

#include <cstddef> // for size_t
#include <memory>
#include <vector>
#include <string>

namespace rte
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
    mat_id new_material();
    void delete_material(mat_id mat);
    void set_material_diffuse_color(mat_id mat, glm::vec3 diffuse_color);
    void set_material_specular_color(mat_id mat, glm::vec3 specular_color);
    void set_material_smoothness(mat_id mat, float smoothness);
    void set_material_texture_path(mat_id mat, const std::string& texture_path);
    void set_material_reflectivity(mat_id mat, float reflectivity);
    void set_material_translucency(mat_id mat, float translucency);
    void set_material_refractive_index(mat_id mat, float refractive_index);
    void set_material_texture_id(mat_id mat, gl_texture_id texture_id);
    void set_material_user_id(mat_id mat, user_id id);
    void set_material_name(mat_id, const std::string& name);
    glm::vec3 get_material_diffuse_color(mat_id mat);
    glm::vec3 get_material_specular_color(mat_id mat);
    float get_material_smoothness(mat_id mat);
    std::string get_material_texture_path(mat_id mat);
    float get_material_reflectivity(mat_id mat);
    float get_material_translucency(mat_id mat);
    float get_material_refractive_index(mat_id mat);
    gl_texture_id get_material_texture_id(mat_id mat);
    user_id get_material_user_id(mat_id mat);
    std::string get_material_name(mat_id);

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

    struct material_handle
    {
        material_handle() : m_material_id(nmat) {}
        material_handle(mat_id material_id) : m_material_id(material_id) {}
        material_handle(std::nullptr_t) : m_material_id(nmat) {}
        operator int() {return m_material_id;}
        operator mat_id() {return m_material_id;}
        bool operator ==(const material_handle &other) const {return m_material_id == other.m_material_id;}
        bool operator !=(const material_handle &other) const {return m_material_id != other.m_material_id;}
        bool operator ==(std::nullptr_t) const {return m_material_id == nmat;}
        bool operator !=(std::nullptr_t) const {return m_material_id != nmat;}

        mat_id m_material_id;
    };

    struct material_deleter
    {
        typedef material_handle pointer;
        material_deleter() {}
        template<class other> material_deleter(const other&) {};
        void operator()(pointer p) const { delete_material(p); }
    };

    typedef std::unique_ptr<mat_id, material_deleter> unique_material;
    typedef std::vector<unique_material> material_vector;
    unique_material make_material();
    void log_materials();

    mesh_id new_mesh();
    void delete_mesh(mesh_id mesh);
    void set_mesh_vertices(mesh_id mesh, const std::vector<glm::vec3>& vertices);
    void set_mesh_texture_coords(mesh_id mesh, const std::vector<glm::vec2>& texture_coords);
    void set_mesh_normals(mesh_id mesh, const std::vector<glm::vec3>& normals);
    void set_mesh_indices(mesh_id mesh, const std::vector<vindex>& indices);    
    void set_mesh_position_buffer_id(mesh_id mesh, gl_buffer_id position_buffer_id);
    void set_mesh_uv_buffer_id(mesh_id mesh, gl_buffer_id uv_buffer_id);
    void set_mesh_normal_buffer_id(mesh_id mesh, gl_buffer_id normal_buffer_id);
    void set_mesh_index_buffer_id(mesh_id mesh, gl_buffer_id index_buffer_id);
    void set_mesh_user_id(mesh_id mesh, user_id id);
    void set_mesh_name(mesh_id mesh, const std::string& name);
    std::vector<glm::vec3> get_mesh_vertices(mesh_id mesh);
    std::vector<glm::vec2> get_mesh_texture_coords(mesh_id mesh);
    std::vector<glm::vec3> get_mesh_normals(mesh_id mesh);
    std::vector<vindex> get_mesh_indices(mesh_id mesh);
    gl_buffer_id get_mesh_position_buffer_id(mesh_id mesh);
    gl_buffer_id get_mesh_uv_buffer_id(mesh_id mesh);
    gl_buffer_id get_mesh_normal_buffer_id(mesh_id mesh);
    gl_buffer_id get_mesh_index_buffer_id(mesh_id mesh);
    user_id get_mesh_user_id(mesh_id mesh);
    std::string get_mesh_name(mesh_id mesh);
    void log_meshes();

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

    struct mesh_handle
    {
        mesh_handle() : m_mesh_id(nmesh) {}
        mesh_handle(mesh_id mesh_id) : m_mesh_id(mesh_id) {}
        mesh_handle(std::nullptr_t) : m_mesh_id(nmesh) {}
        operator int() {return m_mesh_id;}
        operator mesh_id() {return m_mesh_id;}
        bool operator ==(const mesh_handle &other) const {return m_mesh_id == other.m_mesh_id;}
        bool operator !=(const mesh_handle &other) const {return m_mesh_id != other.m_mesh_id;}
        bool operator ==(std::nullptr_t) const {return m_mesh_id == nmesh;}
        bool operator !=(std::nullptr_t) const {return m_mesh_id != nmesh;}

        mesh_id m_mesh_id;
    };

    struct mesh_deleter
    {
        typedef mesh_handle pointer;
        mesh_deleter() {}
        template<class other> mesh_deleter(const other&) {};
        void operator()(pointer p) const { delete_mesh(p); }
    };

    typedef std::unique_ptr<mesh_id, mesh_deleter> unique_mesh;
    typedef std::vector<unique_mesh> mesh_vector;
    unique_mesh make_mesh();

    //-----------------------------------------------------------------------------------------------
    //! @brief Creates a new resource node.
    //! @param parent Id of the resource node to which the new resource node should be added as child.
    //! @return Id of the new node.
    //! @remarks The new resource node has the following properties:
    //!  its local transform is the identity.
    //!  it contains has no meshes
    //-----------------------------------------------------------------------------------------------
    resource_id new_resource();
    resource_id new_resource(resource_id parent);
    void delete_resource(resource_id r);
    void set_resource_local_transform(resource_id r, const glm::mat4& local_transform);
    glm::mat4 get_resource_local_transform(resource_id r);
    void set_resource_mesh(resource_id r, mesh_id m);
    mesh_id get_resource_mesh(resource_id r);
    void set_resource_material(resource_id r, mat_id mat);
    mat_id get_resource_material(resource_id r);
    void set_resource_user_id(resource_id r, user_id id);
    user_id get_resource_user_id(resource_id r);
    void set_resource_name(resource_id r, const std::string& name);
    std::string get_resource_name(resource_id r);

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

    void log_resources();

    struct resource_handle
    {
        resource_handle() : m_resource_id(nresource) {}
        resource_handle(resource_id resource_id) : m_resource_id(resource_id) {}
        resource_handle(std::nullptr_t) : m_resource_id(nresource) {}
        operator int() {return m_resource_id;}
        operator resource_id() {return m_resource_id;}
        bool operator ==(const resource_handle &other) const {return m_resource_id == other.m_resource_id;}
        bool operator !=(const resource_handle &other) const {return m_resource_id != other.m_resource_id;}
        bool operator ==(std::nullptr_t) const {return m_resource_id == nresource;}
        bool operator !=(std::nullptr_t) const {return m_resource_id != nresource;}

        resource_id m_resource_id;
    };

    struct resource_deleter
    {
        typedef resource_handle pointer;
        resource_deleter() {}
        template<class other> resource_deleter(const other&) {};
        void operator()(pointer p) const { delete_resource(p); }
    };

    typedef std::unique_ptr<resource_id, resource_deleter> unique_resource;
    typedef std::vector<unique_resource> resource_vector;
    unique_resource make_resource();
    unique_resource make_resource(resource_id p);

    cubemap_id new_cubemap();
    void delete_cubemap(cubemap_id c);
    void set_cubemap_faces(cubemap_id cid, const std::vector<std::string>& faces);
    void set_cubemap_gl_cubemap_id(cubemap_id cid, gl_cubemap_id gl_id);
    void set_cubemap_user_id(cubemap_id cid, user_id id);
    void set_cubemap_name(cubemap_id cid, const std::string& name);
    std::vector<std::string> get_cubemap_faces(cubemap_id id);
    gl_cubemap_id get_cubemap_gl_cubemap_id(cubemap_id cid);
    cubemap_id get_first_cubemap();
    cubemap_id get_next_cubemap(cubemap_id id);
    user_id get_cubemap_user_id(cubemap_id cid);
    std::string get_cubemap_name(cubemap_id cid);
    void log_cubemaps();

    struct cubemap_handle
    {
        cubemap_handle() : m_cubemap_id(ncubemap) {}
        cubemap_handle(cubemap_id cubemap_id) : m_cubemap_id(cubemap_id) {}
        cubemap_handle(std::nullptr_t) : m_cubemap_id(ncubemap) {}
        operator int() {return m_cubemap_id;}
        operator cubemap_id() {return m_cubemap_id;}
        bool operator ==(const cubemap_handle &other) const {return m_cubemap_id == other.m_cubemap_id;}
        bool operator !=(const cubemap_handle &other) const {return m_cubemap_id != other.m_cubemap_id;}
        bool operator ==(std::nullptr_t) const {return m_cubemap_id == ncubemap;}
        bool operator !=(std::nullptr_t) const {return m_cubemap_id != ncubemap;}

        cubemap_id m_cubemap_id;
    };

    struct cubemap_deleter
    {
        typedef cubemap_handle pointer;
        cubemap_deleter() {}
        template<class other> cubemap_deleter(const other&) {};
        void operator()(pointer p) const { delete_cubemap(p); }
    };

    typedef std::unique_ptr<cubemap_id, cubemap_deleter> unique_cubemap;
    typedef std::vector<unique_cubemap> cubemap_vector;
    unique_cubemap make_cubemap();

    std::string format_mesh_id(mesh_id m);
    std::string format_material_id(mat_id m);
} // namespace rte

#endif // RESOURCE_DATABASE_HPP
