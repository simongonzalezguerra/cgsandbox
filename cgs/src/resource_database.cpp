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
        mmeshes(),
        mlocal_transform(1.0f),
        mfirst_child(nresource),
        mnext_sibling(nresource) {}

      std::vector<mesh_id> mmeshes;             //!< meshes contained in this resource
      glm::mat4            mlocal_transform;    //!< resource transform relative to the parent's reference frame
      resource_id          mfirst_child;        //!< first child resource of this resource
      resource_id          mnext_sibling;       //!< next sibling resource of this resource
    };

    typedef std::vector<resource> resource_vector;

    struct cubemap
    {
      cubemap() : mfaces() {}

      std::vector<std::string> mfaces;          //!< paths to image files containing the six faces of the cubemap
    };

    typedef std::vector<cubemap> cubemap_vector;

    //---------------------------------------------------------------------------------------------
    // Internal data structures
    //---------------------------------------------------------------------------------------------
    material_vector materials;                  //!< collection of all the materials
    mesh_vector meshes;                         //!< collection of all the meshes
    resource_vector resources;                  //!< collection of all the resources
    cubemap_vector cubemaps;                    //!< collection of all the cubemaps
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
    cubemaps.clear();
    cubemaps.reserve(10);
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

  void set_resource_meshes(resource_id r, const std::vector<mesh_id>& m)
  {
    if (!(r < resources.size())) {
      log(LOG_LEVEL_ERROR, "set_resource_meshes error: invalid arguments"); return;
    }

    for (std::size_t i = 0; i < m.size(); i++) {
      if (!(m[i] < meshes.size())) {
        log(LOG_LEVEL_ERROR, "set_resource_meshes error: invalid mesh id"); return;
      }
    }

    resources[r].mmeshes = m;
  }

  void set_resource_local_transform(resource_id r, const glm::mat4& local_transform)
  {
    if (!(r < resources.size())) {
      log(LOG_LEVEL_ERROR, "set_resource_local_transform error: invalid arguments"); return;
    }

    resources[r].mlocal_transform = local_transform;
  }

  std::vector<mesh_id> get_resource_meshes(resource_id r)
  {
    if (!(r < resources.size())) {
      log(LOG_LEVEL_ERROR, "get_resource_meshes error: invalid arguments"); return std::vector<mesh_id>();
    }

    return resources[r].mmeshes;
  }

  glm::mat4 get_resource_local_transform(resource_id r)
  {
    if (!(r < resources.size())) {
      log(LOG_LEVEL_ERROR, "get_resource_local_transform error: invalid arguments"); return glm::mat4{1.0f};
    }

    return resources[r].mlocal_transform;
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

  cubemap_id add_cubemap()
  {
    cubemaps.push_back(cubemap{});
    return cubemaps.size() - 1;
  }

  void set_cubemap_faces(cubemap_id id, const std::vector<std::string>& faces)
  {
    if (!(id < cubemaps.size())) {
      log(LOG_LEVEL_ERROR, "set_cubemap_faces error: invalid cubemap id"); return;
    }

    cubemaps[id].mfaces = faces;
  }

  std::vector<std::string> get_cubemap_faces(cubemap_id id)
  {
    if (!(id < cubemaps.size())) {
      log(LOG_LEVEL_ERROR, "get_cubemap_faces error: invalid cubemap id"); return std::vector<std::string>();
    }

    return cubemaps[id].mfaces;
  }

  cubemap_id get_first_cubemap()
  {
    return (cubemaps.size() ? 0 : ncubemap);
  }

  cubemap_id get_next_cubemap(cubemap_id id)
  {
    return (id + 1 < cubemaps.size()? id + 1 : ncubemap);
  }

} // namespace cgs
