#include "cgs/resource_database.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cgs/log.hpp"

#include <algorithm>
#include <vector>

namespace cgs
{
  namespace
  {
    //---------------------------------------------------------------------------------------------
    // Internal declarations
    //---------------------------------------------------------------------------------------------
    struct material
    {
      material() :
        mcolor_diffuse({1.0f, 1.0f, 1.0f}),
        mcolor_spec({1.0f, 1.0f, 1.0f}),
        msmoothness(1.0f),
        mtexture_path() {}

      glm::vec3 mcolor_diffuse;
      glm::vec3 mcolor_spec;
      float msmoothness;
      std::string mtexture_path;
    };

    typedef std::vector<material> material_vector;

    struct mesh
    {
      mesh() :
        mvertices(),
        mtexture_coords(),
        mnormals(),
        mindices(),
        mmaterial(nmat) {}

      std::vector<glm::vec3> mvertices;        //!< vertex coordinates
      std::vector<glm::vec2> mtexture_coords;  //!< texture coordinates for each vertex
      std::vector<glm::vec3> mnormals;         //!< normals of the mesh
      std::vector<vindex>    mindices;         //!< faces, as a sequence of indexes over the logical vertex array
      mat_id                 mmaterial;        //!< material associated to the mesh
    };

    typedef std::vector<mesh> mesh_vector;

    struct resource
    {
      resource() :
        mnum_meshes(0),
        mlocal_transform(1.0f),
        mfirst_child(nresource),
        mnext_sibling(nresource) {}

      mesh_id mmeshes[max_meshes_by_resource];  //!< meshes in this resource
      std::size_t mnum_meshes;                  //!< number of meshes in this resource
      glm::mat4 mlocal_transform;               //!< resource transform relative to the parent's reference frame
      resource_id mfirst_child;                 //!< first child resource of this resource
      resource_id mnext_sibling;                //!< next sibling resource of this resource
    };

    typedef std::vector<resource> resource_vector;

    //---------------------------------------------------------------------------------------------
    // Internal data structures
    //---------------------------------------------------------------------------------------------
    material_vector materials;                  //!< collection of all the materials
    mesh_vector meshes;                         //!< collection of all the meshes
    resource_vector resources;                  //!< collection of all the resources
  } // anonymous namespace

  //-----------------------------------------------------------------------------------------------
  // Public functions
  //-----------------------------------------------------------------------------------------------
  void resource_database_init()
  {
    materials.clear();
    materials.reserve(100);
    meshes.clear();
    meshes.reserve(100);
    resources.clear();
    resources.reserve(2048);
    resources.push_back(resource{});
  }

  mat_id add_material()
  {
    materials.push_back(material{});
    return materials.size() - 1;
  }

  void set_material_diffuse_color(mat_id m, glm::vec3 diffuse_color)
  {
    if (!(m < materials.size())) {
      log(LOG_LEVEL_ERROR, "set_material_diffuse_color error: invalid material id"); return;
    }

    materials[m].mcolor_diffuse = diffuse_color;
  }

  void set_material_specular_color(mat_id m, glm::vec3 specular_color)
  {
    if (!(m < materials.size())) {
      log(LOG_LEVEL_ERROR, "set_material_specular_color error: invalid material id"); return;
    }

    materials[m].mcolor_spec = specular_color;
  }

  void set_material_smoothness(mat_id m, float smoothness)
  {
    if (!(m < materials.size())) {
      log(LOG_LEVEL_ERROR, "set_material_smoothness error: invalid material id"); return;
    }

    materials[m].msmoothness = smoothness;
  }

  void set_material_texture_path(mat_id m, const std::string& texture_path)
  {
    if (!(m < materials.size())) {
      log(LOG_LEVEL_ERROR, "set_material_texture_path error: invalid material id"); return;
    }

    materials[m].mtexture_path = texture_path;
  }

  glm::vec3 get_material_diffuse_color(mat_id m)
  {
    if (!(m < materials.size())) {
      log(LOG_LEVEL_ERROR, "get_material_diffuse_color error: invalid material id"); return glm::vec3{1.0f};
    }

    return materials[m].mcolor_diffuse;    
  }

  glm::vec3 get_material_specular_color(mat_id m)
  {
    if (!(m < materials.size())) {
      log(LOG_LEVEL_ERROR, "get_material_specular_color error: invalid material id"); return glm::vec3{1.0f};
    }

    return materials[m].mcolor_spec;
  }

  float get_material_smoothness(mat_id m)
  {
    if (!(m < materials.size())) {
      log(LOG_LEVEL_ERROR, "get_material_smoothness error: invalid material id"); return 0.0f;
    }

    return materials[m].msmoothness;
  }

  std::string get_material_texture_path(mat_id m)
  {
    if (!(m < materials.size())) {
      log(LOG_LEVEL_ERROR, "get_material_texture_path error: invalid material id"); return "";
    }

    return materials[m].mtexture_path;
  }

  mat_id get_first_material()
  {
    return (materials.size() ? 0 : nmat);
  }

  mat_id get_next_material(mat_id m)
  {
    return (m + 1 < materials.size()? m + 1 : nmat);
  }

