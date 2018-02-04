#include "serialization_utils.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "scenegraph.hpp"
#include "glm/glm.hpp"
#include "log.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>
#include <stack>
#include <queue>
#include <set>

namespace rte
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        struct node
        {
            node() :
                m_mesh(nmesh),
                m_material(nmat),
                m_local_transform(1.0f),
                m_accum_transform(1.0f),
                m_first_child(nnode),
                m_next_sibling(nnode),
                m_enabled(true),
                m_user_id(nuser_id),
                m_used(true) {}

            mesh_id     m_mesh;             //!< mesh contained in this node
            mat_id      m_material;         //!< material of this node
            glm::mat4   m_local_transform;  //!< node transform relative to the parent
            glm::mat4   m_accum_transform;  //!< node transform relative to the root
            node_id     m_first_child;      //!< first child node of this node
            node_id     m_next_sibling;     //!< next sibling node of this node
            bool        m_enabled;          //!< is this node enabled? (if it is not, all descendants are ignored when rendering)
            user_id     m_user_id;          //!< user id of this node
            std::string m_name;             //!< name of this node
            bool        m_used;             //!< is this cell used? (used for soft deletion)
        };

        struct point_light
        {
            point_light() :
                m_position(0.0f),
                m_ambient_color(0.0f),
                m_diffuse_color(0.0f),
                m_specular_color(0.0f),
                m_constant_attenuation(0.0f),
                m_linear_attenuation(0.0f),
                m_quadratic_attenuation(0.0f),
                m_scene(nscene),
                m_user_id(nuser_id),
                m_used(true) {}

            glm::vec3   m_position;
            glm::vec3   m_ambient_color;
            glm::vec3   m_diffuse_color;
            glm::vec3   m_specular_color;
            float       m_constant_attenuation;
            float       m_linear_attenuation;
            float       m_quadratic_attenuation;
            scene_id    m_scene;
            user_id     m_user_id;
            std::string m_name;
            bool        m_used;
        };

        struct scene
        {
            scene() :
                m_enabled(false),
                m_next_scene(nscene),
                m_view_transform{1.0f},
                m_projection_transform{1.0f},
                m_skybox(ncubemap),
                m_root_node(),
                m_user_id(nuser_id),
                m_name(),
                m_used(true) {}

            bool               m_enabled;                          //!< is this scene enabled?
            scene_id           m_next_scene;                       //!< id of the next scene in the view this scene belongs to
            glm::mat4          m_view_transform;                   //!< the view transform used to render all objects in the scene
            glm::mat4          m_projection_transform;             //!< the projection transform used to render all objects in the scene
            cubemap_id         m_skybox;                           //!< the id of the cubemap to use as skybox (can be ncubemap)
            glm::vec3          m_directional_light_ambient_color;  //!< ambient color of the directional light
            glm::vec3          m_directional_light_diffuse_color;  //!< diffuse color of the directional light
            glm::vec3          m_directional_light_specular_color; //!< specular color of the directional light
            glm::vec3          m_directional_light_direction;      //!< direction of the directional light
            unique_node        m_root_node;                        //!< handle to the root node of this scene
            user_id            m_user_id;                          //!< user id of this scene
            std::string        m_name;                             //!< name of this scene
            bool               m_used;                             //!< is this entry in the vector used?
        };

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        std::vector<scene> scenes;                                //!< collection of all the scenes
        std::vector<node> nodes;                                  //!< collection of all the nodes
        std::vector<point_light> point_lights;                    //!< collection of all the point lights

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        node_id allocate_node(std::vector<node>& nodes)
        {
            node_id n = std::find_if(nodes.begin(), nodes.end(), [](const node& n) { return !n.m_used; }) - nodes.begin();
            if (n == nodes.size()) {
                nodes.push_back(node{});
            } else {
                nodes[n] = node{};
            }
            return n;
        }
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void scenegraph_init()
    {
        scenes.clear();
        scenes.reserve(100);
        nodes.clear();
        nodes.reserve(1000);
    }

    scene_id new_scene()
    {
        scene_id s = std::find_if(scenes.begin(), scenes.end(), [](const scene& s) { return !s.m_used; }) - scenes.begin();
        if (s == scenes.size()) {
            scenes.push_back(scene{});
        } else {
            scenes[s] = scene{};
        }

        scenes[s].m_enabled = true;
        scenes[s].m_next_scene = nscene;
        scenes[s].m_view_transform = glm::mat4{1.0f};
        scenes[s].m_projection_transform = glm::mat4{1.0f};
        scenes[s].m_root_node = unique_node(new_node());

        return s;
    }

    void delete_scene(scene_id s)
    {
        if (s < scenes.size() && scenes[s].m_used) {
            scenes[s] = scene{}; // Implicitly removes the root node
            scenes[s].m_used = false;
        }
    }

    bool is_scene_enabled(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("is_scene_enabled error: invalid arguments");
        }

        return scenes[s].m_enabled;
    }

    void set_scene_enabled(scene_id s, bool enabled)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_scene_enabled error: invalid arguments");
        }

        scenes[s].m_enabled = enabled;
    }

    node_id get_scene_root_node(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_scene_root_node error: invalid arguments");
        }

        return scenes[s].m_root_node.get();
    }

    void set_scene_user_id(scene_id s, user_id uid)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_scene_user_id error: invalid arguments");
        }

        if (uid != nuser_id) {
            auto it = std::find_if(scenes.begin(), scenes.end(), [uid](const scene& m) { return m.m_used && m.m_user_id == uid; });
            if (it != scenes.end()) {
                throw std::logic_error("set_scene_user_id error: user id already in use");
            }
        }

        scenes[s].m_user_id = uid;
    }
    
    user_id get_scene_user_id(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_scene_user_id error: invalid arguments");
        }

        return scenes[s].m_user_id;
    }
    
    void set_scene_name(scene_id s, const std::string& name)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_scene_name error: invalid arguments");
        }

        scenes[s].m_name = name;
    }

    std::string get_scene_name(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_scene_name error: invalid arguments");
        }

        return scenes[s].m_name;
    }

    scene_id get_first_scene()
    {
        auto it = std::find_if(scenes.begin(), scenes.end(), [](const scene& m) { return m.m_used; });
        return (it != scenes.end() ? it - scenes.begin() : nscene);
    }

    scene_id get_next_scene(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_next_scene error: invalid arguments");
        }

        auto it = std::find_if(scenes.begin() + s + 1, scenes.end(), [](const scene& s) { return s.m_used; });
        return (it != scenes.end() ? it - scenes.begin() : nscene);
    }

    void set_scene_view_transform(scene_id s, const glm::mat4& view_transform)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_scene_view_transform error: invalid arguments");
        }

        scenes[s].m_view_transform = view_transform;
    }

    void get_scene_view_transform(scene_id s, glm::mat4* view_transform)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_scene_view_transform error: invalid arguments");
        }

        *view_transform = scenes[s].m_view_transform;
    }

    void set_scene_projection_transform(scene_id s, const glm::mat4& projection_transform)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_scene_projection_transform error: invalid arguments");
        }

        scenes[s].m_projection_transform = projection_transform;
    }

    void get_scene_projection_transform(scene_id s, glm::mat4* projection_transform)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_scene_projection_transform error: invalid arguments");
        }

        *projection_transform = scenes[s].m_projection_transform;
    }

    void set_scene_skybox(scene_id s, cubemap_id id)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_scene_skybox error: invalid arguments");
        }

        scenes[s].m_skybox = id;
    }

    cubemap_id get_scene_skybox(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_scene_skybox error: invalid arguments");
        }

        return scenes[s].m_skybox;
    }

    unique_scene make_scene()
    {
        return unique_scene(new_scene());
    }

    node_id new_node()
    {
        return allocate_node(nodes);
    }

    node_id new_node(node_id parent)
    {
        if (!(parent < nodes.size() && nodes[parent].m_used)) {
            log(LOG_LEVEL_ERROR, "new_node error: invalid parameters"); return nnode;
        }

        node_id child = allocate_node(nodes);
        node_id r = nodes[parent].m_first_child;
        node_id next = nnode;
        while (r != nnode && ((next = nodes[r].m_next_sibling) != nnode)) {
            r = next;
        }
        (r == nnode? nodes[parent].m_first_child : nodes[r].m_next_sibling) = child;

        return child;
    }

    void delete_node(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            log(LOG_LEVEL_ERROR, "delete_node error: invalid parameters"); return;
        }

        auto nit = nodes.begin();
        while (nit != nodes.end()) {
            if (nit->m_used && nit->m_first_child == n) {
                nit->m_first_child = nodes[n].m_next_sibling;
                break;
            } else if (nit->m_used && nit->m_next_sibling == n) {
                nit->m_next_sibling = nodes[n].m_next_sibling;
                break;
            }
            nit++;
        }

        // Mark the vector entry as not used
        nodes[n].m_used = false; // soft removal
    }

    void update_accum_transforms(node_id root_node, node_id n, std::set<node_id>* visited_nodes)
    {
        if (!(n < nodes.size() && nodes[n].m_used && root_node < nodes.size() && nodes[root_node].m_used)) {
            throw std::logic_error("update_accum_transforms error: invalid arguments");
        }

        struct context{ node_id nid; bool intree; glm::mat4 prefix; };
        std::queue<context> pending_nodes;
        pending_nodes.push({root_node, n == root_node, glm::mat4{1.0f}});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();
            if (visited_nodes->count(current.nid) == 0) {
                visited_nodes->insert(current.nid);
                glm::mat4 accum_transform = current.prefix * nodes[current.nid].m_local_transform;
                if (current.intree || current.nid == n) {
                    nodes[current.nid].m_accum_transform = accum_transform;
                }
                for (node_id child = nodes[current.nid].m_first_child; child != nnode; child = nodes[child].m_next_sibling) {
                    pending_nodes.push({child, current.intree || current.nid == n, accum_transform});
                }
            }
        }
    }

    void set_node_transform(node_id n, const glm::mat4& local_transform)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("set_node_transform error: invalid arguments");
        }

        nodes[n].m_local_transform = local_transform;
        // Update the accummulated transforms of n and all its descendant nodes with a breadth-first search
        std::set<node_id> visited_nodes;
        for (unsigned int i = 0; i < nodes.size(); i++) {
            if (visited_nodes.count(n)) {
                break;
            }

            if (nodes[i].m_used && visited_nodes.count(i) == 0) {
                update_accum_transforms(i, n, &visited_nodes);
            }
        }
    }

    void get_node_transform(node_id n, glm::mat4* local_transform, glm::mat4* accum_transform)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("get_node_transform error: invalid arguments");
        }

        *local_transform = nodes[n].m_local_transform;
        *accum_transform = nodes[n].m_accum_transform;
    }

    void set_node_mesh(node_id n, mesh_id m)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("set_node_mesh error: invalid arguments");
        }

        if (n < nodes.size() && nodes[n].m_used) {
            nodes[n].m_mesh = m;
        }
    }

    mesh_id get_node_mesh(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("get_node_mesh error: invalid arguments");
        }

        // TODO remove conditional
        return ((n < nodes.size() && nodes[n].m_used)? nodes[n].m_mesh : nmesh);
    }

    void set_node_material(node_id n, mat_id mat)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("set_node_material error: invalid arguments");
        }

        // TODO remove conditional
        if (n < nodes.size() && nodes[n].m_used) {
            nodes[n].m_material = mat;
        }
    }

    mat_id get_node_material(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("get_node_material error: invalid arguments");
        }

        // TODO remove conditional
        return ((n < nodes.size() && nodes[n].m_used)? nodes[n].m_material : nmat);
    }

    void set_node_name(node_id n, const std::string& name)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("set_node_name error: invalid arguments");
        }

        nodes[n].m_name = name;
    }

    std::string get_node_name(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("get_node_name error: invalid arguments");
        }

        return nodes[n].m_name;
    }

    void set_node_enabled(node_id n, bool enabled)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("set_node_enabled error: invalid arguments");
        }

        nodes[n].m_enabled = enabled;
    }

    bool is_node_enabled(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("is_node_enabled error: invalid arguments");
        }

        return nodes[n].m_enabled;
    }

    node_id get_first_child_node(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("get_first_child_node error: invalid arguments");
        }

        return nodes[n].m_first_child;
    }

    node_id get_next_sibling_node(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("get_next_sibling_node error: invalid arguments");
        }

        return nodes[n].m_next_sibling;
    }

    void set_node_user_id(node_id n, user_id uid)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("set_node_user_id error: invalid arguments");
        }

        if (uid != nuser_id) {
            auto it = std::find_if(nodes.begin(), nodes.end(), [uid](const node& m) { return m.m_used && m.m_user_id == uid; });
            if (it != nodes.end()) {
                throw std::logic_error("set_node_user_id error: user id already in use");
            }
        }

        nodes[n].m_user_id = uid;
    }

    user_id get_node_user_id(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            throw std::logic_error("get_node_user_id error: invalid arguments");
        }

        return nodes[n].m_user_id;
    }

    void make_node(node_id* root_out, node_vector* nodes_out)
    {
        unique_node new_node_handle(new_node());
        *root_out = new_node_handle.get();
        nodes_out->push_back(std::move(new_node_handle));
    }

    void make_node(node_id p, resource_id r, node_id* root_out, node_vector* nodes_out)
    {
        if (!(p < nodes.size() && nodes[p].m_used)) {
            throw std::logic_error("make_node error: invalid parameters");
        }

        if (r == nresource) {
            unique_node new_node_handle(new_node(p));
            *root_out = new_node_handle.get();
            nodes_out->push_back(std::move(new_node_handle));

            return;
        }

        node_vector added_nodes;
        node_id added_root_out = nnode;
        struct context{ resource_id rid; node_id parent; };
        std::queue<context> pending_nodes;
        pending_nodes.push({r, p});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();

            added_nodes.push_back(unique_node(new_node(current.parent)));
            node_id n = added_nodes[added_nodes.size() - 1].get();
            added_root_out = (added_root_out == nnode ? n : added_root_out);
            set_node_transform(n, get_resource_local_transform(current.rid));
            set_node_mesh(n, get_resource_mesh(current.rid));
            set_node_material(n, get_resource_material(current.rid));
            set_node_enabled(n, true);

            for (resource_id child = get_first_child_resource(current.rid); child != nresource; child = get_next_sibling_resource(child)) {
                pending_nodes.push({child, n});
            }
        }

        nodes_out->insert(nodes_out->end(), make_move_iterator(added_nodes.begin()), make_move_iterator(added_nodes.end()));
        *root_out = added_root_out;
    }

    void set_directional_light_ambient_color(scene_id s, glm::vec3 ambient_color)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_directional_light_ambient_color error: invalid arguments");
        }

        if (s < scenes.size()) {
            scenes[s].m_directional_light_ambient_color = ambient_color;
        }
    }

    void set_directional_light_diffuse_color(scene_id s, glm::vec3 diffuse_color)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_directional_light_diffuse_color error: invalid arguments");
        }

        scenes[s].m_directional_light_diffuse_color = diffuse_color;
    }

    void set_directional_light_specular_color(scene_id s, glm::vec3 specular_color)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_directional_light_specular_color error: invalid arguments");
        }

        scenes[s].m_directional_light_specular_color = specular_color;
    }

    void set_directional_light_direction(scene_id s, glm::vec3 direction)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("set_directional_light_direction error: invalid arguments");
        }

        scenes[s].m_directional_light_direction = direction;
    }

    glm::vec3 get_directional_light_ambient_color(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_directional_light_ambient_color error: invalid arguments");
        }

        return scenes[s].m_directional_light_ambient_color;
    }

    glm::vec3 get_directional_light_diffuse_color(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_directional_light_diffuse_color error: invalid arguments");
        }

        return (s < scenes.size()? scenes[s].m_directional_light_diffuse_color : glm::vec3());
    }

    glm::vec3 get_directional_light_specular_color(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_directional_light_specular_color error: invalid arguments");
        }

        return scenes[s].m_directional_light_specular_color;
    }

    glm::vec3 get_directional_light_direction(scene_id s)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            throw std::logic_error("get_directional_light_direction error: invalid arguments");
        }

        return scenes[s].m_directional_light_direction;
    }

    point_light_id new_point_light(scene_id s)
    {
        point_light_id pl = std::find_if(point_lights.begin(), point_lights.end(), [](const point_light& pl) { return !pl.m_used; }) - point_lights.begin();
        if (pl == point_lights.size()) {
            point_lights.push_back(point_light{});
        } else {
            point_lights[pl] = point_light{};
        }
        point_lights[pl].m_scene = s;

        return pl;
    }

    void delete_point_light(point_light_id light)
    {
        if (light < point_lights.size() && point_lights[light].m_used) {
            point_lights[light] = point_light{};
            point_lights[light].m_used = false;
        }
    }

    point_light_id get_first_point_light()
    {
        auto it = std::find_if(point_lights.begin(), point_lights.end(), [](const point_light& pl) { return pl.m_used; });
        return (it != point_lights.end() ? it - point_lights.begin() : npoint_light);
    }

    point_light_id get_next_point_light(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_next_point_light error: invalid arguments");
        }

        auto it = std::find_if(point_lights.begin() + pl + 1, point_lights.end(), [](const point_light& pl) { return pl.m_used; });
        return (it != point_lights.end() ? it - point_lights.begin() : nmat);
    }

    void set_point_light_position(point_light_id pl, glm::vec3 position)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_position error: invalid arguments");
        }

        point_lights[pl].m_position = position;
    }

    void set_point_light_ambient_color(point_light_id pl, glm::vec3 ambient_color)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_ambient_color error: invalid arguments");
        }

        point_lights[pl].m_ambient_color = ambient_color;
    }

    void set_point_light_diffuse_color(point_light_id pl, glm::vec3 diffuse_color)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_diffuse_color error: invalid arguments");
        }

        point_lights[pl].m_diffuse_color = diffuse_color;
    }

    void set_point_light_specular_color(point_light_id pl, glm::vec3 specular_color)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_specular_color error: invalid arguments");
        }

        point_lights[pl].m_specular_color = specular_color;
    }

    void set_point_light_constant_attenuation(point_light_id pl, float constant_attenuation)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_constant_attenuation error: invalid arguments");
        }

        point_lights[pl].m_constant_attenuation = constant_attenuation;
    }

    void set_point_light_linear_attenuation(point_light_id pl, float linear_attenuation)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_linear_attenuation error: invalid arguments");
        }

        point_lights[pl].m_linear_attenuation = linear_attenuation;
    }

    void set_point_light_quadratic_attenuation(point_light_id pl, float quadratic_attenuation)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_quadratic_attenuation error: invalid arguments");
        }

        point_lights[pl].m_quadratic_attenuation = quadratic_attenuation;
    }

    void set_point_light_user_id(point_light_id pl, user_id uid)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_user_id error: invalid arguments");
        }

        if (uid != nuser_id) {
            auto it = std::find_if(point_lights.begin(), point_lights.end(), [uid](const point_light& m) { return m.m_used && m.m_user_id == uid; });
            if (it != point_lights.end()) {
                throw std::logic_error("set_point_light_user_id error: user id already in use");
            }
        }

        point_lights[pl].m_user_id = uid;
    }

    void set_point_light_name(point_light_id pl, const std::string& name)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("set_point_light_name error: invalid arguments");
        }

        point_lights[pl].m_name = name;
    }

    glm::vec3 get_point_light_position(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_position error: invalid arguments");
        }

        return point_lights[pl].m_position;
    }

    glm::vec3 get_point_light_ambient_color(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_ambient_color error: invalid arguments");
        }

        return point_lights[pl].m_ambient_color;
    }

    glm::vec3 get_point_light_diffuse_color(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_diffuse_color error: invalid arguments");
        }

        return point_lights[pl].m_diffuse_color;
    }

    glm::vec3 get_point_light_specular_color(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_specular_color error: invalid arguments");
        }

        return point_lights[pl].m_specular_color;
    }

    float get_point_light_constant_attenuation(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_constant_attenuation error: invalid arguments");
        }

        return point_lights[pl].m_constant_attenuation;
    }

    float get_point_light_linear_attenuation(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_linear_attenuation error: invalid arguments");
        }

        return point_lights[pl].m_linear_attenuation;
    }

    float get_point_light_quadratic_attenuation(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_quadratic_attenuation error: invalid arguments");
        }

        return point_lights[pl].m_quadratic_attenuation;
    }

    user_id get_point_light_user_id(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_user_id error: invalid arguments");
        }

        return point_lights[pl].m_user_id;
    }

    std::string get_point_light_name(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_name error: invalid arguments");
        }

        return point_lights[pl].m_name;
    }

    scene_id get_point_light_scene(point_light_id pl)
    {
        if (!(pl < point_lights.size() && point_lights[pl].m_used)) {
            throw std::logic_error("get_point_light_scene error: invalid arguments");
        }

        return point_lights[pl].m_scene;
    }

    unique_point_light make_point_light(scene_id s)
    {
        return unique_point_light(new_point_light(s));
    }

    std::vector<node_id> get_descendant_nodes(node_id n)
    {
        std::vector<node_id> ret;
        struct node_context{ node_id nid; };
        std::queue<node_context> pending_nodes;
        pending_nodes.push({n});

        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();
            // If a node is not enabled, all its subtree is pruned
            if (!is_node_enabled(current.nid)) continue;

            // If a node doesn't have any meshes or materials it is ignored, but its children are processed
            if (get_node_mesh(current.nid) != nmesh && get_node_material(current.nid) != nmat) {
              ret.push_back(current.nid);
            }

            for (node_id c = get_first_child_node(current.nid); c != nnode; c = get_next_sibling_node(c)) {
                pending_nodes.push({c});
            }
        }

        return ret;
    }

    void log_node(node_id root)
    {
        // Iterate the node tree with a depth-first search printing nodes
        struct context{ node_id nid; unsigned int indentation; };
        std::stack<context, std::vector<context>> pending_nodes;
        pending_nodes.push({root, 3U});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.top();
            pending_nodes.pop();

            std::ostringstream oss;
            oss << std::setprecision(2) << std::fixed;
            for (unsigned int i = 0; i < current.indentation; i++) {
                oss << "    ";
            }

            glm::mat4 local_transform;
            glm::mat4 accum_transform;
            get_node_transform(current.nid, &local_transform, &accum_transform);
            oss << "[ ";
            oss << "node id: " << current.nid;
            oss << ", user id: " << format_user_id(get_node_user_id(current.nid));
            oss << ", name: " << get_node_name(current.nid);
            oss << ", mesh: " << format_mesh_id(get_node_mesh(current.nid));
            oss << ", material: " << format_material_id(get_node_material(current.nid));
            oss << ", local transform: " << local_transform;
            oss << " ]";
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            // We are using a stack to process depth-first, so in order for the children to be
            // processed in the order in which they appear we must push them in reverse order,
            // otherwise the last child will be processed first
            std::vector<node_id> children_list;
            for (node_id child = get_first_child_node(current.nid); child != nnode; child = get_next_sibling_node(child)) {
                children_list.push_back(child);
            }

            for (auto cit = children_list.rbegin(); cit != children_list.rend(); cit++) {
                pending_nodes.push({*cit, current.indentation + 1});
            }
        }
    }

    void log_directional_light(scene_id s)
    {
        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        oss << "        directional light: ";
        oss << "[ ambient color : " << get_directional_light_ambient_color(s);
        oss << ", diffuse color : " << get_directional_light_diffuse_color(s);
        oss << ", specular color : " << get_directional_light_specular_color(s);
        oss << ", direction : " << get_directional_light_direction(s);
        oss << " ]";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }

    void log_point_light(point_light_id pl)
    {
        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        oss << "            [ point light id: " << pl;
        oss << ", user_id : " << get_point_light_user_id(pl);
        oss << ", position : " << get_point_light_position(pl);
        oss << ", ambient color : " << get_point_light_ambient_color(pl);
        oss << ", diffuse color : " << get_point_light_diffuse_color(pl);
        oss << ", specular color : " << get_point_light_specular_color(pl);
        oss << ", constant_attenuation : " << get_point_light_constant_attenuation(pl);
        oss << ", linear_attenuation : " << get_point_light_linear_attenuation(pl);
        oss << ", quadratic_attenuation : " << get_point_light_quadratic_attenuation(pl);
        oss << " ]";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }

    void log_scene(scene_id s)
    {
        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        oss << "    scene id: " << s;
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        oss.str("");
        oss << "        user_id : " << get_scene_user_id(s);
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        log_directional_light(s);

        oss.str("");
        oss << "        point lights :";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        // FIXME this model forces us to iterate over ALL point_lights, even the ones that don't belong to the scene at hand
        for (point_light_id pl = get_first_point_light(); pl != npoint_light; pl = get_next_point_light(pl)) {
            log_point_light(pl);
        }

        oss.str("");
        node_id root = get_scene_root_node(s);
        oss << "        root node :";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        if (root != nnode) {
            log_node(root);
        } else {
            log(LOG_LEVEL_DEBUG, "    no root node found");
        }
    }

    void log_scenes()
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "scenegraph: scenes begin");
        if (scenes.size()) {
            for (unsigned int s = 0; s < scenes.size() && scenes[s].m_used; s++) {
                log_scene(s);
            }
        } else {
            log(LOG_LEVEL_DEBUG, "    no scenes found");
        }

        log(LOG_LEVEL_DEBUG, "scenegraph: scenes end");
    }
} // namespace rte
