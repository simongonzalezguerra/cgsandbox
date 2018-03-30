#include "glm/gtc/type_ptr.hpp"
#include "assimp/postprocess.h"
#include "resource_loader.hpp"
#include "assimp/cimport.h"
#include "sparse_list.hpp"
#include "assimp/scene.h"
#include "math_utils.hpp"
#include "glm/glm.hpp"
#include "system.hpp"
#include "log.hpp"

#include <stdexcept>
#include <iomanip>
#include <cstring>
#include <sstream>
#include <vector>
#include <map>

namespace rte
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        std::map<std::size_t, mat_id>  material_indices;  //!< Map from index in assimp's material array to material index in the database
        std::map<std::size_t, mesh_id> mesh_indices;      //!< Map from index in assimp's mesh array to mesh index in the database

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        void log_callback(const char* message, char*)
        {
            std::string message_trimmed(message);
            std::size_t n = message_trimmed.find('\n');
            if (n != std::string::npos) {
              message_trimmed[n] = '\0';
            }
            log(LOG_LEVEL_DEBUG, std::string("assimp: ") + message_trimmed);
        }

        void create_materials(const struct aiScene* scene, view_database& db, const std::string& file_name)
        {
            for (std::size_t i = 0; i < scene->mNumMaterials; i++) {
                aiColor4D diffuse_color(0.0f, 0.0f, 0.0f, 0.0f);
                aiGetMaterialColor(scene->mMaterials[i], AI_MATKEY_COLOR_DIFFUSE, &diffuse_color);
                glm::vec3 color_diffuse;
                color_diffuse[0] = diffuse_color[0];
                color_diffuse[1] = diffuse_color[1];
                color_diffuse[2] = diffuse_color[2];

                aiColor4D specular_color(0.0f, 0.0f, 0.0f, 0.0f);
                aiGetMaterialColor(scene->mMaterials[i], AI_MATKEY_COLOR_SPECULAR, &specular_color);
                glm::vec3 color_specular;
                color_specular[0] = specular_color[0];
                color_specular[1] = specular_color[1];
                color_specular[2] = specular_color[2];      

                // In all the models I've tested that have textures, the texture paths are in texture type aiTextureType_DIFFUSE and n = 0
                aiString relative_texture_path;
                aiGetMaterialString(scene->mMaterials[i], AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), &relative_texture_path);
                std::string texture_path;
                if (strlen(relative_texture_path.C_Str())) {
                    texture_path = make_path(adapt_slashes(extract_dir(file_name)), adapt_slashes(relative_texture_path.C_Str()));
                }

                float smoothness = 1.0f;
                aiGetMaterialFloat(scene->mMaterials[i], AI_MATKEY_SHININESS, &smoothness);

                material mat;
                mat.m_diffuse_color = color_diffuse;
                mat.m_specular_color = color_specular;
                mat.m_smoothness = smoothness;
                mat.m_texture_path = texture_path;
                material_indices[i] = list_insert(db.m_materials, 0, mat);
            }
        }

        void create_meshes(const struct aiScene* scene, view_database& db)
        {
            for (std::size_t i_mesh = 0; i_mesh < scene->mNumMeshes; i_mesh++) {
                aiMesh* ai_mesh = scene->mMeshes[i_mesh];

                if (material_indices.find(ai_mesh->mMaterialIndex) == material_indices.end()) continue;
                auto new_mesh_index = list_insert(db.m_meshes, 0, mesh());
                auto& new_mesh = db.m_meshes.at(new_mesh_index);

                mesh_indices[i_mesh] = new_mesh_index;

                if (ai_mesh->HasPositions()) {
                    for (std::size_t i_vertices = 0; i_vertices < ai_mesh->mNumVertices; i_vertices++) {
                        aiVector3D vertex = ai_mesh->mVertices[i_vertices];
                        new_mesh.m_vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));
                    }
                }

                if (ai_mesh->HasTextureCoords(0)) {
                    for (std::size_t i_vertices = 0; i_vertices < ai_mesh->mNumVertices; i_vertices++) {
                        aiVector3D tex_coords = ai_mesh->mTextureCoords[0][i_vertices];
                        new_mesh.m_texture_coords.push_back(glm::vec2(tex_coords.x, tex_coords.y));
                    }
                }

                if (ai_mesh->HasNormals()) {
                    for (std::size_t i_normals = 0; i_normals < ai_mesh->mNumVertices; i_normals++) {
                        aiVector3D normal = ai_mesh->mNormals[i_normals];
                        new_mesh.m_normals.push_back(glm::vec3(normal.x, normal.y, normal.z));
                    }
                }

                if (ai_mesh->HasFaces()) {
                    for (std::size_t i_faces = 0; i_faces < ai_mesh->mNumFaces; i_faces++) {
                        aiFace face = ai_mesh->mFaces[i_faces];
                        for (std::size_t i_indices = 0; i_indices < face.mNumIndices; i_indices++) {
                            new_mesh.m_indices.push_back(face.mIndices[i_indices]);
                        }
                    }
                }
            }
        }

        void create_resources(const struct aiScene* scene, index_type& root_out, view_database& db)
        {
            resource_database new_resource_db;
            index_type new_resource_index = tree_insert(new_resource_db, resource());
            struct context{ index_type added_resource_index; aiNode* ai_node; };
            std::vector<context> pending_nodes;
            pending_nodes.push_back({new_resource_index, scene->mRootNode});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.back();
                pending_nodes.pop_back();

                auto& added_resource = new_resource_db.at(current.added_resource_index);

                // Fill resource mesh and material
                if (current.ai_node->mNumMeshes > 0 && mesh_indices.find(current.ai_node->mMeshes[0]) != mesh_indices.end()) {
                    added_resource.m_mesh = mesh_indices[current.ai_node->mMeshes[0]]; 
                    unsigned int material_index = scene->mMeshes[current.ai_node->mMeshes[0]]->mMaterialIndex;
                    if (material_indices.find(material_index) != material_indices.end()) {
                        added_resource.m_material = material_indices.at(material_index);
                    }
                }

                // Fill the resource local transform. The aiMatrix4x4 type uses a contiguous layout
                // for its elements, so we can just use its representation as if it were a float
                // array. But we need to transpose it first, because assimp uses a row-major layout
                // and we need column-major.
                aiMatrix4x4 local_transform = current.ai_node->mTransformation;
                aiTransposeMatrix4(&local_transform);
                added_resource.m_local_transform = glm::make_mat4((float *) &local_transform);

                // Assimp creates a structure with several meshes by node, and each mesh has a
                // material. In practice though most models have one mesh by node. Our model has one
                // mesh by resource node, and the material is assigned to the resource not the mesh.
                // We convert Assimp's structure to our own by translating a node with several
                // meshes into several resource nodes. f the node has more than one mesh, we map
                // each mesh to a new node, hanging them as descendants of current.added_resource_index as a vertical branch,
                // not siblings. All these node have identity as their transform, so that they will
                // in effect use the same transform as current.added_resource_index.
                index_type last_parent_index = current.added_resource_index;
                unsigned int ai_mesh = 1;
                while (ai_mesh < current.ai_node->mNumMeshes) {
                    resource res;
                    last_parent_index = tree_insert(new_resource_db, res, last_parent_index);
                    auto& last_parent = new_resource_db.at(last_parent_index);
                    last_parent.m_mesh = mesh_indices.at(current.ai_node->mMeshes[ai_mesh]);
                    last_parent.m_material = material_indices.at(scene->mMeshes[current.ai_node->mMeshes[ai_mesh]]->mMaterialIndex);
                    last_parent.m_local_transform = glm::mat4(1.0f);
                    ai_mesh++;
                }

                // Push the children to be processed next
                for (unsigned int i = 0; i < current.ai_node->mNumChildren; ++i) {
                    resource res;
                    index_type child_index = tree_insert(new_resource_db, res, last_parent_index);
                    pending_nodes.push_back({child_index, current.ai_node->mChildren[i]});
                }
            }

            root_out = tree_insert(new_resource_db, new_resource_index, db.m_resources, 0);
        }
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void load_resources(const std::string& file_name, index_type& root_out, view_database& db)
    {
        if (!(file_name.size() > 0)) {
            throw std::domain_error("load_resources error: empty file name");
        }

        struct aiLogStream log_stream;
        log_stream.callback = log_callback;
        log_stream.user = nullptr;
        aiAttachLogStream(&log_stream);
        const struct aiScene* scene = aiImportFile(file_name.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
        if (!scene) {
            throw std::runtime_error("load_resources error: invalid arguments");
        }

        material_indices.clear();
        mesh_indices.clear();
        create_materials(scene, db, file_name);
        create_meshes(scene, db);
        create_resources(scene, root_out, db);

        aiReleaseImport(scene);
        aiDetachAllLogStreams();
    }
} // namespace rte
