#include "cgs/scenegraph.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cgs/log.hpp"
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
        struct view
        {
            view() : menabled(true), mfirst_scene(nscene) {}
            bool menabled;           //<! is this view enabled?
            scene_id mfirst_scene;   //<! id of the first scene of this view
        };

        typedef std::vector<view> view_vector;

        struct node
        {
            node() :
                mmesh(nmesh),
                mmaterial(nmat),
                mlocal_transform(1.0f),
                maccum_transform(1.0f),
                mfirst_child(nnode),
                mnext_sibling(nnode),
                menabled(true),
                mused(true) {}

            mesh_id   mmesh;             //!< mesh contained in this node
            mat_id    mmaterial;         //!< material of this node
            glm::mat4 mlocal_transform;  //!< node transform relative to the parent
            glm::mat4 maccum_transform;  //!< node transform relative to the root
            node_id   mfirst_child;      //!< first child node of this node
            node_id   mnext_sibling;     //!< next sibling node of this node
            bool      menabled;          //!< is this node enabled? (if it is not, all descendants are ignored when rendering)
            bool      mused;             //!< is this cell used? (used for soft deletion)
        };

        typedef std::vector<node> node_vector;
        typedef node_vector::iterator node_iterator;

        struct point_light
        {
            point_light() :
                mposition(0.0f),
                mambient_color(0.0f),
                mdiffuse_color(0.0f),
                mspecular_color(0.0f),
                mconstant_attenuation(0.0f),
                mlinear_attenuation(0.0f),
                mquadratic_attenuation(0.0f) {}

            glm::vec3 mposition;
            glm::vec3 mambient_color;
            glm::vec3 mdiffuse_color;
            glm::vec3 mspecular_color;
            float     mconstant_attenuation;
            float     mlinear_attenuation;
            float     mquadratic_attenuation;
        };

        typedef std::vector<point_light> point_light_vector;

        struct scene
        {
            scene() :
                mview(0),
                menabled(false),
                mnext_scene(nscene),
                mview_transform{1.0f},
                mprojection_transform{1.0f},
                mskybox(ncubemap),
                mpoint_lights(),
                mroot_node(nnode) {}

            view_id            mview;                             //!< id of the view this scene belongs to
            bool               menabled;                          //!< is this scene enabled?
            scene_id           mnext_scene;                       //!< id of the next scene in the view this scene belongs to
            node_vector        mnodes;                            //!< collection of all the nodes in this scene
            glm::mat4          mview_transform;                   //!< the view transform used to render all objects in the scene
            glm::mat4          mprojection_transform;             //!< the projection transform used to render all objects in the scene
            cubemap_id         mskybox;                           //!< the id of the cubemap to use as skybox (can be ncubemap)
            glm::vec3          mdirectional_light_ambient_color;  //!< ambient color of the directional light
            glm::vec3          mdirectional_light_diffuse_color;  //!< diffuse color of the directional light
            glm::vec3          mdirectional_light_specular_color; //!< specular color of the directional light
            glm::vec3          mdirectional_light_direction;      //!< direction of the directional light
            point_light_vector mpoint_lights;                     //!< collection of all the point lights in this scene
            node_id            mroot_node;                        //!< id of the root node of this scene
        };

        typedef std::vector<scene> scene_vector;

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        view_vector views;          //!< collection of all the views
        scene_vector scenes;        //!< collection of all the scenes

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        scene_id get_last_scene(view_id v)
        {
            scene_id l = views[v].mfirst_scene;
            scene_id next = nscene;
            while (l != nscene && ((next = scenes[l].mnext_scene) != nscene)) {
                l = next;
            }
            return l;
        }

        node_id allocate_node(node_vector& nodes)
        {
            node_id n = std::find_if(nodes.begin(), nodes.end(), [](const node& n) { return !n.mused; }) - nodes.begin();
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
        views.clear();
        views.reserve(10);
        scenes.clear();
        views.reserve(100);
    }

    view_id add_view()
    {
        views.push_back(view{});
        return views.size() - 1;
    }

    bool is_view_enabled(view_id vid)
    {
        return (vid < views.size()? views[vid].menabled : false);
    }

    void set_view_enabled(view_id vid, bool enabled)
    {
        if (vid < views.size()) {
            views[vid].menabled = enabled;
        }
    }

    scene_id add_scene(view_id v)
    {
        if (!(v < views.size())) {
            log(LOG_LEVEL_ERROR, "add_scene error: invalid view id");
            return nscene;
        }

        scenes.push_back(scene{});
        scene& lr = scenes[scenes.size() - 1];
        lr.mview = v;
        lr.menabled = true;
        lr.mnext_scene = nscene;
        lr.mnodes.clear();
        lr.mnodes.reserve(2048);
        lr.mview_transform = glm::mat4{1.0f};
        lr.mprojection_transform = glm::mat4{1.0f};

        scene_id lid = scenes.size() - 1;
        scene_id last_scene = get_last_scene(v);
        (last_scene == nscene? views[v].mfirst_scene : scenes[last_scene].mnext_scene) = lid;
        lr.mroot_node = add_node(lid);
        return lid;
    }

    bool is_scene_enabled(scene_id l)
    {
        if (!(l < scenes.size())) {
            log(LOG_LEVEL_ERROR, "is_scene_enabled error: invalid scene id"); return false;
        }
        return scenes[l].menabled;
    }

    void set_scene_enabled(scene_id l, bool enabled)
    {
        if (!(l < scenes.size())) {
            log(LOG_LEVEL_ERROR, "set_scene_enabled error: invalid scene id"); return;
        }
        scenes[l].menabled = enabled;
    }

    node_id get_scene_root_node(scene_id l)
    {
        if (!(l < scenes.size())) {
            throw std::logic_error("get_scene_root_node error: invalid arguments");
        }

        return scenes[l].mroot_node;
    }

    scene_id get_first_scene(view_id v)
    {
        if (!(v < views.size())) {
            log(LOG_LEVEL_ERROR, "get_first_scene error: invalid view id");
            return nscene;
        }
        return views[v].mfirst_scene;
    }

    scene_id get_next_scene(scene_id l)
    {
        if (!(l < scenes.size())) {
            log(LOG_LEVEL_ERROR, "get_next_scene error: invalid scene id"); return nscene;
        }
        return scenes[l].mnext_scene;
    }

    void set_scene_view_transform(scene_id l, const glm::mat4& view_transform)
    {
        if (!(l < scenes.size())) {
            log(LOG_LEVEL_ERROR, "set_scene_view_transform error: invalid scene id"); return;
        }

        scenes[l].mview_transform = view_transform;
    }

    void get_scene_view_transform(scene_id l, glm::mat4* view_transform)
    {
        if (!(l < scenes.size() && view_transform)) {
            log(LOG_LEVEL_ERROR, "get_scene_view_transform error: invalid scene id"); return;
        }

        *view_transform = scenes[l].mview_transform;
    }

    void set_scene_projection_transform(scene_id l, const glm::mat4& projection_transform)
    {
        if (!(l < scenes.size())) {
            log(LOG_LEVEL_ERROR, "set_scene_projection_transform error: invalid arguments"); return;
        }

        scenes[l].mprojection_transform = projection_transform;
    }

    void get_scene_projection_transform(scene_id l, glm::mat4* projection_transform)
    {
        if (!(l < scenes.size() && projection_transform)) {
            log(LOG_LEVEL_ERROR, "get_scene_projection_transform error: invalid arguments"); return;
        }

        *projection_transform = scenes[l].mprojection_transform;
    }

    void set_scene_skybox(scene_id l, cubemap_id id)
    {
        if (!(l < scenes.size())) {
            log(LOG_LEVEL_ERROR, "set_scene_skybox error: invalid arguments"); return;
        }

        scenes[l].mskybox = id;
    }

    cubemap_id get_scene_skybox(scene_id l)
    {
        if (!(l < scenes.size())) {
            log(LOG_LEVEL_ERROR, "get_scene_skybox error: invalid arguments"); return ncubemap;
        }

        return scenes[l].mskybox;
    }

    node_id add_node(scene_id l)
    {
        if (!(l < scenes.size())) {
            log(LOG_LEVEL_ERROR, "add_node error: invalid parameters"); return nnode;
        }

        return allocate_node(scenes[l].mnodes);
    }

    node_id add_node(scene_id l, node_id parent)
    {
        if (!(l < scenes.size() && parent < scenes[l].mnodes.size() && scenes[l].mnodes[parent].mused)) {
            log(LOG_LEVEL_ERROR, "add_node error: invalid parameters"); return nnode;
        }

        node_id child = allocate_node(scenes[l].mnodes);
        node_id r = scenes[l].mnodes[parent].mfirst_child;
        node_id next = nnode;
        while (r != nnode && ((next = scenes[l].mnodes[r].mnext_sibling) != nnode)) {
            r = next;
        }
        (r == nnode? scenes[l].mnodes[parent].mfirst_child : scenes[l].mnodes[r].mnext_sibling) = child;

        return child;
    }

    node_id add_node(scene_id l, node_id p, resource_id root)
    {
        if (!(l < scenes.size() && p < scenes[l].mnodes.size() && scenes[l].mnodes[p].mused)) {
            log(LOG_LEVEL_ERROR, "add_node error: invalid parameters"); return nnode;
        }

        node_id ret = nnode;

        struct context{ resource_id rid; node_id parent; };
        std::queue<context> pending_nodes;
        pending_nodes.push({root, p});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();

            node_id n = add_node(l, current.parent);
            set_node_transform(l, n, get_resource_local_transform(current.rid));
            set_node_mesh(l, n, get_resource_mesh(current.rid));
            set_node_material(l, n, get_resource_material(current.rid));
            set_node_enabled(l, n, true);
            if (ret == nnode) {
                ret = n;
            }

            for (resource_id child = get_first_child_resource(current.rid); child != nresource; child = get_next_sibling_resource(child)) {
                pending_nodes.push({child, n});
            }
        }

        return ret;
    }

    void remove_node(scene_id l, node_id n)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "remove_node error: invalid parameters"); return;
        }

        auto nit = scenes[l].mnodes.begin();
        while (nit != scenes[l].mnodes.end()) {
            if (nit->mused && nit->mfirst_child == n) {
                nit->mfirst_child = scenes[l].mnodes[n].mnext_sibling;
                break;
            } else if (nit->mused && nit->mnext_sibling == n) {
                nit->mnext_sibling = scenes[l].mnodes[n].mnext_sibling;
                break;
            }
            nit++;
        }

        // Mark all its descendants as not used
        std::queue<node_id> pending_nodes;
        pending_nodes.push(n);
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();
            scenes[l].mnodes[current] = node(); // reset all fields back to default state
            for (auto child = get_first_child_node(l, current); child != nnode; child = get_next_sibling_node(l, child)) {
                pending_nodes.push(child);
            }
            scenes[l].mnodes[current].mused = false; // soft removal
        }

        // Mark the vector entry as not used
        scenes[l].mnodes[n].mused = false; // soft removal
    }

    void update_accum_transforms(scene_id l, node_id root_node, node_id n, std::set<node_id>* visited_nodes)
    {
        struct context{ node_id nid; bool intree; glm::mat4 prefix; };
        std::queue<context> pending_nodes;
        pending_nodes.push({root_node, n == root_node, glm::mat4{1.0f}});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();
            if (visited_nodes->count(current.nid) == 0) {
                visited_nodes->insert(current.nid);
                glm::mat4 accum_transform = current.prefix * scenes[l].mnodes[current.nid].mlocal_transform;
                if (current.intree || current.nid == n) {
                    scenes[l].mnodes[current.nid].maccum_transform = accum_transform;
                }
                for (node_id child = scenes[l].mnodes[current.nid].mfirst_child; child != nnode; child = scenes[l].mnodes[child].mnext_sibling) {
                    pending_nodes.push({child, current.intree || current.nid == n, accum_transform});
                }
            }
        }
    }

    void set_node_transform(scene_id l, node_id n, const glm::mat4& local_transform)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "set_node_transform error: invalid parameters");
            throw std::logic_error("");
        }

        scenes[l].mnodes[n].mlocal_transform = local_transform;
        // Update the accummulated transforms of n and all its descendant nodes with a breadth-first search
        std::set<node_id> visited_nodes;
        for (unsigned int i = 0; i < scenes[l].mnodes.size(); i++) {
            if (visited_nodes.count(n)) {
                break;
            }

            if (scenes[l].mnodes[i].mused && visited_nodes.count(i) == 0) {
                update_accum_transforms(l, i, n, &visited_nodes);
            }
        }
    }

    void get_node_transform(scene_id l, node_id n, glm::mat4* local_transform, glm::mat4* accum_transform)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused && local_transform && accum_transform)) {
            log(LOG_LEVEL_ERROR, "get_node_transform error: invalid parameters"); return;
        }

        *local_transform = scenes[l].mnodes[n].mlocal_transform;
        *accum_transform = scenes[l].mnodes[n].maccum_transform;
    }

    void set_node_meshes(scene_id l, node_id n, const std::vector<mesh_id>&)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "set_node_meshes error: invalid parameters"); return;
        }

        //scenes[l].mnodes[n].mmeshes = m;
    }

    std::vector<mesh_id> get_node_meshes(scene_id l, node_id n)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "get_node_meshes error: invalid parameters"); return std::vector<mesh_id>();
        }

        return std::vector<mesh_id>();
    }

    void set_node_mesh(scene_id l, node_id n, mesh_id m)
    {
        if (l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused) {
            scenes[l].mnodes[n].mmesh = m;
        }
    }

    mesh_id get_node_mesh(scene_id l, node_id n)
    {
        return ((l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)? scenes[l].mnodes[n].mmesh : nmesh);
    }

    void set_node_material(scene_id l, node_id n, mat_id mat)
    {
        if (l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused) {
            scenes[l].mnodes[n].mmaterial = mat;
        }
    }

    mat_id get_node_material(scene_id l, node_id n)
    {
        return ((l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)? scenes[l].mnodes[n].mmaterial : nmat);
    }

    void set_node_enabled(scene_id l, node_id n, bool enabled)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "set_node_enabled error: invalid parameters"); return;
        }

        scenes[l].mnodes[n].menabled = enabled;
    }

    bool is_node_enabled(scene_id l, node_id n)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "is_node_enabled error: invalid parameters"); return false;
        }

        return scenes[l].mnodes[n].menabled;
    }

    node_id get_first_child_node(scene_id l, node_id n)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "get_first_child_node error: invalid parameters"); return nnode;
        }

        return scenes[l].mnodes[n].mfirst_child;
    }

    node_id get_next_sibling_node(scene_id l, node_id n)
    {
        if (!(l < scenes.size() && n < scenes[l].mnodes.size() && scenes[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "get_next_sibling_node error: invalid parameters"); return nnode;
        }

        return scenes[l].mnodes[n].mnext_sibling;
    }

    void set_directional_light_ambient_color(scene_id l, glm::vec3 ambient_color)
    {
        if (l < scenes.size()) {
            scenes[l].mdirectional_light_ambient_color = ambient_color;
        }
    }

    void set_directional_light_diffuse_color(scene_id l, glm::vec3 diffuse_color)
    {
        if (l < scenes.size()) {
            scenes[l].mdirectional_light_diffuse_color = diffuse_color;
        }
    }

    void set_directional_light_specular_color(scene_id l, glm::vec3 specular_color)
    {
        if (l < scenes.size()) {
            scenes[l].mdirectional_light_specular_color = specular_color;
        }
    }

    void set_directional_light_direction(scene_id l, glm::vec3 direction)
    {
        if (l < scenes.size()) {
            scenes[l].mdirectional_light_direction = direction;
        }
    }

    glm::vec3 get_directional_light_ambient_color(scene_id l)
    {
        return (l < scenes.size()? scenes[l].mdirectional_light_ambient_color : glm::vec3());
    }

    glm::vec3 get_directional_light_diffuse_color(scene_id l)
    {
        return (l < scenes.size()? scenes[l].mdirectional_light_diffuse_color : glm::vec3());
    }

    glm::vec3 get_directional_light_specular_color(scene_id l)
    {
        return (l < scenes.size()? scenes[l].mdirectional_light_specular_color : glm::vec3());
    }

    glm::vec3 get_directional_light_direction(scene_id l)
    {
        return (l < scenes.size()? scenes[l].mdirectional_light_direction : glm::vec3());
    }

    point_light_id add_point_light(scene_id scene)
    {
        if (!(scene < scenes.size())) { return npoint_light; }
        scenes[scene].mpoint_lights.push_back(point_light{});
        return scenes[scene].mpoint_lights.size() - 1;
    }

    point_light_id get_first_point_light(scene_id scene)
    {
        if (!(scene < scenes.size())) { return npoint_light; }
        return (scenes[scene].mpoint_lights.size()? 0U : npoint_light);
    }

    point_light_id get_next_point_light(scene_id scene, point_light_id light)
    {
        if (!(scene < scenes.size())) { return npoint_light; }
        return (light + 1 < scenes[scene].mpoint_lights.size()? light + 1 : npoint_light);
    }

    void set_point_light_position(scene_id scene, point_light_id light, glm::vec3 position)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return; }
        scenes[scene].mpoint_lights[light].mposition = position;
    }

    void set_point_light_ambient_color(scene_id scene, point_light_id light, glm::vec3 ambient_color)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return; }
        scenes[scene].mpoint_lights[light].mambient_color = ambient_color;
    }

    void set_point_light_diffuse_color(scene_id scene, point_light_id light, glm::vec3 diffuse_color)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return; }
        scenes[scene].mpoint_lights[light].mdiffuse_color = diffuse_color;
    }

    void set_point_light_specular_color(scene_id scene, point_light_id light, glm::vec3 specular_color)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return; }
        scenes[scene].mpoint_lights[light].mspecular_color = specular_color;
    }

    void set_point_light_constant_attenuation(scene_id scene, point_light_id light, float constant_attenuation)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return; }
        scenes[scene].mpoint_lights[light].mconstant_attenuation = constant_attenuation;
    }

    void set_point_light_linear_attenuation(scene_id scene, point_light_id light, float linear_attenuation)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return; }
        scenes[scene].mpoint_lights[light].mlinear_attenuation = linear_attenuation;
    }

    void set_point_light_quadratic_attenuation(scene_id scene, point_light_id light, float quadratic_attenuation)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return; }
        scenes[scene].mpoint_lights[light].mquadratic_attenuation = quadratic_attenuation;
    }

    glm::vec3 get_point_light_position(scene_id scene, point_light_id light)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return glm::vec3(); }
        return scenes[scene].mpoint_lights[light].mposition;
    }

    glm::vec3 get_point_light_ambient_color(scene_id scene, point_light_id light)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return glm::vec3(); }
        return scenes[scene].mpoint_lights[light].mambient_color;
    }

    glm::vec3 get_point_light_diffuse_color(scene_id scene, point_light_id light)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return glm::vec3(); }
        return scenes[scene].mpoint_lights[light].mdiffuse_color;
    }

    glm::vec3 get_point_light_specular_color(scene_id scene, point_light_id light)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return glm::vec3(); }
        return scenes[scene].mpoint_lights[light].mspecular_color;
    }

    float get_point_light_constant_attenuation(scene_id scene, point_light_id light)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return 0.0f; }
        return scenes[scene].mpoint_lights[light].mconstant_attenuation;
    }

    float get_point_light_linear_attenuation(scene_id scene, point_light_id light)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return 0.0f; }
        return scenes[scene].mpoint_lights[light].mlinear_attenuation;
    }

    float get_point_light_quadratic_attenuation(scene_id scene, point_light_id light)
    {
        if (!(scene < scenes.size() && light < scenes[scene].mpoint_lights.size())) { return 0.0f; }
        return scenes[scene].mpoint_lights[light].mquadratic_attenuation;
    }

    std::vector<node_id> get_descendant_nodes(scene_id l, node_id n)
    {
        std::vector<node_id> ret;
        struct node_context{ node_id nid; };
        std::queue<node_context> pending_nodes;
        pending_nodes.push({n});

        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();
            if (!is_node_enabled(l, current.nid)) continue; // note children are pruned

            // Nodes without meshes or without materials are ignored
            if (get_node_mesh(l, current.nid) != nmesh && get_node_material(l, current.nid) != nmat) {
              ret.push_back(current.nid);
            }

            for (node_id c = get_first_child_node(l, current.nid); c != nnode; c = get_next_sibling_node(l, c)) {
                pending_nodes.push({c});
            }
        }

        return ret;
    }
} // namespace cgs
