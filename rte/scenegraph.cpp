#include "glm/gtc/type_ptr.hpp"
#include "scenegraph.hpp"
#include "log.hpp"
#include "glm/glm.hpp"

#include <stdexcept>
#include <algorithm>
#include <vector>
#include <queue>
#include <set>

namespace cgs
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
                m_used(true) {}

            mesh_id   m_mesh;             //!< mesh contained in this node
            mat_id    m_material;         //!< material of this node
            glm::mat4 m_local_transform;  //!< node transform relative to the parent
            glm::mat4 m_accum_transform;  //!< node transform relative to the root
            node_id   m_first_child;      //!< first child node of this node
            node_id   m_next_sibling;     //!< next sibling node of this node
            bool      m_enabled;          //!< is this node enabled? (if it is not, all descendants are ignored when rendering)
            bool      m_used;             //!< is this cell used? (used for soft deletion)
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
                m_used(true) {}

            glm::vec3 m_position;
            glm::vec3 m_ambient_color;
            glm::vec3 m_diffuse_color;
            glm::vec3 m_specular_color;
            float     m_constant_attenuation;
            float     m_linear_attenuation;
            float     m_quadratic_attenuation;
            scene_id  m_scene;
            bool      m_used;
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
        scenes[s].m_root_node = make_node();

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
            log(LOG_LEVEL_ERROR, "is_scene_enabled error: invalid scene id"); return false;
        }
        return scenes[s].m_enabled;
    }

    void set_scene_enabled(scene_id s, bool enabled)
    {
        if (!(s < scenes.size() && scenes[s].m_used)) {
            log(LOG_LEVEL_ERROR, "set_scene_enabled error: invalid scene id"); return;
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

    scene_id get_first_scene()
    {
        auto it = std::find_if(scenes.begin(), scenes.end(), [](const scene& m) { return m.m_used; });
        return (it != scenes.end() ? it - scenes.begin() : nscene);
    }

    scene_id get_next_scene(scene_id s)
    {
        auto it = std::find_if(scenes.begin() + s + 1, scenes.end(), [](const scene& s) { return s.m_used; });
        return (it != scenes.end() ? it - scenes.begin() : nscene);
    }

    void set_scene_view_transform(scene_id s, const glm::mat4& view_transform)
    {
        if (!(s < scenes.size())) {
            log(LOG_LEVEL_ERROR, "set_scene_view_transform error: invalid scene id"); return;
        }

        scenes[s].m_view_transform = view_transform;
    }

    void get_scene_view_transform(scene_id s, glm::mat4* view_transform)
    {
        if (!(view_transform)) {
            log(LOG_LEVEL_ERROR, "get_scene_view_transform error: invalid scene id"); return;
        }

        *view_transform = scenes[s].m_view_transform;
    }

    void set_scene_projection_transform(scene_id s, const glm::mat4& projection_transform)
    {
        if (!(s < scenes.size())) {
            log(LOG_LEVEL_ERROR, "set_scene_projection_transform error: invalid arguments"); return;
        }

        scenes[s].m_projection_transform = projection_transform;
    }

    void get_scene_projection_transform(scene_id s, glm::mat4* projection_transform)
    {
        if (!(projection_transform)) {
            log(LOG_LEVEL_ERROR, "get_scene_projection_transform error: invalid arguments"); return;
        }

        *projection_transform = scenes[s].m_projection_transform;
    }

    void set_scene_skybox(scene_id s, cubemap_id id)
    {
        if (!(s < scenes.size())) {
            log(LOG_LEVEL_ERROR, "set_scene_skybox error: invalid arguments"); return;
        }

        scenes[s].m_skybox = id;
    }

    cubemap_id get_scene_skybox(scene_id s)
    {
        if (!(s < scenes.size())) {
            log(LOG_LEVEL_ERROR, "get_scene_skybox error: invalid arguments"); return ncubemap;
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
            log(LOG_LEVEL_ERROR, "set_node_transform error: invalid parameters");
            throw std::logic_error("");
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
        if (!(n < nodes.size() && nodes[n].m_used && local_transform && accum_transform)) {
            log(LOG_LEVEL_ERROR, "get_node_transform error: invalid parameters"); return;
        }

        *local_transform = nodes[n].m_local_transform;
        *accum_transform = nodes[n].m_accum_transform;
    }

    void set_node_mesh(node_id n, mesh_id m)
    {
        if (n < nodes.size() && nodes[n].m_used) {
            nodes[n].m_mesh = m;
        }
    }

    mesh_id get_node_mesh(node_id n)
    {
        return ((n < nodes.size() && nodes[n].m_used)? nodes[n].m_mesh : nmesh);
    }

    void set_node_material(node_id n, mat_id mat)
    {
        if (n < nodes.size() && nodes[n].m_used) {
            nodes[n].m_material = mat;
        }
    }

    mat_id get_node_material(node_id n)
    {
        return ((n < nodes.size() && nodes[n].m_used)? nodes[n].m_material : nmat);
    }

    void set_node_enabled(node_id n, bool enabled)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            log(LOG_LEVEL_ERROR, "set_node_enabled error: invalid parameters"); return;
        }

        nodes[n].m_enabled = enabled;
    }

    bool is_node_enabled(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            log(LOG_LEVEL_ERROR, "is_node_enabled error: invalid parameters"); return false;
        }

        return nodes[n].m_enabled;
    }

    node_id get_first_child_node(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            log(LOG_LEVEL_ERROR, "get_first_child_node error: invalid parameters"); return nnode;
        }

        return nodes[n].m_first_child;
    }

    node_id get_next_sibling_node(node_id n)
    {
        if (!(n < nodes.size() && nodes[n].m_used)) {
            log(LOG_LEVEL_ERROR, "get_next_sibling_node error: invalid parameters"); return nnode;
        }

        return nodes[n].m_next_sibling;
    }

    unique_node make_node()
    {
        return unique_node(new_node());
    }

    unique_node make_node(node_id parent)
    {
        return unique_node(new_node(parent));
    }

    void make_node(node_id p, resource_id r, node_id* root_out, node_vector* nodes_out)
    {
        node_vector added_nodes;
        if (!(p < nodes.size() && nodes[p].m_used)) {
            throw std::logic_error("make_node error: invalid parameters");
        }

        node_id added_root_out = nnode;
        struct context{ resource_id rid; node_id parent; };
        std::queue<context> pending_nodes;
        pending_nodes.push({r, p});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();

            added_nodes.push_back(make_node(current.parent));
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
        if (s < scenes.size()) {
            scenes[s].m_directional_light_ambient_color = ambient_color;
        }
    }

    void set_directional_light_diffuse_color(scene_id s, glm::vec3 diffuse_color)
    {
        if (s < scenes.size()) {
            scenes[s].m_directional_light_diffuse_color = diffuse_color;
        }
    }

    void set_directional_light_specular_color(scene_id s, glm::vec3 specular_color)
    {
        if (s < scenes.size()) {
            scenes[s].m_directional_light_specular_color = specular_color;
        }
    }

    void set_directional_light_direction(scene_id s, glm::vec3 direction)
    {
        if (s < scenes.size()) {
            scenes[s].m_directional_light_direction = direction;
        }
    }

    glm::vec3 get_directional_light_ambient_color(scene_id s)
    {
        return (s < scenes.size()? scenes[s].m_directional_light_ambient_color : glm::vec3());
    }

    glm::vec3 get_directional_light_diffuse_color(scene_id s)
    {
        return (s < scenes.size()? scenes[s].m_directional_light_diffuse_color : glm::vec3());
    }

    glm::vec3 get_directional_light_specular_color(scene_id s)
    {
        return (s < scenes.size()? scenes[s].m_directional_light_specular_color : glm::vec3());
    }

    glm::vec3 get_directional_light_direction(scene_id s)
    {
        return (s < scenes.size()? scenes[s].m_directional_light_direction : glm::vec3());
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
        auto it = std::find_if(point_lights.begin() + pl + 1, point_lights.end(), [](const point_light& pl) { return pl.m_used; });
        return (it != point_lights.end() ? it - point_lights.begin() : nmat);
    }

    void set_point_light_position(point_light_id light, glm::vec3 position)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return; }
        point_lights[light].m_position = position;
    }

    void set_point_light_ambient_color(point_light_id light, glm::vec3 ambient_color)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return; }
        point_lights[light].m_ambient_color = ambient_color;
    }

    void set_point_light_diffuse_color(point_light_id light, glm::vec3 diffuse_color)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return; }
        point_lights[light].m_diffuse_color = diffuse_color;
    }

    void set_point_light_specular_color(point_light_id light, glm::vec3 specular_color)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return; }
        point_lights[light].m_specular_color = specular_color;
    }

    void set_point_light_constant_attenuation(point_light_id light, float constant_attenuation)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return; }
        point_lights[light].m_constant_attenuation = constant_attenuation;
    }

    void set_point_light_linear_attenuation(point_light_id light, float linear_attenuation)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return; }
        point_lights[light].m_linear_attenuation = linear_attenuation;
    }

    void set_point_light_quadratic_attenuation(point_light_id light, float quadratic_attenuation)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return; }
        point_lights[light].m_quadratic_attenuation = quadratic_attenuation;
    }

    glm::vec3 get_point_light_position(point_light_id light)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return glm::vec3(); }
        return point_lights[light].m_position;
    }

    glm::vec3 get_point_light_ambient_color(point_light_id light)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return glm::vec3(); }
        return point_lights[light].m_ambient_color;
    }

    glm::vec3 get_point_light_diffuse_color(point_light_id light)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return glm::vec3(); }
        return point_lights[light].m_diffuse_color;
    }

    glm::vec3 get_point_light_specular_color(point_light_id light)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return glm::vec3(); }
        return point_lights[light].m_specular_color;
    }

    float get_point_light_constant_attenuation(point_light_id light)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return 0.0f; }
        return point_lights[light].m_constant_attenuation;
    }

    float get_point_light_linear_attenuation(point_light_id light)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return 0.0f; }
        return point_lights[light].m_linear_attenuation;
    }

    float get_point_light_quadratic_attenuation(point_light_id light)
    {
        if (!(light < point_lights.size() && point_lights[light].m_used)) { return 0.0f; }
        return point_lights[light].m_quadratic_attenuation;
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
} // namespace cgs