  mesh_id add_mesh()
  {
    meshes.push_back(mesh{});
    return meshes.size() - 1;
  }

  void set_mesh_vertices(mesh_id id, const std::vector<glm::vec3>& vertices)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "set_mesh_vertices error: invalid arguments"); return;
    }

    meshes[id].mvertices = vertices;
  }

  void set_mesh_texture_coords(mesh_id id, const std::vector<glm::vec2>& texture_coords)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "set_mesh_texture_coords error: invalid arguments"); return;
    }

    meshes[id].mtexture_coords = texture_coords;
  }

  void set_mesh_normals(mesh_id id, const std::vector<glm::vec3>& normals)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "set_mesh_normals error: invalid arguments"); return;
    }

    meshes[id].mnormals = normals;
  }

  void set_mesh_indices(mesh_id id, const std::vector<vindex>& indices)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "set_mesh_indices error: invalid arguments"); return;
    }

    meshes[id].mindices = indices;
  }

  void set_mesh_material(mesh_id id, mat_id material)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "set_mesh_material error: invalid arguments"); return;
    }

    meshes[id].mmaterial = material;
  }

  std::vector<glm::vec3> get_mesh_vertices(mesh_id id)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "get_mesh_vertices error: invalid arguments"); return std::vector<glm::vec3>();
    }

    return meshes[id].mvertices;
  }

  std::vector<glm::vec2> get_mesh_texture_coords(mesh_id id)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "get_mesh_texture_coords error: invalid arguments"); return std::vector<glm::vec2>();
    }

    return meshes[id].mtexture_coords;
  }

  std::vector<glm::vec3> get_mesh_normals(mesh_id id)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "get_mesh_normals error: invalid arguments"); return std::vector<glm::vec3>();
    }

    return meshes[id].mnormals;
  }

  std::vector<vindex> get_mesh_indices(mesh_id id)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "get_mesh_indices error: invalid arguments"); return std::vector<vindex>();
    }

    return meshes[id].mindices;
  }

  mat_id get_mesh_material(mesh_id id)
  {
    if (!(id < meshes.size())) {
      log(LOG_LEVEL_ERROR, "get_mesh_material error: invalid arguments"); return nmat;
    }

    return meshes[id].mmaterial;
  }

  mesh_id get_first_mesh()
  {
    return (meshes.size() ? 0 : nmesh);
  }

  mesh_id get_next_mesh(mesh_id m)
  {
    return (m + 1 < meshes.size()? m + 1 : nmesh);
  }

  resource_id add_resource(resource_id p)
  {
    if (!(p < resources.size())) {
      log(LOG_LEVEL_ERROR, "add_resource error: invalid resource id for parent"); return nresource;
    }

    // Create a new resource and link it as last child of p
    resources.push_back(resource{});
    resource_id r = resources[p].mfirst_child;
    resource_id next = nresource;
    while (r != nresource && ((next = resources[r].mnext_sibling) != nresource)) {
      r = next;
    }
    (r == nresource? resources[p].mfirst_child : resources[r].mnext_sibling) = resources.size() - 1;

    return resources.size() - 1;
  }

  void get_resource_properties(resource_id r, const mesh_id** meshes, std::size_t* num_meshes, const float** local_transform)
  {
    if (!(r < resources.size() && meshes && num_meshes && local_transform)) {
      log(LOG_LEVEL_ERROR, "get_resource_properties error: invalid arguments"); return;
    }

    *meshes = resources[r].mmeshes;
    *num_meshes = resources[r].mnum_meshes;
    *local_transform = glm::value_ptr(resources[r].mlocal_transform);
  }

  void set_resource_properties(resource_id r, mesh_id* m, std::size_t num_meshes, float* local_transform)
  {
    if (!(r < resources.size() && (num_meshes == 0 || m != nullptr) && local_transform)) {
      log(LOG_LEVEL_ERROR, "set_resource_properties error: invalid arguments"); return;
    }

    for (std::size_t i = 0; i < num_meshes && m; i++) {
      if (!(m[i] < meshes.size())) {
        log(LOG_LEVEL_ERROR, "set_resource_properties error: invalid mesh id"); return;
      }
    }

    for (std::size_t i = 0; i < num_meshes; i++) {
      resources[r].mmeshes[i] = m[i];
    }
    resources[r].mnum_meshes = num_meshes;

    for (std::size_t i = 0; i < 16; i++) {
      glm::value_ptr(resources[r].mlocal_transform)[i] = local_transform[i];
    }
  }

  resource_id get_first_child_resource(resource_id r)
  {
    if (!(r < resources.size())) {
      log(LOG_LEVEL_ERROR, "get_first_child_resource error: invalid resource id"); return nresource;
    }

    return resources[r].mfirst_child;
  }

  resource_id get_next_sibling_resource(resource_id r)
  {
    if (!(r < resources.size())) {
      log(LOG_LEVEL_ERROR, "get_next_sibling_resource error: invalid resource id"); return nresource;
    }

    return resources[r].mnext_sibling;
  }
} // namespace cgs
