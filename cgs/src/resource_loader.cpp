#include "cgs/resource_loader.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "assimp/postprocess.h"
#include "cgs/log.hpp"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "glm/glm.hpp"

#include <iomanip>
#include <cstring>
#include <sstream>
#include <queue>
#include <map>

#ifndef _WIN32
#define PATH_SEPARATOR  "/"
#define CURRENT_DIR     "./"
#else
#define PATH_SEPARATOR  "\\"
#define CURRENT_DIR     ""
#endif

namespace cgs
{
  namespace
  {
    //---------------------------------------------------------------------------------------------
    // Internal declarations
    //---------------------------------------------------------------------------------------------
    std::vector<mat_id> added_materials;         //!< List of materials created in the last call to load_resources()
    std::map<std::size_t, mat_id> material_ids;  //!< Map from index in assimp's material array to resource database material_id
    std::vector<mesh_id> added_meshes;           //!< List of meshes created in the last call to load_resources()
    std::map<std::size_t, mesh_id> mesh_ids;     //!< Map from index in assimp's mesh array to resource database mesh_id
    resource_id added_root;                      //!< Id of the root resource created in the last call to load_resources()

    //---------------------------------------------------------------------------------------------
    // Helper functions
    //---------------------------------------------------------------------------------------------
    void log_callback(const char* message, char*)
    {
      log(LOG_LEVEL_DEBUG, message);
    }

    std::string extract_dir(const std::string& file_name)
    {
      std::string dir = CURRENT_DIR;
      std::size_t pos = file_name.rfind(PATH_SEPARATOR);
      if (pos != std::string::npos) {
        dir.append(file_name.substr(0, pos));
      }

      return dir;
    }

    void create_materials(const struct aiScene* scene, const mat_id** materials_out, std::size_t* num_materials_out, const std::string& file_name)
    {
      for (std::size_t i = 0; i < scene->mNumMaterials; i++) {
        aiColor4D diffuse_color(0.0f, 0.0f, 0.0f, 0.0f);
        aiGetMaterialColor(scene->mMaterials[i], AI_MATKEY_COLOR_DIFFUSE, &diffuse_color);
        float color_diffuse[3];
        color_diffuse[0] = diffuse_color[0];
        color_diffuse[1] = diffuse_color[1];
        color_diffuse[2] = diffuse_color[2];

        aiColor4D specular_color(0.0f, 0.0f, 0.0f, 0.0f);
        aiGetMaterialColor(scene->mMaterials[i], AI_MATKEY_COLOR_SPECULAR, &specular_color);
        float color_specular[3];
        color_specular[0] = specular_color[0];
        color_specular[1] = specular_color[1];
        color_specular[2] = specular_color[2];      

        // In all the models I've tested that have textures, the texture paths are in texture type aiTextureType_DIFFUSE and n = 0
        aiString relative_texture_path;
        aiGetMaterialString(scene->mMaterials[i], AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), &relative_texture_path);
        std::string texture_path;
        if (strlen(relative_texture_path.C_Str())) {
          texture_path = extract_dir(file_name) + std::string(PATH_SEPARATOR) + std::string(relative_texture_path.C_Str());
        }

        float smoothness = 1.0f;
        aiGetMaterialFloat(scene->mMaterials[i], AI_MATKEY_SHININESS, &smoothness);

        mat_id mat = add_material();
        set_material_properties(mat, color_diffuse, color_specular, smoothness, texture_path.size()? texture_path.c_str() : nullptr);
        added_materials.push_back(mat);
        material_ids[i] = mat;
      }
      *materials_out = &added_materials[0];
      *num_materials_out = added_materials.size();
    }

