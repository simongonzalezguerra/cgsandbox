#include "database_loader.hpp"
#include "resource_loader.hpp"
#include "nlohmann/json.hpp"
#include "cmd_line_args.hpp"
#include "glm/glm.hpp"
#include "log.hpp"

#include <fstream>
#include <cassert>
#include <string>

using json = nlohmann::json;

namespace rte
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        bool                    initialized = false;
        json                    document;
        material_vector         materials;
        mesh_vector             meshes;
        resource_vector         resources;
        cubemap_vector          cubemaps;
        node_vector             nodes;
        point_light_vector      point_lights;
        scene_vector            scenes;

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        glm::vec3 array_to_vec3(const json& array) {
            return glm::vec3(array[0].get<float>(), array[1].get<float>(), array[2].get<float>());
        }

        void load_materials()
        {
            material_vector added_materials;
            for (auto& m : document["materials"]) {
                auto mat = make_material();
                set_material_diffuse_color(mat.get(), array_to_vec3(m["diffuse_color"]));
                set_material_specular_color(mat.get(), array_to_vec3(m["specular_color"]));
                set_material_smoothness(mat.get(), m.value("smoothness", 1.0f));
                set_material_texture_path(mat.get(), m.value("texture_path", std::string("")));
                set_material_reflectivity(mat.get(), m.value("reflectivity", 0.0f));
                set_material_translucency(mat.get(), m.value("translucency", 0.0f));
                set_material_refractive_index(mat.get(), m.value("refractive_index", 1.0f));
                set_material_user_id(mat.get(), m.value("user_id", nuser_id));
                set_material_name(mat.get(), m.value("name", std::string("")));
                added_materials.push_back(std::move(mat));
            }

            materials.insert(materials.end(), make_move_iterator(added_materials.begin()), make_move_iterator(added_materials.end()));
        }

        void fill_index_vector(const json& mesh_document, std::vector<vindex>* out, const std::string& field_name)
        {
            for (auto& index : mesh_document[field_name]) {
                out->push_back(index);
            }
        }

        void fill_2d_vector(const json& mesh_document, std::vector<glm::vec2>* out, const std::string& field_name)
        {
            unsigned int n_elements = 0;
            auto& texture_coords_document = mesh_document[field_name];
            assert(texture_coords_document.size() % 2 == 0);
            auto doc_it = texture_coords_document.begin();
            float u, v;
            u = v = 0.0f;
            while (doc_it != texture_coords_document.end()) {
                unsigned int mod = n_elements++ % 2;
                if (mod == 0) {
                    u = *doc_it++;
                } else {
                    v = *doc_it++;
                    out->push_back(glm::vec2(u, v));
                }
            }
        }

        void fill_3d_vector(const json& mesh_document, std::vector<glm::vec3>* out, const std::string& field_name)
        {
            unsigned int n_elements = 0;
            auto& vertices_document = mesh_document[field_name];
            assert(vertices_document.size() % 3 == 0);
            auto doc_it = vertices_document.begin();

            float x, y, z;
            x = y = z = 0.0f;
            while (doc_it != vertices_document.end()) {
                unsigned int mod = n_elements++ % 3;
                if (mod == 0) {
                    x = *doc_it++;
                } else if (mod == 1) {
                    y = *doc_it++;
                } else {
                    z = *doc_it++;
                    out->push_back(glm::vec3(x, y, z));
                }
            }
        }


        void load_meshes()
        {
            mesh_vector added_meshes;
            for (auto& m : document["meshes"]) {
                auto mesh = make_mesh();
                set_mesh_user_id(mesh.get(), m.value("user_id", nuser_id));
                std::vector<glm::vec3> vertices;
                fill_3d_vector(m, &vertices, "vertices");
                set_mesh_vertices(mesh.get(), vertices);
                std::vector<glm::vec2> texture_coords;
                fill_2d_vector(m, &texture_coords, "texture_coords");
                set_mesh_texture_coords(mesh.get(), texture_coords);
                std::vector<glm::vec3> normals;
                fill_3d_vector(m, &vertices, "normals");
                set_mesh_normals(mesh.get(), normals);
                std::vector<vindex> indices;
                fill_index_vector(m, &indices, "indices");
                set_mesh_indices(mesh.get(), indices);
                added_meshes.push_back(std::move(mesh));
            }

            meshes.insert(meshes.end(), make_move_iterator(added_meshes.begin()), make_move_iterator(added_meshes.end()));
        }

        void load_resources()
        {
            // TODO
        }

        void load_cubemaps()
        {
            // TODO
        }

        void load_nodes()
        {
            // TODO
        }

        void load_point_lights()
        {
            // TODO
        }

        void load_scenes()
        {
            // TODO
        }
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void database_loader_initialize()
    {
        if (!initialized) {
            log(LOG_LEVEL_DEBUG, "database_loader: initializing database loader");
            document = json();
            materials.clear();
            meshes.clear();
            resources.clear();
            cubemaps.clear();
            nodes.clear();
            point_lights.clear();
            scenes.clear();
            initialized = true;
            log(LOG_LEVEL_DEBUG, "database_loader: database loader initialized");
        }
    }

    void load_database()
    {
        if (!initialized) return;

        std::string filename = cmd_line_args_get_option_value("-config", "config.json");

        log(LOG_LEVEL_DEBUG, std::string("database_loader: loading database from file ") + filename);
        std::ifstream ifs(filename);
        document = json();
        ifs >> document;

        load_materials();
        log_materials();
        load_meshes();
        log_meshes();
        load_resources();
        load_cubemaps();
        load_nodes();
        load_point_lights();
        load_scenes();

        document = json();
        log(LOG_LEVEL_DEBUG, "database_loader: database loaded successfully");
    }

    void log_database()
    {
        // TODO
    }

    void database_loader_finalize()
    {
        if (initialized) {
            log(LOG_LEVEL_DEBUG, "database_loader: finalizing database loader");
            materials.clear();
            meshes.clear();
            resources.clear();
            cubemaps.clear();
            nodes.clear();
            point_lights.clear();
            scenes.clear();
            initialized = false;
            log(LOG_LEVEL_DEBUG, "database_loader: initialized finalized");
        }
    }
} // namespace rte
