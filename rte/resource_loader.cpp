#include "glm/gtc/type_ptr.hpp"
#include "assimp/postprocess.h"
#include "resource_loader.hpp"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "glm/glm.hpp"
#include "system.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <stdexcept>
#include <iomanip>
#include <cstring>
#include <sstream>
#include <queue>
#include <map>

namespace rte
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        std::map<std::size_t, mat_id>  material_ids;  //!< Map from index in assimp's material array to resource database material_id
        std::map<std::size_t, mesh_id> mesh_ids;      //!< Map from index in assimp's mesh array to resource database mesh_id

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

        void create_materials(const struct aiScene* scene, material_vector* materials_out, const std::string& file_name)
        {
            material_vector materials;
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

                auto mat = make_material();
                set_material_diffuse_color(mat.get(), color_diffuse);
                set_material_specular_color(mat.get(), color_specular);
                set_material_smoothness(mat.get(), smoothness);
                set_material_texture_path(mat.get(), texture_path);
                material_ids[i] = mat.get();
                materials.push_back(std::move(mat));
            }

            materials_out->insert(materials_out->end(), make_move_iterator(materials.begin()), make_move_iterator(materials.end()));
        }

        void create_meshes(const struct aiScene* scene, mesh_vector* meshes_out)
        {
            mesh_vector meshes;
            for (std::size_t i_mesh = 0; i_mesh < scene->mNumMeshes; i_mesh++) {
                aiMesh* mesh = scene->mMeshes[i_mesh];

                if (material_ids.find(mesh->mMaterialIndex) == material_ids.end()) continue;
                auto m = make_mesh();

                std::vector<glm::vec3> vertices;
                if (mesh->HasPositions()) {
                    for (std::size_t i_vertices = 0; i_vertices < mesh->mNumVertices; i_vertices++) {
                        aiVector3D vertex = mesh->mVertices[i_vertices];
                        vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));
                    }
                }
                set_mesh_vertices(m.get(), vertices);

                std::vector<glm::vec2> texture_coords;
                if (mesh->HasTextureCoords(0)) {
                    for (std::size_t i_vertices = 0; i_vertices < mesh->mNumVertices; i_vertices++) {
                        aiVector3D tex_coords = mesh->mTextureCoords[0][i_vertices];
                        texture_coords.push_back(glm::vec2(tex_coords.x, tex_coords.y));
                    }
                }
                set_mesh_texture_coords(m.get(), texture_coords);

                std::vector<glm::vec3> normals;
                if (mesh->HasNormals()) {
                    for (std::size_t i_normals = 0; i_normals < mesh->mNumVertices; i_normals++) {
                        aiVector3D normal = mesh->mNormals[i_normals];
                        normals.push_back(glm::vec3(normal.x, normal.y, normal.z));
                    }
                }
                set_mesh_normals(m.get(), normals);

                std::vector<vindex> indices;
                if (mesh->HasFaces()) {
                    for (std::size_t i_faces = 0; i_faces < mesh->mNumFaces; i_faces++) {
                        aiFace face = mesh->mFaces[i_faces];
                        for (std::size_t i_indices = 0; i_indices < face.mNumIndices; i_indices++) {
                            indices.push_back(face.mIndices[i_indices]);
                        }
                    }
                }
                set_mesh_indices(m.get(), indices);

                mesh_ids[i_mesh] = m.get();
                meshes.push_back(std::move(m));
            }
            meshes_out->insert(meshes_out->end(), make_move_iterator(meshes.begin()), make_move_iterator(meshes.end()));
        }


        void create_resources(const struct aiScene* scene, resource_id* root_out, resource_vector* resources_out)
        {
            resource_vector added_resources;
            added_resources.push_back(make_resource());
            *root_out = added_resources[added_resources.size() - 1].get();
            struct context{ resource_id added_resource; aiNode* ai_node; };
            std::queue<context> pending_nodes;
            pending_nodes.push({added_resources[0].get(), scene->mRootNode});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.front();
                pending_nodes.pop();

                // Fill resource mesh and material
                if (current.ai_node->mNumMeshes > 0 && mesh_ids.find(current.ai_node->mMeshes[0]) != mesh_ids.end()) {
                    set_resource_mesh(current.added_resource, mesh_ids[current.ai_node->mMeshes[0]]);                
                    unsigned int material_index = scene->mMeshes[current.ai_node->mMeshes[0]]->mMaterialIndex;
                    if (material_ids.find(material_index) != material_ids.end()) {
                        set_resource_material(current.added_resource, material_ids[material_index]);
                    }
                }

                // Fill the resource local transform. The aiMatrix4x4 type uses a contiguous layout
                // for its elements, so we can just use its representation as if it were a float
                // array. But we need to transpose it first, because assimp uses a row-major layout
                // and we need column-major.
                aiMatrix4x4 local_transform = current.ai_node->mTransformation;
                aiTransposeMatrix4(&local_transform);
                set_resource_local_transform(current.added_resource, glm::make_mat4((float *) &local_transform));

                // Assimp creates a structure with several meshes by node, and each mesh has a
                // material. In practice though most models have one mesh by node. Our model has one
                // mesh by resource node, and the material is assigned to the resource not the mesh.
                // We convert Assimp's structure to our own by translating a node with several
                // meshes into several resource nodes. f the node has more than one mesh, we map
                // each mesh to a new node, hanging them as descendants of current.added_resource as a vertical branch,
                // not siblings. All these node have identity as their transform, so that they will
                // in effect use the same transform as current.added_resource.
                resource_id last_parent = current.added_resource;
                unsigned int ai_mesh = 1;
                while (ai_mesh < current.ai_node->mNumMeshes) {
                    added_resources.push_back(make_resource(last_parent));
                    last_parent = added_resources[added_resources.size() - 1].get();
                    set_resource_mesh(last_parent, mesh_ids[current.ai_node->mMeshes[ai_mesh]]);
                    set_resource_material(last_parent, material_ids[scene->mMeshes[current.ai_node->mMeshes[ai_mesh]]->mMaterialIndex]);
                    set_resource_local_transform(last_parent, glm::mat4(1.0f));
                    ai_mesh++;
                }

                // Push the children to be processed next
                for (unsigned int i = 0; i < current.ai_node->mNumChildren; ++i) {
                    added_resources.push_back(make_resource(last_parent));
                    resource_id child = added_resources[added_resources.size() - 1].get();
                    pending_nodes.push({child, current.ai_node->mChildren[i]});
                }
            }

            resources_out->insert(resources_out->end(), make_move_iterator(added_resources.begin()), make_move_iterator(added_resources.end()));
        }
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void load_resources(const std::string& file_name,
                        resource_id* root_out,
                        resource_vector* resources_out,
                        material_vector* materials_out,
                        mesh_vector* meshes_out)
    {
        if (!(file_name.size() > 0 && materials_out && meshes_out)) {
            throw std::runtime_error("load_resources error: invalid arguments");
        }

        struct aiLogStream log_stream;
        log_stream.callback = log_callback;
        log_stream.user = nullptr;
        aiAttachLogStream(&log_stream);
        const struct aiScene* scene = aiImportFile(file_name.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
        if (!scene) {
            throw std::runtime_error("load_resources error: invalid arguments");
        }

        material_ids.clear();
        mesh_ids.clear();
        material_vector materials;
        create_materials(scene, &materials, file_name);
        mesh_vector meshes;
        create_meshes(scene, &meshes);
        resource_vector added_resources;
        resource_id added_root_out = nresource;
        create_resources(scene, &added_root_out, &added_resources);

        aiReleaseImport(scene);
        aiDetachAllLogStreams();

        materials_out->insert(materials_out->end(), make_move_iterator(materials.begin()), make_move_iterator(materials.end()));
        meshes_out->insert(meshes_out->end(), make_move_iterator(meshes.begin()), make_move_iterator(meshes.end()));
        resources_out->insert(resources_out->end(), make_move_iterator(added_resources.begin()), make_move_iterator(added_resources.end()));
        *root_out = added_root_out;
    }
} // namespace rte
