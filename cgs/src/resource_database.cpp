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
        mvertex_stride(3U),
        mtexture_coords(),
        mtexture_coords_stride(2U),
        mfaces(),
        mfaces_stride(3U),
        mnormals(),
        mmaterial(nmat) {}

      std::vector<float> mvertices;             //!< vertex coordinates
      std::size_t mvertex_stride;               //!< number of vertex coordinates per vertex
      std::vector<float> mtexture_coords;       //!< texture coordinates for each vertex
      std::size_t mtexture_coords_stride;       //!< number of texture coordinates per vertex
      std::vector<vindex> mfaces;               //!< faces, as a sequence of indexes over the logical vertex array
      std::size_t mfaces_stride;                //!< number of indexes per face
      std::vector<float> mnormals;              //!< normals of the mesh
      mat_id mmaterial;                         //!< material associated to the mesh
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

  void get_mesh_properties(mesh_id id, const float** vertex_base, std::size_t* vertex_stride,
                          const float** texture_coords, std::size_t* texture_coords_stride, std::size_t* num_vertices,
                          const vindex** faces, std::size_t* faces_stride, std::size_t* num_faces,
                          const float** normals, mat_id* material)
  {
    if (!(id < meshes.size() && vertex_base && vertex_stride && texture_coords && texture_coords_stride
        && num_vertices && faces && faces_stride && num_faces && material)) {
      log(LOG_LEVEL_ERROR, "get_mesh_properties error: invalid arguments"); return;
    }

    mesh& m = meshes[id];
    *vertex_base = &(m.mvertices[0]);
    *vertex_stride = m.mvertex_stride;
    *texture_coords = nullptr;
    if (m.mtexture_coords.size()) {
      *texture_coords = &(m.mtexture_coords[0]);
    }
    *texture_coords_stride = m.mtexture_coords_stride;
    *num_vertices = m.mvertices.size() / m.mvertex_stride;
    *faces = &(m.mfaces[0]);
    *faces_stride = m.mfaces_stride;
    *num_faces = m.mfaces.size() / m.mfaces_stride;
    *normals = nullptr;
    if (m.mnormals.size()) {
      *normals = &m.mnormals[0];      
    }
    *material = m.mmaterial;
  }

  void set_mesh_properties(mesh_id id, const float* vertex_base, std::size_t vertex_stride,
                          float* texture_coords, std::size_t texture_coords_stride, std::size_t num_vertices,
                          const vindex* faces, std::size_t faces_stride, std::size_t num_faces,
                          const float* normals, mat_id mat_id)
  {
    if (!(id < meshes.size() && vertex_base && vertex_stride && num_vertices && faces && faces_stride && num_faces)) {
      log(LOG_LEVEL_ERROR, "set_mesh_properties error: invalid arguments"); return;
    }

    mesh& m = meshes[id];
    std::size_t vertices_nelements = vertex_stride * num_vertices;
    m.mvertices.clear();
    m.mvertices.reserve(vertices_nelements);
    std::copy(vertex_base, vertex_base + vertices_nelements, std::back_inserter(m.mvertices));
    m.mvertex_stride = vertex_stride;
    if (texture_coords) {
      std::size_t texture_coords_nelements = texture_coords_stride * num_vertices;
      m.mtexture_coords.clear();
      m.mtexture_coords.reserve(texture_coords_nelements);
      std::copy(texture_coords, texture_coords + texture_coords_nelements, std::back_inserter(m.mtexture_coords));
      m.mtexture_coords_stride = texture_coords_stride;
    }
    std::size_t faces_nelements = faces_stride * num_faces;
    m.mfaces.clear();
    m.mfaces.reserve(faces_nelements);
    std::copy(faces, faces + faces_nelements, std::back_inserter(m.mfaces));
    m.mfaces_stride = faces_stride;
    std::size_t normals_nelements = num_vertices * 3;
    m.mnormals.reserve(normals_nelements);
    std::copy(normals, normals + normals_nelements, std::back_inserter(m.mnormals));
    m.mmaterial = mat_id;
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
