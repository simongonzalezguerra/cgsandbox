#include "database_loader.hpp"
#include "resource_loader.hpp"
#include "nlohmann/json.hpp"
#include "cmd_line_args.hpp"
#include "glm/glm.hpp"
#include "log.hpp"

#include <fstream>
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
            // TODO add user_id field to everything
        }

        void load_meshes()
        {
            // TODO
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
