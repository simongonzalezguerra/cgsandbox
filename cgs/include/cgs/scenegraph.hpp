#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

#include "cgs/resource_database.hpp"
#include "glm/glm.hpp"

#include <cstddef> // for size_t

namespace cgs
{
  //-----------------------------------------------------------------------------------------------
  // Types
  //-----------------------------------------------------------------------------------------------

  //-----------------------------------------------------------------------------------------------
  //! @brief Handle to a view.
  //-----------------------------------------------------------------------------------------------
  typedef std::size_t view_id;

  //-----------------------------------------------------------------------------------------------
  //! @brief Handle to a layer.
  //-----------------------------------------------------------------------------------------------
  typedef std::size_t layer_id;

  //-----------------------------------------------------------------------------------------------
  //! @brief Handle to a node.
  //-----------------------------------------------------------------------------------------------
  typedef std::size_t node_id;

  struct light_data
  {
    light_data() :
      mposition{0.0f, 0.0f, 0.0f},
      mdiffuse_power(1.0f),
      mdiffuse_color{1.0f, 1.0f, 1.0f},
      mspecular_power(1.0f),
      mspecular_color{1.0f, 1.0f, 1.0f} {}

    float mposition[3];
    float mdiffuse_power;
    float mdiffuse_color[3];
    float mspecular_power;
    float mspecular_color[3];
  };

  //-----------------------------------------------------------------------------------------------
  // Constants
  //-----------------------------------------------------------------------------------------------

  //-----------------------------------------------------------------------------------------------
  //! @brief Constant representing 'not a layer'. Used as a wildcard when iterating the
  //!  layers of a view to indicate the end of the sequence has been reached.
  //-----------------------------------------------------------------------------------------------
  constexpr layer_id nlayer = -1;

  //-----------------------------------------------------------------------------------------------
  //! @brief Constant representing 'not a node'. Used as a wildcard when iterating the
  //!  children of a node to indicate the end of the sequence has been reached.
  //-----------------------------------------------------------------------------------------------
  constexpr node_id nnode = -1;

  //-----------------------------------------------------------------------------------------------
  //! @brief Id of the root node of each layer. This node is the root of the the layer's node 
  //! hierarchy. It it created when the layer is created and can't be removed.
  //-----------------------------------------------------------------------------------------------
  constexpr node_id root_node = 0;

  //-----------------------------------------------------------------------------------------------
  //! @brief Initializes scene graph state.
  //! @remark This function clears all existing views, layers and nodes.
  //! @remark This function can be called several times during program execution.
  //-----------------------------------------------------------------------------------------------
  void scenegraph_init();

  //-----------------------------------------------------------------------------------------------
  //! @brief Creates a view.
  //! @remark The view is created as enabled.
  //! @return The handle to the new view.
  //-----------------------------------------------------------------------------------------------
  view_id add_view();

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns whether a view is enabled.
  //! @param vid Handle to the view.
  //! @return true if the view is enabled, false otherwise
  //-----------------------------------------------------------------------------------------------
  bool is_view_enabled(view_id vid);

  //-----------------------------------------------------------------------------------------------
  //! @brief Sets the enabled flag of a view.
  //! @param vid Handle to the view.
  //! @param enabled Value to set in the enabled flag of the view.
  //-----------------------------------------------------------------------------------------------
  void set_view_enabled(view_id vid, bool enabled);

  //-----------------------------------------------------------------------------------------------
  //! @brief Adds a layer to a view.
  //! @param view Id of the view to add the layer to.
  //! @return Id of the new layer, or nlayer on error.
  //-----------------------------------------------------------------------------------------------
  layer_id add_layer(view_id view);

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns the id of the first layer of a view.
  //! @param view The view to query.
  //! @return Id of the first layer if the view. If the view has no layers, or an error occurs,
  //!  nlayer is returned.
  //-----------------------------------------------------------------------------------------------
  layer_id get_first_layer(view_id view);

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns the id of the next layer in the sequence of layers of a view.
  //! @param layer Current layer.
  //! @return The id of the next layer. If the end of the sequence has been reached or an error
  //!  occurs, nlayer is returned.
  //-----------------------------------------------------------------------------------------------
  layer_id get_next_layer(layer_id layer);

  void set_layer_view_transform(layer_id layer, const glm::mat4& view_transform);
  void get_layer_view_transform(layer_id layer, glm::mat4* view_transform);
  void set_layer_projection_transform(layer_id layer, const glm::mat4& projection_transform);
  void get_layer_projection_transform(layer_id layer, glm::mat4* projection_transform);

  //-----------------------------------------------------------------------------------------------
  //! @brief Returns if a layer is enabled for rendering or not.
  //! @param layer Id of the layer to query.
  //! @return True if the layer is enabled, false otherwise.
  //-----------------------------------------------------------------------------------------------
  bool is_layer_enabled(layer_id layer);

  //-----------------------------------------------------------------------------------------------
  //! @brief Sets the enabled flag of a layer.
  //! @param layer If of the layer to set the flag to.
  //! @param enabled Value to set to the flag
  //! @remarks If a layer has its enabled flag set to false then none of its nodes should be
  //!  rendered.
  //-----------------------------------------------------------------------------------------------
  void set_layer_enabled(layer_id layer, bool enabled);

  //-----------------------------------------------------------------------------------------------
  //! @brief Adds a node to the node hierarchy.
  //! @param layer Id of the layer to add the node to.
  //! @param parent If of the parent node within that layer to insert the new node under.
  //! @return The id of the new node.
  //-----------------------------------------------------------------------------------------------
  node_id add_node(layer_id layer, node_id parent);

