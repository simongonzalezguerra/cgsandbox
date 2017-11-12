#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

#include "cgs/resource_database.hpp"
#include "glm/glm.hpp"

#include <cstddef> // for size_t
#include <vector>

namespace cgs
{
    //-----------------------------------------------------------------------------------------------
    // Types
    //-----------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------
    //! @brief Handle to a scene.
    //-----------------------------------------------------------------------------------------------
    typedef std::size_t scene_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Handle to a node.
    //-----------------------------------------------------------------------------------------------
    typedef std::size_t node_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Handle to a point light.
    //-----------------------------------------------------------------------------------------------
    typedef std::size_t point_light_id;

    //-----------------------------------------------------------------------------------------------
    // Constants
    //-----------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------
    //! @brief Constant representing 'not a scene'. Used as a wildcard when iterating the
    //!  scenes of a view to indicate the end of the sequence has been reached.
    //-----------------------------------------------------------------------------------------------
    constexpr scene_id nscene = -1;

    //-----------------------------------------------------------------------------------------------
    //! @brief Constant representing 'not a node'. Used as a wildcard when iterating the
    //!  children of a node to indicate the end of the sequence has been reached.
    //-----------------------------------------------------------------------------------------------
    constexpr node_id nnode = -1;

    //-----------------------------------------------------------------------------------------------
    //! @brief Constant representing 'not a point light'. Used as a wildcard when iterating the
    //!  point lights to indicate the end of the sequence has been reached.
    //-----------------------------------------------------------------------------------------------
    constexpr node_id npoint_light = -1;

    //-----------------------------------------------------------------------------------------------
    //! @brief Initializes scene graph state.
    //! @remark This function clears all existing views, scenes and nodes.
    //! @remark This function can be called several times during program execution.
    //-----------------------------------------------------------------------------------------------
    void scenegraph_init();

    //-----------------------------------------------------------------------------------------------
    //! @brief Adds a scene to a view.
    //! @param view Id of the view to add the scene to.
    //! @return Id of the new scene, or nscene on error.
    //-----------------------------------------------------------------------------------------------
    scene_id add_scene();

    void remove_scene(scene_id scene);

    //-----------------------------------------------------------------------------------------------
    //! @brief Returns the id of the first scene of a view.
    //! @param view The view to query.
    //! @return Id of the first scene if the view. If the view has no scenes, or an error occurs,
    //!  nscene is returned.
    //-----------------------------------------------------------------------------------------------
    scene_id get_first_scene();

    //-----------------------------------------------------------------------------------------------
    //! @brief Returns the id of the next scene in the sequence of scenes of a view.
    //! @param scene Current scene.
    //! @return The id of the next scene. If the end of the sequence has been reached or an error
    //!  occurs, nscene is returned.
    //-----------------------------------------------------------------------------------------------
    scene_id get_next_scene(scene_id scene);

    void set_scene_view_transform(scene_id scene, const glm::mat4& view_transform);
    void get_scene_view_transform(scene_id scene, glm::mat4* view_transform);
    void set_scene_projection_transform(scene_id scene, const glm::mat4& projection_transform);
    void get_scene_projection_transform(scene_id scene, glm::mat4* projection_transform);

    void set_scene_skybox(scene_id scene, cubemap_id id);
    cubemap_id get_scene_skybox(scene_id scene);

    //-----------------------------------------------------------------------------------------------
    //! @brief Returns if a scene is enabled for rendering or not.
    //! @param scene Id of the scene to query.
    //! @return True if the scene is enabled, false otherwise.
    //-----------------------------------------------------------------------------------------------
    bool is_scene_enabled(scene_id scene);

    //-----------------------------------------------------------------------------------------------
    //! @brief Sets the enabled flag of a scene.
    //! @param scene If of the scene to set the flag to.
    //! @param enabled Value to set to the flag
    //! @remarks If a scene has its enabled flag set to false then none of its nodes should be
    //!  rendered.
    //-----------------------------------------------------------------------------------------------
    void set_scene_enabled(scene_id scene, bool enabled);

    node_id get_scene_root_node(scene_id scene);

    struct scene_handle
    {   
        scene_handle() : m_scene_id(nscene) {}
        scene_handle(scene_id s) : m_scene_id(s) {}
        scene_handle(std::nullptr_t) : m_scene_id(nscene) {}
        operator int() {return m_scene_id;}
        operator scene_id() {return m_scene_id;}
        bool operator ==(const scene_handle &other) const {return m_scene_id == other.m_scene_id;}
        bool operator !=(const scene_handle &other) const {return m_scene_id != other.m_scene_id;}
        bool operator ==(std::nullptr_t) const {return m_scene_id == nscene;}
        bool operator !=(std::nullptr_t) const {return m_scene_id != nscene;}

        scene_id m_scene_id;
    };

    struct scene_deleter
    {   
        typedef scene_handle pointer;
        scene_deleter() {}
        template<class other> scene_deleter(const other&) {}; 
        void operator()(pointer p) const { remove_scene(p); }
    };