    void create_meshes(const struct aiScene* scene, const mesh_id** meshes_out, std::size_t* num_meshes_out)
    {

      for (std::size_t i_mesh = 0; i_mesh < scene->mNumMeshes; i_mesh++) {
        aiMesh* mesh = scene->mMeshes[i_mesh];
        if (material_ids.find(mesh->mMaterialIndex) == material_ids.end()) continue;
        mat_id material = material_ids[mesh->mMaterialIndex];

        std::vector<float> vertex_base;
        if (mesh->HasPositions()) {
          for (std::size_t i_vertices = 0; i_vertices < mesh->mNumVertices; i_vertices++) {
            aiVector3D vertex = mesh->mVertices[i_vertices];
            vertex_base.push_back(vertex.x);
            vertex_base.push_back(vertex.y);
            vertex_base.push_back(vertex.z);
          }
        }

        std::size_t texture_coords_stride = mesh->mNumUVComponents[0];
        std::vector<float> texture_coords;
        if (mesh->HasTextureCoords(0)) {
          for (std::size_t i_vertices = 0; i_vertices < mesh->mNumVertices; i_vertices++) {
            aiVector3D tex_coords = mesh->mTextureCoords[0][i_vertices];
            texture_coords.push_back(tex_coords.x); // U coordinate
            texture_coords.push_back(tex_coords.y); // V coordinate
            if (texture_coords_stride > 2) {
              texture_coords.push_back(tex_coords.z); // W coordinate
            }
          }
        }

        std::vector<vindex> faces;
        std::size_t faces_stride = 0U;
        std::size_t num_faces = 0U;
        if (mesh->HasFaces()) {
          faces_stride = mesh->mFaces[0].mNumIndices;
          num_faces = mesh->mNumFaces;
          for (std::size_t i_faces = 0; i_faces < num_faces; i_faces++) {
            aiFace face = mesh->mFaces[i_faces];
            for (std::size_t i_indices = 0; i_indices < face.mNumIndices; i_indices++) {
              faces.push_back(face.mIndices[i_indices]);
            }
          }
        }

        std::vector<float> normals;
        if (mesh->HasNormals()) {
          for (std::size_t i_normals = 0; i_normals < mesh->mNumVertices; i_normals++) {
            aiVector3D normal = mesh->mNormals[i_normals];
            normals.push_back(normal.x);
            normals.push_back(normal.y);
            normals.push_back(normal.z);
          }
        }

        mesh_id m = add_mesh();
        set_mesh_properties(m,
                            &vertex_base[0],
                            3U,
                            &texture_coords[0],
                            texture_coords_stride,
                            mesh->mNumVertices,
                            &faces[0],
                            faces_stride,
                            num_faces,
                            &normals[0],
                            material);
        added_meshes.push_back(m);
        mesh_ids[i_mesh] = m;
      }

      *meshes_out = &added_meshes[0];
      *num_meshes_out = added_meshes.size();
    }

    resource_id create_resources(const struct aiScene* scene)
    {
      // Iterate the scene tree with a breadth-first search creating resources
      struct context{ const aiNode* node; resource_id parent; };
      std::queue<context> pending_nodes;
      pending_nodes.push({ scene->mRootNode, root_resource });
      while (!pending_nodes.empty()) {
        auto current = pending_nodes.front();
        pending_nodes.pop();
        
        std::vector<mesh_id> meshes;
        for (std::size_t i = 0; i < current.node->mNumMeshes; i++) {
          if (mesh_ids.find(current.node->mMeshes[i]) != mesh_ids.end()) {
            meshes.push_back(mesh_ids[current.node->mMeshes[i]]);
          }
        }

        // The aiMatrix4x4 type uses a contiguous layout for its elements, so we can just use its
        // representation as if it were a float array. But we need to transpose it first, because
        // assimp uses a row-major layout and we need column-major.
        aiMatrix4x4 local_transform = current.node->mTransformation;
        aiTransposeMatrix4(&local_transform);

        resource_id res = add_resource(current.parent);
        set_resource_properties(res, meshes.size()? &meshes[0] : nullptr, meshes.size(), (float*) &local_transform);
        if (added_root == nresource) {
          added_root = res;
        }

        for (std::size_t i = 0; i < current.node->mNumChildren; ++i) {
          pending_nodes.push({ current.node->mChildren[i], res });
        }
      }

      return added_root;
    }