  //-----------------------------------------------------------------------------------------------
  //! @brief Adds a node to the node hierarchy by instantiating a resource tree. 
  //! @param layer Id of the layer to add the node to.
  //! @param parent Id of the parent node within that layer to insert the new node under.
  //! @param resource Id of the resource to instantiate.
  //! @return The id of the new node.
  //! @remarks One node node will created for the given resource and each of its descendants,
  //!  containing the same meshes and local transforms.
  //-----------------------------------------------------------------------------------------------
  node_id add_node(layer_id layer, node_id parent, resource_id resource);

  //-----------------------------------------------------------------------------------------------
  //! @brief Removes a node from the node hierarchy.
  //! @param layer Id of the layer the node to remove belongs to.
  //! @param node If of the node to remove within that layer.
  //! @remarks The root node of a layer (node id = 0) cannot be removed.
  //-----------------------------------------------------------------------------------------------
  void remove_node(layer_id layer, node_id node);

  //-----------------------------------------------------------------------------------------------
  //! @brief Updates the local and accumulated transforms of a node.
  //! @param layer Id of the layer the node belongs to.
  //! @param node Id of the node within that layer.
  //! @param local transform An array containing the local transform that should be assigned to
  //!  the node, in column-major format (that is, one column, then the other, and so on),
  //!  expressed in the coordinate system of the  parent node.
  //! @remarks The accumulated transform of the node and all its descendants are updated. The
  //!  accumulated transform of a node is the product of the local transforms of all the nodes
  //!  along the path from the root node to the node, starting from the root and including
  //!  the node.
  //-----------------------------------------------------------------------------------------------
  void set_node_transform(layer_id layer, node_id node, const float* local_transform);

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the local and accumulated transforms of a node.
  //! @param layer Id of the layer the node belongs to.
  //! @param node Id of the node within that layer.
  //! @param local_transform Address of a pointer to store the address of an  internal array
  //!  representing the nodes's local transform as a matrix in column major format (that is, one
  //!  column, then the other, and so on). Can't be nullptr.
  //! @param accum_transform Address of a pointer to store the address of an  internal array
  //!  representing the nodes's accumulated transform as a matrix in column major format. Can't
  //!  be nullptr. See set_node_transform for the meaning of this value.
  //-----------------------------------------------------------------------------------------------
  void get_node_transform(layer_id layer, node_id node, const float** local_transform, const float** accumm_transform);

  //-----------------------------------------------------------------------------------------------
  //! @brief Updates the list of meshes contained in a node.
  //! @param layer Id of the layer the node belongs to.
  //! @param node Id of the node within that layer.
  //! @param meshes An array of num_meshes elements containing the list of meshes that should
  //!  be contained in the node. Can't be nullptr.
  //! @param num_meshes The number of elements of the meshes array. Can be zero to indicate an
  //!  empty set of meshes.
  //! @remarks If some of the meshes in the provided array do not exist, they will not be added.
  //-----------------------------------------------------------------------------------------------
  void set_node_meshes(layer_id layer, node_id node, const mesh_id* meshes, std::size_t num_meshes);

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the list of meshes contained in a node.
  //! @param layer Id of the layer the node belongs to.
  //! @param node Id of the node within that layer.
  //! @param meshes Address of a pointer to store the address of an internal array containing the
  //!  list of meshes in the node. Can't be nullptr.
  //! @param num_meshes Address of an object to store the number of meshes contained in the
  //!  resource node. Can't be nullptr.
  //-----------------------------------------------------------------------------------------------
  void get_node_meshes(layer_id layer, node_id node, const mesh_id** meshes, std::size_t* num_meshes);

  //-----------------------------------------------------------------------------------------------
  //! @brief Updates the enabled flag of a node, which determines if the node and its descendants
  //!  will be rendered.
  //! @param layer Id of the layer the node belongs to.
  //! @param node Id of the node within that layer.
  //! @param enabled Value to set to the flag.
  //! @remarks When a node is first created, it is enabled by default.
  //-----------------------------------------------------------------------------------------------
  void set_node_enabled(layer_id layer, node_id node, bool enabled);

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the value of the enabled flag of a node. See set_node_enabled for the meaning
  //!  of this value.
  //! @param layer Id of the layer the node belongs to.
  //! @param node Id of the node within that layer.
  //! @return True of the node is found and is enabled, false otherwise.
  //! @remarks
  //-----------------------------------------------------------------------------------------------
  bool is_node_enabled(layer_id layer, node_id node);

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the id of the fist child of a node.
  //! @param layer Id of the layer the node belongs to.
  //! @param node Id of the node within that layer.
  //! @return The id of the first child node, or nnode if the node is not found.
  //-----------------------------------------------------------------------------------------------
  node_id get_first_child_node(layer_id layer, node_id node);

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the id of the next sibling of a node.
  //! @param layer Id of the layer the node belongs to.
  //! @param node Id of the node within that layer.
  //! @return The id of the next sibling node, or nnode if the node is not found.
  //-----------------------------------------------------------------------------------------------
  node_id get_next_sibling_node(layer_id layer, node_id node);

  //-----------------------------------------------------------------------------------------------
  //! @brief Reads the light data of a layer.
  //! @param layer Id of the layer to query.
  //! @param data Address of an object to write light data to. Can't be nullptr.
  //-----------------------------------------------------------------------------------------------
  void get_light_data(layer_id layer, light_data* data);

  //-----------------------------------------------------------------------------------------------
  //! @brief Writes the light data of a layer.
  //! @param layer The layer to write to.
  //! @param data Address of an object containing light data to write.
  //-----------------------------------------------------------------------------------------------
  void set_light_data(layer_id layer, const light_data* data);
} // namespace cgs

#endif // SCENE_GRAPH_HPP
