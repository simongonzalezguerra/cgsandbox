#include "glsandbox/scenegraph.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glsandbox/log.hpp"
#include "glm/glm.hpp"

#include <algorithm>
#include <vector>
#include <queue>

namespace glsandbox
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
        mnum_meshes(0U),
        mlocal_transform(1.0f),
        maccum_transform(1.0f),
        mfirst_child(nnode),
        mnext_sibling(nnode),
        menabled(true),
        mused(true) {}

      mesh_id mmeshes[max_meshes_by_resource];  //!< meshes in this node
      std::size_t mnum_meshes;                  //!< number of meshes in this node
      glm::mat4 mlocal_transform;               //!< node transform relative to the parent
      glm::mat4 maccum_transform;               //!< node transform relative to the root
      node_id mfirst_child;                     //!< first child node of this node
      node_id mnext_sibling;                    //!< next sibling node of this node
      bool menabled;                            //!< is this node enabled? (if it is not, all descendants are ignored when rendering)
      bool mused;                               //!< is this cell used? (used for soft deletion)
    };

    typedef std::vector<node> node_vector;
    typedef node_vector::iterator node_iterator;

    struct layer
    {
      layer() :
        mview(0),
        menabled(false),
        mnext_layer(nlayer),
        mlight(light_data{}),
        mview_transform{1.0f},
        mprojection_transform{1.0f} {}

      view_id mview;                     //!< id of the view this layer belongs to
      bool menabled;                     //!< is this layer enabled?
      layer_id mnext_layer;              //!< id of the next layer in the view this layer belongs to
      node_vector mnodes;                //!< collection of all the nodes in this layer
      light_data mlight;                 //!< light source (one for the whole layer)
      glm::mat4 mview_transform;         //!< the view transform used to render all objects in the layer
      glm::mat4 mprojection_transform;   //!< the projection transform used to render all objects in the layer
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

      const float* local_transform_out = nullptr;
      std::size_t num_meshes_out = 0U;
      const mesh_id* meshes_out = nullptr;
      get_resource_properties(current.rid, &meshes_out, &num_meshes_out, &local_transform_out);

      node_id n = add_node(l, current.parent);
      set_node_transform(l, n, local_transform_out);
      set_node_meshes(l, n, meshes_out, num_meshes_out);
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

  void set_node_transform(layer_id l, node_id n, const float* local_transform)
  {
    if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused && local_transform)) {
      log(LOG_LEVEL_ERROR, "set_node_transform error: invalid parameters"); return;
    }

    for (std::size_t i = 0; i < 16; i++) {
      glm::value_ptr(layers[l].mnodes[n].mlocal_transform)[i] = local_transform[i];
    }

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

  void get_node_transform(layer_id l, node_id n, const float** local_transform, const float** accum_transform)
  {
    if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused && local_transform && accum_transform)) {
      log(LOG_LEVEL_ERROR, "get_node_transform error: invalid parameters"); return;
    }

    *local_transform = glm::value_ptr(layers[l].mnodes[n].mlocal_transform);
    *accum_transform = glm::value_ptr(layers[l].mnodes[n].maccum_transform);
  }

  void set_node_meshes(layer_id l, node_id n, const mesh_id* m, std::size_t num_meshes)
  {
    if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused && m)) {
      log(LOG_LEVEL_ERROR, "set_node_meshes error: invalid parameters"); return;
    }

    std::size_t added_meshes = 0U;
    for (std::size_t i = 0; i < num_meshes; i++) {
      for (mesh_id mid = get_first_mesh(); mid != nmesh; mid = get_next_mesh(mid)) {
        if (mid == m[i]) {
          layers[l].mnodes[n].mmeshes[i] = m[i];
          added_meshes++;
          break;
        }
      }
    }
    layers[l].mnodes[n].mnum_meshes = added_meshes;
  }

  void get_node_meshes(layer_id l, node_id n, const mesh_id** meshes, std::size_t* num_meshes)
  {
    if (!(l < layers.size() && n < layers[l].mnodes.size() && layers[l].mnodes[n].mused && meshes && num_meshes)) {
      log(LOG_LEVEL_ERROR, "get_node_meshes error: invalid parameters"); return;
    }

    *meshes = layers[l].mnodes[n].mmeshes;
    *num_meshes = layers[l].mnodes[n].mnum_meshes;
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

  void get_light_data(layer_id l, light_data* data)
  {
    if (!(l < layers.size() && data)) {
      log(LOG_LEVEL_ERROR, "get_light_data error: invalid parameters"); return;
    }

    *data = layers[l].mlight;
  }

  void set_light_data(layer_id l, const light_data* data)
  {
    if (!(l < layers.size() && data)) {
      log(LOG_LEVEL_ERROR, "get_light_data error: invalid parameters"); return;
    }

    layers[l].mlight = *data;
  }
} // namespace glsandbox