    typedef std::unique_ptr<scene_id, scene_deleter> unique_scene;
    typedef std::vector<unique_scene> scene_vector;
    unique_scene make_scene();

    //-----------------------------------------------------------------------------------------------
    //! @brief Adds a root node to the node hierarchy.
    //! @param scene Id of the scene to add the node to.
    //! @return The id of the new node.
    //-----------------------------------------------------------------------------------------------
    node_id add_node();

    //-----------------------------------------------------------------------------------------------
    //! @brief Adds a node to the node hierarchy.
    //! @param scene Id of the scene to add the node to.
    //! @param parent If of the parent node within that scene to insert the new node under.
    //! @return The id of the new node.
    //-----------------------------------------------------------------------------------------------
    node_id add_node(node_id parent);

    //-----------------------------------------------------------------------------------------------
    //! @brief Removes a node from the node hierarchy.
    //! @param scene Id of the scene the node to remove belongs to.
    //! @param node If of the node to remove within that scene.
    //! @remarks The root node of a scene (node id = 0) cannot be removed.
    //-----------------------------------------------------------------------------------------------
    void remove_node(node_id node);

    //-----------------------------------------------------------------------------------------------
    //! @brief Updates the local and accumulated transforms of a node.
    //! @param scene Id of the scene the node belongs to.
    //! @param node Id of the node within that scene.
    //! @param local transform A matrix containing the local transform that should be assigned to
    //!  the node.
    //! @remarks The accumulated transform of the node and all its descendants are updated. The
    //!  accumulated transform of a node is the product of the local transforms of all the nodes
    //!  along the path from the root node to the node, starting from the root and including
    //!  the node.
    //-----------------------------------------------------------------------------------------------
    void set_node_transform(node_id node, const glm::mat4& local_transform);

    //-----------------------------------------------------------------------------------------------
    //! @brief Reads the local and accumulated transforms of a node.
    //! @param scene Id of the scene the node belongs to.
    //! @param node Id of the node within that scene.
    //! @param local_transform Pointer to a matrix in which to store the nodes's local transform.
    //!  Can't be nullptr.
    //! @param accum_transform Pointer to a matrix in which to store the nodes's accumulated
    //!  transform. Can't be nullptr.
    //-----------------------------------------------------------------------------------------------
    void get_node_transform(node_id node, glm::mat4* local_transform, glm::mat4* accum_transform);

    //-----------------------------------------------------------------------------------------------
    //! @brief Updates the list of meshes contained in a node.
    //! @param scene Id of the scene the node belongs to.
    //! @param node Id of the node within that scene.
    //! @param meshes An array of num_meshes elements containing the list of meshes that should
    //!  be contained in the node. Can't be nullptr.
    //! @param num_meshes The number of elements of the meshes array. Can be zero to indicate an
    //!  empty set of meshes.
    //! @remarks If some of the meshes in the provided array do not exist, they will not be added.
    //-----------------------------------------------------------------------------------------------
    void set_node_meshes(node_id node, const std::vector<mesh_id>& meshes);

    //-----------------------------------------------------------------------------------------------
    //! @brief Reads the list of meshes contained in a node.
    //! @param scene Id of the scene the node belongs to.
    //! @param node Id of the node within that scene.
    //! @param meshes Address of a pointer to store the address of an internal array containing the
    //!  list of meshes in the node. Can't be nullptr.
    //! @param num_meshes Address of an object to store the number of meshes contained in the
    //!  resource node. Can't be nullptr.
    //-----------------------------------------------------------------------------------------------
    std::vector<mesh_id> get_node_meshes(node_id node);

    void set_node_mesh(node_id node, mesh_id mesh);
    mesh_id get_node_mesh(node_id node);
    void set_node_material(node_id node, mat_id mat);
    mat_id get_node_material(node_id node);

    //-----------------------------------------------------------------------------------------------
    //! @brief Updates the enabled flag of a node, which determines if the node and its descendants
    //!  will be rendered.
    //! @param scene Id of the scene the node belongs to.
    //! @param node Id of the node within that scene.
    //! @param enabled Value to set to the flag.
    //! @remarks When a node is first created, it is enabled by default.
    //-----------------------------------------------------------------------------------------------
    void set_node_enabled(node_id node, bool enabled);

    //-----------------------------------------------------------------------------------------------
    //! @brief Reads the value of the enabled flag of a node. See set_node_enabled for the meaning
    //!  of this value.
    //! @param scene Id of the scene the node belongs to.
    //! @param node Id of the node within that scene.
    //! @return True of the node is found and is enabled, false otherwise.
    //! @remarks
    //-----------------------------------------------------------------------------------------------
    bool is_node_enabled(node_id node);

    //-----------------------------------------------------------------------------------------------
    //! @brief Reads the id of the fist child of a node.
    //! @param scene Id of the scene the node belongs to.
    //! @param node Id of the node within that scene.
    //! @return The id of the first child node, or nnode if the node is not found.
    //-----------------------------------------------------------------------------------------------
    node_id get_first_child_node(node_id node);

