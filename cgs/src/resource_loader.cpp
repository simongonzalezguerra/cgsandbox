#include "cgs/resource_loader.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "cgs/log.hpp"
#include "glm/glm.hpp"

#include <stdexcept>
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
        std::map<std::size_t, mat_id>  material_ids;  //!< Map from index in assimp's material array to resource database material_id
        std::map<std::size_t, mesh_id> mesh_ids;      //!< Map from index in assimp's mesh array to resource database mesh_id

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

        void create_materials(const struct aiScene* scene, std::vector<mat_id>* materials_out, const std::string& file_name)
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
                    texture_path = extract_dir(file_name) + std::string(PATH_SEPARATOR) + std::string(relative_texture_path.C_Str());
                }

                float smoothness = 1.0f;
                aiGetMaterialFloat(scene->mMaterials[i], AI_MATKEY_SHININESS, &smoothness);

                mat_id mat = add_material();
                set_material_diffuse_color(mat, color_diffuse);
                set_material_specular_color(mat, color_specular);
                set_material_smoothness(mat, smoothness);
                set_material_texture_path(mat, texture_path);
                materials_out->push_back(mat);
                material_ids[i] = mat;
            }
        }

        void create_meshes(const struct aiScene* scene, std::vector<mesh_id>* meshes_out)
        {
            for (std::size_t i_mesh = 0; i_mesh < scene->mNumMeshes; i_mesh++) {
                aiMesh* mesh = scene->mMeshes[i_mesh];

                if (material_ids.find(mesh->mMaterialIndex) == material_ids.end()) continue;
                mesh_id m = add_mesh();

                std::vector<glm::vec3> vertices;
                if (mesh->HasPositions()) {
                    for (std::size_t i_vertices = 0; i_vertices < mesh->mNumVertices; i_vertices++) {
                        aiVector3D vertex = mesh->mVertices[i_vertices];
                        vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));
                    }
                }
                set_mesh_vertices(m, vertices);

                std::vector<glm::vec2> texture_coords;
                if (mesh->HasTextureCoords(0)) {
                    for (std::size_t i_vertices = 0; i_vertices < mesh->mNumVertices; i_vertices++) {
                        aiVector3D tex_coords = mesh->mTextureCoords[0][i_vertices];
                        texture_coords.push_back(glm::vec2(tex_coords.x, tex_coords.y));
                    }
                }
                set_mesh_texture_coords(m, texture_coords);

                std::vector<glm::vec3> normals;
                if (mesh->HasNormals()) {
                    for (std::size_t i_normals = 0; i_normals < mesh->mNumVertices; i_normals++) {
                        aiVector3D normal = mesh->mNormals[i_normals];
                        normals.push_back(glm::vec3(normal.x, normal.y, normal.z));
                    }
                }
                set_mesh_normals(m, normals);

                std::vector<vindex> indices;
                if (mesh->HasFaces()) {
                    for (std::size_t i_faces = 0; i_faces < mesh->mNumFaces; i_faces++) {
                        aiFace face = mesh->mFaces[i_faces];
                        for (std::size_t i_indices = 0; i_indices < face.mNumIndices; i_indices++) {
                            indices.push_back(face.mIndices[i_indices]);
                        }
                    }
                }
                set_mesh_indices(m, indices);

                meshes_out->push_back(m);
                mesh_ids[i_mesh] = m;
            }
        }

        // Assimp creates a structure with several meshes by node, and each mesh has a material. In
        // practice though most models have one mesh by node. Our model has one mesh by resource
        // node, and the material is assigned to the resource not the mesh. We convert Assimp's
        // structure to our own by translating a node with several meshes into several resource
        // nodes, all with the same transform, each with one mesh. The children of the node, if any,
        // are all added recursively as children if the first resource created. Function
        // map_meshes_to_resources a node with many meshes into a list of resource nodes
        std::vector<resource_id> map_meshes_to_resources(const struct aiScene* scene, const aiNode* node, resource_id parent)
        {
            std::vector<resource_id> new_resources{add_resource(parent)};
            if (node->mNumMeshes > 0 && mesh_ids.find(node->mMeshes[0]) != mesh_ids.end()) {
                set_resource_mesh(new_resources[0], mesh_ids[node->mMeshes[0]]);
                set_resource_material(new_resources[0], material_ids[scene->mMeshes[node->mMeshes[0]]->mMaterialIndex]);
            }
            for (std::size_t i = 1U; i < node->mNumMeshes; i++) {
                if (mesh_ids.find(node->mMeshes[i]) != mesh_ids.end()) {
                    new_resources.push_back(add_resource(parent));
                    set_resource_mesh(new_resources[new_resources.size() - 1], mesh_ids[node->mMeshes[i]]);
                    set_resource_material(new_resources[new_resources.size() - 1], material_ids[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex]);
                }
            }
            
            return new_resources;
        }

        void create_resources(const struct aiScene* scene, resource_id* added_root)
        {
            // Iterate the scene tree with a breadth-first search creating resources
            *added_root = nresource;
            struct context{ const aiNode* node; resource_id parent; };
            std::queue<context> pending_nodes;
            pending_nodes.push({ scene->mRootNode, root_resource });
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.front();
                pending_nodes.pop();

                // Convert the list of meshes into a list of resource nodes
                std::vector<resource_id> new_resources = map_meshes_to_resources(scene, current.node, current.parent);

                // The aiMatrix4x4 type uses a contiguous layout for its elements, so we can just use its
                // representation as if it were a float array. But we need to transpose it first, because
                // assimp uses a row-major layout and we need column-major.
                aiMatrix4x4 local_transform = current.node->mTransformation;
                aiTransposeMatrix4(&local_transform);
                for (auto r : new_resources) {
                    // All the resource nodes created for the same assimp node have the same local transform
                    set_resource_local_transform(r, glm::make_mat4((float *) &local_transform));
                }

                if (*added_root == nresource) {
                    *added_root = new_resources[0];
                }

                for (std::size_t i = 0; i < current.node->mNumChildren; ++i) {
                    // Recursion adds children to the first resource node of the list
                    pending_nodes.push({ current.node->mChildren[i], new_resources[0] });
                }
            }
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
            std::string texture_path = get_material_texture_path(m);
            glm::vec3 diffuse_color = get_material_diffuse_color(m);
            glm::vec3 specular_color = get_material_specular_color(m);
            float smoothness = get_material_smoothness(m);
            std::ostringstream oss;
            oss << std::setprecision(2) << std::fixed;
            oss << "    id: " << m << ", color diffuse: ";
            print_sequence((float*) &diffuse_color, 3U, oss);
            oss << ", color specular: ";
            print_sequence((float*) &specular_color, 3U, oss);
            oss << ", smoothness: " << smoothness;
            oss << ", texture path: " << texture_path;
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
        }


        void print_mesh(mesh_id m)
        {
            std::vector<glm::vec3> vertices = get_mesh_vertices(m);
            std::vector<glm::vec2> texture_coords = get_mesh_texture_coords(m);
            std::vector<glm::vec3> normals = get_mesh_normals(m);
            std::vector<vindex> indices = get_mesh_indices(m);

            std::ostringstream oss;
            oss << std::setprecision(2) << std::fixed;
            oss << "    " << "id: " << m << ", vertices: " << vertices.size();
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            oss.str("");
            oss << "        vertex base: ";
            if (vertices.size()) {
                preview_sequence(&vertices[0][0], 3 * vertices.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            oss.str("");
            oss << "        texture coords: ";
            if (texture_coords.size()) {
                preview_sequence(&texture_coords[0][0], 2 * texture_coords.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            oss.str("");
            oss << "        indices: ";
            preview_sequence(&indices[0], indices.size(), oss);
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            oss.str("");
            oss << "        normals: ";
            if (normals.size()) {
                preview_sequence(&normals[0][0], 3 * normals.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
        }

        std::string format_mesh_id(mesh_id m)
        {
            std::ostringstream oss;
            if (m != nmesh) {
                oss << m;
            } else {
                oss << "nmesh";
            }

            return oss.str();
        }

        std::string format_material_id(mat_id m)
        {
            std::ostringstream oss;
            if (m != nmat) {
                oss << m;
            } else {
                oss << "nmat";
            }

            return oss.str();
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

                glm::mat4 local_transform = get_resource_local_transform(current.rid);

                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed;
                for (unsigned int i = 0; i < current.indentation; i++) {
                    oss << "    ";
                }
                oss << "[ ";
                oss << "resource id: " << current.rid;
                oss << ", mesh: " << format_mesh_id(get_resource_mesh(current.rid));
                oss << ", material: " << format_material_id(get_resource_material(current.rid));
                oss << ", local transform: ";
                print_sequence(glm::value_ptr(local_transform), 16, oss);
                oss << " ]";
                log(LOG_LEVEL_DEBUG, oss.str().c_str());

                for (resource_id child = get_first_child_resource(current.rid); child != nresource; child = get_next_sibling_resource(child)) {
                    pending_nodes.push({child, current.indentation + 1});
                }
            }
        }

        void log_statistics(resource_id added_root, const std::vector<mat_id>& added_materials, const std::vector<mesh_id>& added_meshes)
        {
            log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
            log(LOG_LEVEL_DEBUG, "load_resources: finished loading file, summary:");
            log(LOG_LEVEL_DEBUG, "materials:");
            if (added_materials.size()) {
                for (auto m : added_materials) {
                    print_material(m);
                }
            } else {
                log(LOG_LEVEL_DEBUG, "    no materials found");
            }

            log(LOG_LEVEL_DEBUG, "meshes:");
            if (added_meshes.size()) {
                for (auto m : added_meshes) {
                    print_mesh(m);
                }
            } else {
                log(LOG_LEVEL_DEBUG, "    no meshes found");
            }

            log(LOG_LEVEL_DEBUG, "resources:");
            if (added_root != nresource) {
                print_resource_tree(added_root);
            } else {
                log(LOG_LEVEL_DEBUG, "    no resources found");
            }      

            log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        }
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    resource_id load_resources(const std::string& file_name,
            std::vector<mat_id>* materials_out,
            std::vector<mesh_id>* meshes_out)
    {
        if (!(file_name.size() > 0 && materials_out && meshes_out)) {
            log(LOG_LEVEL_ERROR, "load_resources error: invalid arguments");
            throw std::runtime_error("");
        }

        struct aiLogStream log_stream;
        log_stream.callback = log_callback;
        log_stream.user = nullptr;
        aiAttachLogStream(&log_stream);
        const struct aiScene* scene = aiImportFile(file_name.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
        if (!scene) {
            log(LOG_LEVEL_ERROR, "load_resources: some errors occured, aborting");
            throw std::runtime_error("");
        }

        material_ids.clear();
        mesh_ids.clear();
        materials_out->clear();
        meshes_out->clear();
        resource_id added_root = nresource;
        create_materials(scene, materials_out, file_name);
        create_meshes(scene, meshes_out);
        create_resources(scene, &added_root);

        log_statistics(added_root, *materials_out, *meshes_out);

        aiReleaseImport(scene);
        aiDetachAllLogStreams();

        return added_root;
    }
} // namespace cgs