    template<typename T>
    void print_sequence(T* a, std::size_t num_elems, std::ostringstream& oss)
    {
      oss << "[";
      if (num_elems) {
        oss << " " << a[0];
      }
      for (std::size_t i = 1U; i < num_elems; i++) {
        oss << ", " << a[i];
      }
      oss << " ]";
    }

    template<typename T>
    void preview_sequence(T* a, std::size_t num_elems, std::ostringstream& oss)
    {
      oss << "[";
      if (num_elems > 0) {
        oss << " " << a[0];
      }
      if (num_elems > 1) {
        oss << ", " << a[1];
      }
      if (num_elems > 2) {
        oss << ", " << a[2];
      }
      oss << ", ...//... , ";
      if (num_elems - 3 >= 0) {
        oss << ", " << a[num_elems - 3];
      }
      if (num_elems - 2 >= 0) {
        oss << ", " << a[num_elems - 2];
      }
      if (num_elems - 1 >= 0) {
        oss << ", " << a[num_elems - 1];
      }
      oss << " ]";
    }

    void print_material(mat_id m)
    {
      float color_diffuse[3];
      float color_specular[3];
      float smoothness = 0.0f;
      const char* texture_path = nullptr;
      get_material_properties(m, color_diffuse, color_specular, smoothness, &texture_path);
      std::ostringstream oss;
      oss << std::setprecision(2) << std::fixed;
      oss << "\tid: " << m << ", color diffuse: ";
      print_sequence(color_diffuse, 3U, oss);
      oss << ", color specular: ";
      print_sequence(color_specular, 3U, oss);
      oss << ", smoothness: " << smoothness;
      oss << ", texture path: " << (std::strlen(texture_path)? texture_path : "");
      log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }


    void print_mesh(mesh_id m)
    {
      const float* vertex_base_out;
      std::size_t vertex_stride_out;
      const float* texture_coords_out;
      std::size_t texture_coords_stride_out;
      std::size_t num_vertices_out;
      const vindex* faces_out;
      std::size_t faces_stride_out;
      const float* normals_out;
      std::size_t num_faces_out;
      mat_id material_out;
      get_mesh_properties(m,
                          &vertex_base_out,
                          &vertex_stride_out,
                          &texture_coords_out,
                          &texture_coords_stride_out,
                          &num_vertices_out,
                          &faces_out,
                          &faces_stride_out,
                          &num_faces_out,
                          &normals_out,
                          &material_out);
      std::ostringstream oss;
      oss << std::setprecision(2) << std::fixed;
      oss << "\t" << "id: " << m << ", vertices: " << num_vertices_out
                            << ", vertex stride: " << vertex_stride_out
                            << ", faces: " << num_faces_out
                            << ", faces stride: " << faces_stride_out;
      log(LOG_LEVEL_DEBUG, oss.str().c_str());

      oss.str("");
      oss << "\t\tvertex base: ";
      if (num_vertices_out) {
        preview_sequence(vertex_base_out, vertex_stride_out * num_vertices_out, oss);
      }
      log(LOG_LEVEL_DEBUG, oss.str().c_str());

      oss.str("");
      oss << "\t\ttexture coords: ";
      if (texture_coords_out) {
        preview_sequence(texture_coords_out, texture_coords_stride_out * num_vertices_out, oss);
      }
      log(LOG_LEVEL_DEBUG, oss.str().c_str());

      oss.str("");
      oss << "\t\ttexture coords stride: " << texture_coords_stride_out;
      log(LOG_LEVEL_DEBUG, oss.str().c_str());

      oss.str("");
      oss << "\t\tfaces: ";
      preview_sequence(faces_out, faces_stride_out * num_faces_out, oss);
      log(LOG_LEVEL_DEBUG, oss.str().c_str());

      oss.str("");
      oss << "\t\tnormals: ";
      if (normals_out) {
        preview_sequence(normals_out, 3 * num_vertices_out, oss);
      }
      log(LOG_LEVEL_DEBUG, oss.str().c_str());

      oss.str("");
      oss << "\t\tmaterial id: " << material_out;
      log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }

