#include "cgs/scenegraph.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cgs/log.hpp"
#include "glm/glm.hpp"

#include <algorithm>
#include <vector>
#include <queue>

namespace cgs
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        struct view
        {
            view() : menabled(true), mfirst_layer(nlayer) {}
            bool menabled;           //<! is this view enabled?
            layer_id mfirst_layer;   //<! id of the first layer of this view
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

        struct layer
        {
            layer() :
                mview(0),
                menabled(false),
                mnext_layer(nlayer),
                mview_transform{1.0f},
                mprojection_transform{1.0f},
                mskybox(ncubemap),
                mpoint_lights() {}

            view_id            mview;                             //!< id of the view this layer belongs to
            bool               menabled;                          //!< is this layer enabled?
            layer_id           mnext_layer;                       //!< id of the next layer in the view this layer belongs to
            node_vector        mnodes;                            //!< collection of all the nodes in this layer
            glm::mat4          mview_transform;                   //!< the view transform used to render all objects in the layer
            glm::mat4          mprojection_transform;             //!< the projection transform used to render all objects in the layer
            cubemap_id         mskybox;                           //!< the id of the cubemap to use as skybox (can be ncubemap)
            glm::vec3          mdirectional_light_ambient_color;  //!< ambient color of the directional light
            glm::vec3          mdirectional_light_diffuse_color;  //!< diffuse color of the directional light
            glm::vec3          mdirectional_light_specular_color; //!< specular color of the directional light
            glm::vec3          mdirectional_light_direction;      //!< direction of the directional light
            point_light_vector mpoint_lights;                     //!< collection of all the point lights in this layer
        };

        typedef std::vector<layer> layer_vector;

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        view_vector views;          //!< collection of all the views
        layer_vector layers;        //!< collection of all the layers

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        layer_id get_last_layer(view_id v)
        {
            layer_id l = views[v].mfirst_layer;
            layer_id next = nlayer;
            while (l != nlayer && ((next = layers[l].mnext_layer) != nlayer)) {
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
        layers.clear();
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

    layer_id add_layer(view_id v)
    {
        if (!(v < views.size())) {
            log(LOG_LEVEL_ERROR, "add_layer error: invalid view id");
            return nlayer;
        }

        layers.push_back(layer{});
        layer& lr = layers[layers.size() - 1];
        lr.mview = v;
        lr.menabled = true;
        lr.mnext_layer = nlayer;
        lr.mnodes.clear();
        lr.mnodes.reserve(2048);
        lr.mnodes.push_back(node{});
        lr.mview_transform = glm::mat4{1.0f};
        lr.mprojection_transform = glm::mat4{1.0f};

        layer_id lid = layers.size() - 1;
        layer_id last_layer = get_last_layer(v);
        (last_layer == nlayer? views[v].mfirst_layer : layers[last_layer].mnext_layer) = lid;
        return lid;
    }

    bool is_layer_enabled(layer_id l)
    {
        if (!(l < layers.size())) {
            log(LOG_LEVEL_ERROR, "is_layer_enabled error: invalid layer id"); return false;
        }
        return layers[l].menabled;
    }

    void set_layer_enabled(layer_id l, bool enabled)
    {
        if (!(l < layers.size())) {
            log(LOG_LEVEL_ERROR, "set_layer_enabled error: invalid layer id"); return;
        }
        layers[l].menabled = enabled;
    }

    layer_id get_first_layer(view_id v)
    {
        if (!(v < views.size())) {
            log(LOG_LEVEL_ERROR, "get_first_layer error: invalid view id");
            return nlayer;
        }
        return views[v].mfirst_layer;
    }

    layer_id get_next_layer(layer_id l)
    {
        if (!(l < layers.size())) {
            log(LOG_LEVEL_ERROR, "get_next_layer error: invalid layer id"); return nlayer;
        }
        return layers[l].mnext_layer;
    }

    void set_layer_view_transform(layer_id l, const glm::mat4& view_transform)
    {
        if (!(l < layers.size())) {
            log(LOG_LEVEL_ERROR, "set_layer_view_transform error: invalid layer id"); return;
        }

        layers[l].mview_transform = view_transform;
    }

    void get_layer_view_transform(layer_id l, glm::mat4* view_transform)
    {
        if (!(l < layers.size() && view_transform)) {
            log(LOG_LEVEL_ERROR, "get_layer_view_transform error: invalid layer id"); return;
        }

        *view_transform = layers[l].mview_transform;
    }

    void set_layer_projection_transform(layer_id l, const glm::mat4& projection_transform)
    {
        if (!(l < layers.size())) {
            log(LOG_LEVEL_ERROR, "set_layer_projection_transform error: invalid arguments"); return;
        }

        layers[l].mprojection_transform = projection_transform;
    }

    void get_layer_projection_transform(layer_id l, glm::mat4* projection_transform)
    {
        if (!(l < layers.size() && projection_transform)) {
            log(LOG_LEVEL_ERROR, "get_layer_projection_transform error: invalid arguments"); return;
        }

        *projection_transform = layers[l].mprojection_transform;
    }

    void set_layer_skybox(layer_id l, cubemap_id id)
    {
        if (!(l < layers.size())) {
            log(LOG_LEVEL_ERROR, "set_layer_skybox error: invalid arguments"); return;
        }

        layers[l].mskybox = id;
    }

    cubemap_id get_layer_skybox(layer_id l)
    {
        if (!(l < layers.size())) {
            log(LOG_LEVEL_ERROR, "get_layer_skybox error: invalid arguments"); return ncubemap;
        }

        return layers[l].mskybox;
    }

    node_id add_node(layer_id l, node_id parent)
    {
        if (!(l < layers.size() && parent < layers[l].mnodes.size() && layers[l].mnodes[parent].mused)) {
            log(LOG_LEVEL_ERROR, "add_node error: invalid parameters"); return nnode;
        }

        node_id child = allocate_node(layers[l].mnodes);
        node_id r = layers[l].mnodes[parent].mfirst_child;
        node_id next = nnode;
        while (r != nnode && ((next = layers[l].mnodes[r].mnext_sibling) != nnode)) {
            r = next;
        }
        (r == nnode? layers[l].mnodes[parent].mfirst_child : layers[l].mnodes[r].mnext_sibling) = child;

        return child;
    }

    node_id add_node(layer_id l, node_id p, resource_id root)
    {
        if (!(l < layers.size() && p < layers[l].mnodes.size() && layers[l].mnodes[p].mused)) {
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

    void remove_node(layer_id l, node_id n)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "remove_node error: invalid parameters"); return;
        }

        auto nit = layers[l].mnodes.begin();
        while (nit != layers[l].mnodes.end()) {
            if (nit->mused && nit->mfirst_child == n) {
                nit->mfirst_child = layers[l].mnodes[n].mnext_sibling;
                break;
            } else if (nit->mused && nit->mnext_sibling == n) {
                nit->mnext_sibling = layers[l].mnodes[n].mnext_sibling;
                break;
            }
            nit++;
        }

        if (nit == layers[l].mnodes.end()) {
            log(LOG_LEVEL_ERROR, "remove_node error: attempting to remove layer's root node"); return;
        }

        layers[l].mnodes[n].mused = false; // soft removal
    }

    void set_node_transform(layer_id l, node_id n, const glm::mat4& local_transform)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "set_node_transform error: invalid parameters"); return;
        }

        layers[l].mnodes[n].mlocal_transform = local_transform;

        // Update the accummulated transforms of n and all its descendant nodes with a breadth-first search
        struct context{ node_id nid; bool intree; glm::mat4 prefix; };
        std::queue<context> pending_nodes;
        pending_nodes.push({root_node, n == root_node, glm::mat4{1.0f}});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();
            glm::mat4 accum_transform = current.prefix * layers[l].mnodes[current.nid].mlocal_transform;
            if (current.intree || current.nid == n) {
                layers[l].mnodes[current.nid].maccum_transform = accum_transform;
            }
            for (node_id child = layers[l].mnodes[current.nid].mfirst_child; child != nnode; child = layers[l].mnodes[child].mnext_sibling) {
                pending_nodes.push({child, current.intree || current.nid == n, accum_transform});
            }
        }
    }

    void get_node_transform(layer_id l, node_id n, glm::mat4* local_transform, glm::mat4* accum_transform)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused && local_transform && accum_transform)) {
            log(LOG_LEVEL_ERROR, "get_node_transform error: invalid parameters"); return;
        }

        *local_transform = layers[l].mnodes[n].mlocal_transform;
        *accum_transform = layers[l].mnodes[n].maccum_transform;
    }

    void set_node_meshes(layer_id l, node_id n, const std::vector<mesh_id>&)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "set_node_meshes error: invalid parameters"); return;
        }

        //layers[l].mnodes[n].mmeshes = m;
    }

    std::vector<mesh_id> get_node_meshes(layer_id l, node_id n)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "get_node_meshes error: invalid parameters"); return std::vector<mesh_id>();
        }

        return std::vector<mesh_id>();
    }

    void set_node_mesh(layer_id l, node_id n, mesh_id m)
    {
        if (l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused) {
            layers[l].mnodes[n].mmesh = m;
        }
    }

    mesh_id get_node_mesh(layer_id l, node_id n)
    {
        return ((l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)? layers[l].mnodes[n].mmesh : nmesh);
    }

    void set_node_material(layer_id l, node_id n, mat_id mat)
    {
        if (l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused) {
            layers[l].mnodes[n].mmaterial = mat;
        }
    }

    mat_id get_node_material(layer_id l, node_id n)
    {
        return ((l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)? layers[l].mnodes[n].mmaterial : nmat);
    }

    void set_node_enabled(layer_id l, node_id n, bool enabled)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "set_node_enabled error: invalid parameters"); return;
        }

        layers[l].mnodes[n].menabled = enabled;
    }

    bool is_node_enabled(layer_id l, node_id n)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "is_node_enabled error: invalid parameters"); return false;
        }

        return layers[l].mnodes[n].menabled;
    }

    node_id get_first_child_node(layer_id l, node_id n)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "get_first_child_node error: invalid parameters"); return nnode;
        }

        return layers[l].mnodes[n].mfirst_child;
    }

    node_id get_next_sibling_node(layer_id l, node_id n)
    {
        if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused)) {
            log(LOG_LEVEL_ERROR, "get_next_sibling_node error: invalid parameters"); return nnode;
        }

        return layers[l].mnodes[n].mnext_sibling;
    }

    void set_directional_light_ambient_color(layer_id l, glm::vec3 ambient_color)
    {
        if (l < layers.size()) {
            layers[l].mdirectional_light_ambient_color = ambient_color;
        }
    }

    void set_directional_light_diffuse_color(layer_id l, glm::vec3 diffuse_color)
    {
        if (l < layers.size()) {
            layers[l].mdirectional_light_diffuse_color = diffuse_color;
        }
    }

    void set_directional_light_specular_color(layer_id l, glm::vec3 specular_color)
    {
        if (l < layers.size()) {
            layers[l].mdirectional_light_specular_color = specular_color;
        }
    }

    void set_directional_light_direction(layer_id l, glm::vec3 direction)
    {
        if (l < layers.size()) {
            layers[l].mdirectional_light_direction = direction;
        }
    }

    glm::vec3 get_directional_light_ambient_color(layer_id l)
    {
        return (l < layers.size()? layers[l].mdirectional_light_ambient_color : glm::vec3());
    }

    glm::vec3 get_directional_light_diffuse_color(layer_id l)
    {
        return (l < layers.size()? layers[l].mdirectional_light_diffuse_color : glm::vec3());
    }

    glm::vec3 get_directional_light_specular_color(layer_id l)
    {
        return (l < layers.size()? layers[l].mdirectional_light_specular_color : glm::vec3());
    }

    glm::vec3 get_directional_light_direction(layer_id l)
    {
        return (l < layers.size()? layers[l].mdirectional_light_direction : glm::vec3());
    }

    point_light_id add_point_light(layer_id layer)
    {
        if (!(layer < layers.size())) { return npoint_light; }
        layers[layer].mpoint_lights.push_back(point_light{});
        return layers[layer].mpoint_lights.size() - 1;
    }

    point_light_id get_first_point_light(layer_id layer)
    {
        if (!(layer < layers.size())) { return npoint_light; }
        return (layers[layer].mpoint_lights.size()? 0U : npoint_light);
    }

    point_light_id get_next_point_light(layer_id layer, point_light_id light)
    {
        if (!(layer < layers.size())) { return npoint_light; }
        return (light + 1 < layers[layer].mpoint_lights.size()? light + 1 : npoint_light);
    }

    void set_point_light_position(layer_id layer, point_light_id light, glm::vec3 position)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return; }
        layers[layer].mpoint_lights[light].mposition = position;
    }

    void set_point_light_ambient_color(layer_id layer, point_light_id light, glm::vec3 ambient_color)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return; }
        layers[layer].mpoint_lights[light].mambient_color = ambient_color;
    }

    void set_point_light_diffuse_color(layer_id layer, point_light_id light, glm::vec3 diffuse_color)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return; }
        layers[layer].mpoint_lights[light].mdiffuse_color = diffuse_color;
    }

    void set_point_light_specular_color(layer_id layer, point_light_id light, glm::vec3 specular_color)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return; }
        layers[layer].mpoint_lights[light].mspecular_color = specular_color;
    }

    void set_point_light_constant_attenuation(layer_id layer, point_light_id light, float constant_attenuation)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return; }
        layers[layer].mpoint_lights[light].mconstant_attenuation = constant_attenuation;
    }

    void set_point_light_linear_attenuation(layer_id layer, point_light_id light, float linear_attenuation)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return; }
        layers[layer].mpoint_lights[light].mlinear_attenuation = linear_attenuation;
    }

    void set_point_light_quadratic_attenuation(layer_id layer, point_light_id light, float quadratic_attenuation)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return; }
        layers[layer].mpoint_lights[light].mquadratic_attenuation = quadratic_attenuation;
    }

    glm::vec3 get_point_light_position(layer_id layer, point_light_id light)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return glm::vec3(); }
        return layers[layer].mpoint_lights[light].mposition;
    }

    glm::vec3 get_point_light_ambient_color(layer_id layer, point_light_id light)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return glm::vec3(); }
        return layers[layer].mpoint_lights[light].mambient_color;
    }

    glm::vec3 get_point_light_diffuse_color(layer_id layer, point_light_id light)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return glm::vec3(); }
        return layers[layer].mpoint_lights[light].mdiffuse_color;
    }

    glm::vec3 get_point_light_specular_color(layer_id layer, point_light_id light)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return glm::vec3(); }
        return layers[layer].mpoint_lights[light].mspecular_color;
    }

    float get_point_light_constant_attenuation(layer_id layer, point_light_id light)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return 0.0f; }
        return layers[layer].mpoint_lights[light].mconstant_attenuation;
    }

    float get_point_light_linear_attenuation(layer_id layer, point_light_id light)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return 0.0f; }
        return layers[layer].mpoint_lights[light].mlinear_attenuation;
    }

    float get_point_light_quadratic_attenuation(layer_id layer, point_light_id light)
    {
        if (!(layer < layers.size() && light < layers[layer].mpoint_lights.size())) { return 0.0f; }
        return layers[layer].mpoint_lights[light].mquadratic_attenuation;
    }

} // namespace cgs
