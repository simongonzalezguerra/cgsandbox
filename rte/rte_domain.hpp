#ifndef RTE_DOMAIN_HPP
#define RTE_DOMAIN_HPP

#include "sparse_tree.hpp"
#include "rte_common.hpp"
#include "glm/glm.hpp"

#include <memory>

namespace rte
{
    struct material : public sparse_node
    {
        material() :
            m_diffuse_color({1.0f, 1.0f, 1.0f}),
            m_specular_color({1.0f, 1.0f, 1.0f}),
            m_smoothness(1.0f),
            m_texture_path(),
            m_reflectivity(0.0f),
            m_translucency(0.0f),
            m_refractive_index(1.0f),
            m_texture_id(0U),
            m_user_id(nuser_id),
            m_name() {}
    
        glm::vec3          m_diffuse_color;
        glm::vec3          m_specular_color;
        float              m_smoothness;
        std::string        m_texture_path;
        float              m_reflectivity;
        float              m_translucency;
        float              m_refractive_index;
        gl_texture_id      m_texture_id;
        user_id            m_user_id;
        std::string        m_name;
    };

    typedef sparse_vector<material> material_database;

    struct mesh : public sparse_node
    {
        mesh() :
            m_position_buffer_id(0U),
            m_uv_buffer_id(0U),
            m_normal_buffer_id(0U),
            m_index_buffer_id(0U),
            m_num_vertices(0U),
            m_user_id(nuser_id),
            m_name() {}
    
        mesh(const mesh& m) = default;

        mesh(mesh&& m) :
            m_position_buffer_id(std::move(m.m_position_buffer_id)),
            m_uv_buffer_id(std::move(m.m_uv_buffer_id)),
            m_normal_buffer_id(std::move(m.m_normal_buffer_id)),
            m_index_buffer_id(std::move(m.m_index_buffer_id)),
            m_num_vertices(std::move(m.m_num_vertices)),
            m_user_id(std::move(m.m_user_id)),
            m_name(std::move(m.m_name)) {}

        mesh& operator=(const mesh& m) = default;

        mesh& operator=(mesh&& m)
        {
            if (&m != this) {
                m_position_buffer_id = std::move(m.m_position_buffer_id);
                m_uv_buffer_id = std::move(m.m_uv_buffer_id);
                m_normal_buffer_id = std::move(m.m_normal_buffer_id);
                m_index_buffer_id = std::move(m.m_index_buffer_id);
                m_num_vertices = std::move(m.m_num_vertices);
                m_user_id = std::move(m.m_user_id);
                m_name = std::move(m.m_name);            
            }

            return *this;            
        }

        gl_buffer_id                m_position_buffer_id;  //!< id of the position buffer in the graphics API
        gl_buffer_id                m_uv_buffer_id;        //!< id of the uv buffer in the graphics API
        gl_buffer_id                m_normal_buffer_id;    //!< id of the normal buffer in the graphics API
        gl_buffer_id                m_index_buffer_id;     //!< id of the index buffer in the graphics API
        unsigned int                m_num_vertices;        //!< number of vertices of this mesh
        user_id                     m_user_id;             //!< user id of this mesh
        std::string                 m_name;                //!< name of this mesh
    };

    typedef sparse_vector<mesh> mesh_database;

    struct mesh_buffer : public sparse_node
    {
        mesh_buffer() :
            m_mesh(npos),
            m_vertices(),
            m_texture_coords(),
            m_normals(),
            m_indices() {}
    
        mesh_buffer(const mesh_buffer& m) = default;

        mesh_buffer(mesh_buffer&& m) :
            m_mesh(std::move(m.m_mesh)),
            m_vertices(std::move(m.m_vertices)),
            m_texture_coords(std::move(m.m_texture_coords)),
            m_normals(std::move(m.m_normals)),
            m_indices(std::move(m.m_indices)) {}

        mesh_buffer& operator=(const mesh_buffer& m) = default;

        mesh_buffer& operator=(mesh_buffer&& m)
        {
            if (&m != this) {
                m_mesh = std::move(m.m_mesh);
                m_vertices = std::move(m.m_vertices);
                m_texture_coords = std::move(m.m_texture_coords);
                m_normals = std::move(m.m_normals);
                m_indices = std::move(m.m_indices);
            }

            return *this;            
        }

        index_type                  m_mesh;                //!< index of the mesh these buffers belong to
        std::vector<glm::vec3>      m_vertices;            //!< vertex coordinates
        std::vector<glm::vec2>      m_texture_coords;      //!< texture coordinates for each vertex
        std::vector<glm::vec3>      m_normals;             //!< normals of the mesh
        std::vector<vindex>         m_indices;             //!< faces, as a sequence of indexes over the logical vertex array
    };

    typedef sparse_vector<mesh_buffer> mesh_buffer_database;

    struct resource : public sparse_node
    {
        resource() :
            m_mesh(npos),
            m_material(npos),
            m_local_transform(1.0f),
            m_user_id(nuser_id),
            m_name() {}
     
        index_type       m_mesh;               //!< mesh contained in this resource
        index_type       m_material;           //!< material of this resource
        glm::mat4        m_local_transform;    //!< resource transform relative to the parent's reference frame
        user_id          m_user_id;            //!< user id of this resource
        std::string      m_name;               //!< name of this resource
    };

    typedef sparse_vector<resource> resource_database;

    struct cubemap : public sparse_node
    {
        cubemap() :
            m_faces(),
            m_gl_cubemap_id(0U),
            m_user_id(nuser_id),
            m_name() {}
    