    //-----------------------------------------------------------------------------------------------
    //! @brief Reads the id of the next sibling of a node.
    //! @param scene Id of the scene the node belongs to.
    //! @param node Id of the node within that scene.
    //! @return The id of the next sibling node, or nnode if the node is not found.
    //-----------------------------------------------------------------------------------------------
    node_id get_next_sibling_node(node_id node);

    struct node_handle
    {   
        node_handle() : m_node_id(nnode) {}
        node_handle(node_id n) : m_node_id(n) {}
        node_handle(std::nullptr_t) : m_node_id(nnode) {}
        operator int() {return m_node_id;}
        operator node_id() {return m_node_id;}
        bool operator ==(const node_handle &other) const {return m_node_id == other.m_node_id;}
        bool operator !=(const node_handle &other) const {return m_node_id != other.m_node_id;}
        bool operator ==(std::nullptr_t) const {return m_node_id == nnode;}
        bool operator !=(std::nullptr_t) const {return m_node_id != nnode;}

        node_id m_node_id;
    };  

    struct node_deleter
    {   
        typedef node_handle pointer;
        node_deleter() {}
        template<class other> node_deleter(const other&) {}; 
        void operator()(pointer p) const { remove_node(p); }
    };

    typedef std::unique_ptr<node_id, node_deleter> unique_node;
    typedef std::vector<unique_node> node_vector;
    unique_node make_node();
    unique_node make_node(node_id parent);
    void make_node(node_id parent, resource_id resource, node_id* root_out, node_vector* nodes_out);

    void set_directional_light_ambient_color(scene_id scene, glm::vec3 ambient_color);
    void set_directional_light_diffuse_color(scene_id scene, glm::vec3 diffuse_color);
    void set_directional_light_specular_color(scene_id scene, glm::vec3 specular_color);
    void set_directional_light_direction(scene_id scene, glm::vec3 direction); // from the light to the objects
    glm::vec3 get_directional_light_ambient_color(scene_id scene);
    glm::vec3 get_directional_light_diffuse_color(scene_id scene);
    glm::vec3 get_directional_light_specular_color(scene_id scene);
    glm::vec3 get_directional_light_direction(scene_id scene);

    point_light_id add_point_light(scene_id scene);
    void remove_point_light(point_light_id light);
    point_light_id get_first_point_light(scene_id scene);
    point_light_id get_next_point_light(scene_id scene, point_light_id light);
    void set_point_light_position(scene_id scene, point_light_id light, glm::vec3 position);
    void set_point_light_ambient_color(scene_id scene, point_light_id light, glm::vec3 ambient_color);
    void set_point_light_diffuse_color(scene_id scene, point_light_id light, glm::vec3 diffuse_color);
    void set_point_light_specular_color(scene_id scene, point_light_id light, glm::vec3 specular_color);
    void set_point_light_constant_attenuation(scene_id scene, point_light_id light, float constant_attenuation);
    void set_point_light_linear_attenuation(scene_id scene, point_light_id light, float linear_attenuation);
    void set_point_light_quadratic_attenuation(scene_id scene, point_light_id light, float quadratic_attenuation);
    glm::vec3 get_point_light_position(scene_id scene, point_light_id light);
    glm::vec3 get_point_light_ambient_color(scene_id scene, point_light_id light);
    glm::vec3 get_point_light_diffuse_color(scene_id scene, point_light_id light);
    glm::vec3 get_point_light_specular_color(scene_id scene, point_light_id light);
    float get_point_light_constant_attenuation(scene_id scene, point_light_id light);
    float get_point_light_linear_attenuation(scene_id scene, point_light_id light);
    float get_point_light_quadratic_attenuation(scene_id scene, point_light_id light);

    struct point_light_handle
    {   
        point_light_handle() : m_point_light_id(npoint_light) {}
        point_light_handle(point_light_id n) : m_point_light_id(n) {}
        point_light_handle(std::nullptr_t) : m_point_light_id(npoint_light) {}
        operator int() {return m_point_light_id;}
        operator point_light_id() {return m_point_light_id;}
        bool operator ==(const point_light_handle &other) const {return m_point_light_id == other.m_point_light_id;}
        bool operator !=(const point_light_handle &other) const {return m_point_light_id != other.m_point_light_id;}
        bool operator ==(std::nullptr_t) const {return m_point_light_id == npoint_light;}
        bool operator !=(std::nullptr_t) const {return m_point_light_id != npoint_light;}

        point_light_id m_point_light_id;
    };  

    struct point_light_deleter
    {   
        typedef point_light_handle pointer;
        point_light_deleter() {}
        template<class other> point_light_deleter(const other&) {}; 
        void operator()(pointer p) const { remove_point_light(p); }
    };

    typedef std::unique_ptr<point_light_id, point_light_deleter> unique_point_light;
    typedef std::vector<unique_point_light> point_light_vector;
    unique_point_light make_point_light();

    std::vector<node_id> get_descendant_nodes(node_id node);
} // namespace cgs

#endif // SCENE_GRAPH_HPP