    void print_resource_tree(resource_id root)
    {
      // Iterate the resource tree with a breadth-first search printing resources
      struct context{ resource_id rid; unsigned int indentation; };
      std::queue<context> pending_nodes;
      pending_nodes.push({root, 1U});
      while (!pending_nodes.empty()) {
        auto current = pending_nodes.front();
        pending_nodes.pop();

        const float* local_transform_out = nullptr;
        std::size_t num_meshes_out = 0U;
        const mesh_id* meshes_out = nullptr;
        get_resource_properties(current.rid, &meshes_out, &num_meshes_out, &local_transform_out);

        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        for (unsigned int i = 0; i < current.indentation; i++) {
          oss << "\t";
        }
        oss << "[ ";
        oss << "resource id: " << current.rid;
        oss << ", meshes: ";
        print_sequence(meshes_out, num_meshes_out, oss);
        oss << ", local transform: ";
        print_sequence(local_transform_out, 16, oss);
        oss << " ]";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        for (resource_id child = get_first_child_resource(current.rid); child != nresource; child = get_next_sibling_resource(child)) {
          pending_nodes.push({child, current.indentation + 1});
        }
      }
    }

    void log_statistics()
    {
      log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
      log(LOG_LEVEL_DEBUG, "load_resources: finished loading file, summary:");
      log(LOG_LEVEL_DEBUG, "materials:");
      if (added_materials.size()) {
        for (auto m : added_materials) {
          print_material(m);
        }
      } else {
        log(LOG_LEVEL_DEBUG, "\tno materials found");
      }

      log(LOG_LEVEL_DEBUG, "meshes:");
      if (added_meshes.size()) {
        for (auto m : added_meshes) {
          print_mesh(m);
        }
      } else {
        log(LOG_LEVEL_DEBUG, "\tno meshes found");
      }

      log(LOG_LEVEL_DEBUG, "resources:");
      if (added_root != nresource) {
        print_resource_tree(added_root);
      } else {
        log(LOG_LEVEL_DEBUG, "\tno resources found");
      }      

      log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
    }
  } // anonymous namespace

  //-----------------------------------------------------------------------------------------------
  // Public functions
  //-----------------------------------------------------------------------------------------------
  resource_id load_resources(const std::string& file_name,
                             const mat_id** materials_out,
                             std::size_t* num_materials_out,
                             const mesh_id** meshes_out,
                             std::size_t* num_meshes_out)
  {
    if (!(file_name.size() > 0 && materials_out && num_materials_out && meshes_out && num_meshes_out)) {
      log(LOG_LEVEL_ERROR, "load_resources error: invalid arguments"); return nresource;
    }

    struct aiLogStream log_stream;
    log_stream.callback = log_callback;
    log_stream.user = nullptr;
    aiAttachLogStream(&log_stream);
    const struct aiScene* scene = aiImportFile(file_name.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
    if (!scene) {
      log(LOG_LEVEL_ERROR, "load_resources: some errors occured, aborting"); return nresource;
    }

    added_materials.clear();
    material_ids.clear();
    create_materials(scene, materials_out, num_materials_out, file_name);

    added_meshes.clear();
    mesh_ids.clear();
    create_meshes(scene, meshes_out, num_meshes_out);

    added_root = nresource;
    resource_id root = create_resources(scene);

    log_statistics();

    aiReleaseImport(scene);
    aiDetachAllLogStreams();

    return root;
  }
} // namespace cgs