        std::vector<std::string>      m_faces;          //!< paths to image files containing the six faces of the cubemap
        gl_cubemap_id                 m_gl_cubemap_id;  //!< id of this cubemap in the graphics API
        user_id                       m_user_id;        //!< user id of this cubemap
        std::string                   m_name;           //!< name of this resource
    };

    typedef sparse_vector<cubemap> cubemap_database;

    struct node : public sparse_node
    {    
        node() :
            m_mesh(npos),
            m_material(npos),
            m_local_transform(1.0f),
            m_accum_transform(1.0f),
            m_enabled(true),
            m_user_id(nuser_id),
            m_name() {}
    
        index_type       m_mesh;             //!< mesh contained in this node
        index_type       m_material;         //!< material of this node
        glm::mat4        m_local_transform;  //!< node transform relative to the parent
        glm::mat4        m_accum_transform;  //!< node transform relative to the root
        bool             m_enabled;          //!< is this node enabled? (if it is not, all descendants are ignored when rendering)
        user_id          m_user_id;          //!< user id of this node
        std::string      m_name;             //!< name of this node
    };

    typedef sparse_vector<node> node_database;

    struct point_light : public sparse_node
    {
        point_light() :
            m_position(0.0f),
            m_ambient_color(0.0f),
            m_diffuse_color(0.0f),
            m_specular_color(0.0f),
            m_constant_attenuation(0.0f),
            m_linear_attenuation(0.0f),
            m_quadratic_attenuation(0.0f),
            m_user_id(nuser_id) {}
    
        glm::vec3        m_position;
        glm::vec3        m_ambient_color;
        glm::vec3        m_diffuse_color;
        glm::vec3        m_specular_color;
        float            m_constant_attenuation;
        float            m_linear_attenuation;
        float            m_quadratic_attenuation;
        user_id          m_user_id;
        std::string      m_name;
    };

    typedef sparse_vector<point_light> point_light_database;

    struct dirlight : public sparse_node
    {
        dirlight() :
            m_ambient_color(),
            m_diffuse_color(),
            m_specular_color(),
            m_direction() {}

        glm::vec3      m_ambient_color;  //!< ambient color of the directional light
        glm::vec3      m_diffuse_color;  //!< diffuse color of the directional light
        glm::vec3      m_specular_color; //!< specular color of the directional light
        glm::vec3      m_direction;      //!< direction of the directional light
    };

    struct view_database : public sparse_node
    {
        view_database() = default;

        view_database(const view_database& vbd) = default;

        view_database(view_database&& vdb) :
            m_materials(std::move(vdb.m_materials)),
            m_meshes(std::move(vdb.m_meshes)),
            m_mesh_buffers(std::move(vdb.m_mesh_buffers)),
            m_resources(std::move(vdb.m_resources)),
            m_cubemaps(std::move(vdb.m_cubemaps)),
            m_nodes(std::move(vdb.m_nodes)),
            m_point_lights(std::move(vdb.m_point_lights)),
            m_root_node(std::move(vdb.m_root_node)),
            m_view_transform(std::move(vdb.m_view_transform)),
            m_projection_transform(std::move(vdb.m_projection_transform)),
            m_skybox(std::move(vdb.m_skybox)),
            m_dirlight(std::move(vdb.m_dirlight)) {}

        view_database& operator=(const view_database&) = default;

        view_database& operator=(view_database&& vdb)
        {
            if (&vdb != this) {
                m_materials = std::move(vdb.m_materials);
                m_meshes = std::move(vdb.m_meshes);
                m_mesh_buffers = std::move(vdb.m_mesh_buffers);
                m_resources = std::move(vdb.m_resources);
                m_cubemaps = std::move(vdb.m_cubemaps);
                m_nodes = std::move(vdb.m_nodes);
                m_point_lights = std::move(vdb.m_point_lights);
                m_root_node = std::move(vdb.m_root_node);
                m_view_transform = std::move(vdb.m_view_transform);
                m_projection_transform = std::move(vdb.m_projection_transform);
                m_skybox = std::move(vdb.m_skybox);
                m_dirlight = std::move(vdb.m_dirlight);
            }

            return *this;
        }

        material_database         m_materials;
        mesh_database             m_meshes;
        mesh_buffer_database      m_mesh_buffers;
        resource_database         m_resources;
        cubemap_database          m_cubemaps;
        node_database             m_nodes;
        point_light_database      m_point_lights;
        index_type                m_root_node;
        glm::mat4                 m_view_transform;                   //!< the view transform used to render all objects in the scene
        glm::mat4                 m_projection_transform;             //!< the projection transform used to render all objects in the scene
        index_type                m_skybox;                           //!< the id of the cubemap to use as skybox (can be npos)
        dirlight                  m_dirlight;                         //!< directional light
    };

    void log_materials(const view_database& db);
    void log_meshes(const view_database& db);
    void log_resources(const view_database& db);
    void log_cubemaps(const view_database& db);
    void log_nodes(const view_database& db);
    void log_directional_light(const view_database& db);
    void log_point_lights(const view_database& db);
    void log_database(const view_database& db);
    // resource_index can be npos, in that case insert_node_tree() creates a tree with a single, empty node
    void insert_node_tree(index_type resource_index,
                        index_type parent_index,
                        index_type& node_index_out,
                        view_database& db);
    void get_descendant_nodes(index_type node_index,
                        std::vector<index_type>& nodes_out,
                        const view_database& db);
} // namespace rte

#endif // RTE_DOMAIN_HPP
