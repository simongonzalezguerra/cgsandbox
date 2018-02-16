#include "glm/gtx/transform.hpp"
#include "database_loader.hpp"
#include "resource_loader.hpp"
#include "nlohmann/json.hpp"
#include "cmd_line_args.hpp"
#include "glm/glm.hpp"
#include "log.hpp"

#include <fstream>
#include <cassert>
#include <string>
#include <vector>
#include <map>

using json = nlohmann::json;

namespace rte
{
    namespace
    {
        typedef std::map<user_id, material_database::size_type> material_map;
        typedef std::map<user_id, mesh_database::size_type>     mesh_map;
        typedef std::map<user_id, resource_database::size_type> resource_map;
        typedef std::map<user_id, cubemap_database::size_type>  cubemap_map;

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        bool                           initialized = false;
        json                           document;
        material_map                   material_ids;
        mesh_map                       mesh_ids;
        resource_map                   resource_ids;
        cubemap_map                    cubemap_ids;

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        glm::vec3 array_to_vec3(const json& array) {
            return glm::vec3(array.at(0).get<float>(), array.at(1).get<float>(), array.at(2).get<float>());
        }

        void load_materials(material_database& db)
        {
            for (auto& m : document.at("materials")) {
                material_database::size_type new_index = db.insert(material());
                auto& new_material = db.at(new_index);
                user_id material_user_id = m.value("user_id", nuser_id);
                new_material.m_elem.m_diffuse_color = array_to_vec3(m.at("diffuse_color"));
                new_material.m_elem.m_specular_color = array_to_vec3(m.at("specular_color"));
                new_material.m_elem.m_smoothness = m.value("smoothness", 1.0f);
                new_material.m_elem.m_texture_path = m.value("texture_path", std::string(""));
                new_material.m_elem.m_reflectivity = m.value("reflectivity", 0.0f);
                new_material.m_elem.m_translucency = m.value("translucency", 0.0f);
                new_material.m_elem.m_refractive_index = m.value("refractive_index", 1.0f);
                new_material.m_elem.m_name = m.value("name", std::string(""));
                new_material.m_elem.m_user_id = material_user_id;
                if (material_user_id != nuser_id) {
                    material_ids[material_user_id] = new_index;
                }
            }
        }

        void fill_index_vector(const json& mesh_document, std::vector<vindex>& out, const std::string& field_name)
        {
            for (auto& index : mesh_document.at(field_name)) {
                out.push_back(index);
            }
        }

        void fill_2d_vector(const json& mesh_document, std::vector<glm::vec2>& out, const std::string& field_name)
        {
            unsigned int n_elements = 0;
            auto& texture_coords_document = mesh_document.at(field_name);
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
                    out.push_back(glm::vec2(u, v));
                }
            }
        }

        void fill_3d_vector(const json& mesh_document, std::vector<glm::vec3>& out, const std::string& field_name)
        {
            unsigned int n_elements = 0;
            auto& vertices_document = mesh_document.at(field_name);
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
                    out.push_back(glm::vec3(x, y, z));
                }
            }
        }

        void load_meshes(mesh_database& db)
        {
            for (auto& m : document.at("meshes")) {
                mesh_database::size_type new_index = db.insert(mesh());
                auto& new_mesh = db.at(new_index);
                user_id mesh_user_id = m.value("user_id", nuser_id);
                new_mesh.m_elem.m_user_id = mesh_user_id;
                fill_3d_vector(m, new_mesh.m_elem.m_vertices, "vertices");
                fill_2d_vector(m, new_mesh.m_elem.m_texture_coords, "texture_coords");
                fill_3d_vector(m, new_mesh.m_elem.m_normals, "normals");
                fill_index_vector(m, new_mesh.m_elem.m_indices, "indices");
                if (mesh_user_id != nuser_id) {
                    mesh_ids[mesh_user_id] = new_index;
                }
            }
        }

        bool resource_has_child(resource_database::size_type resource_index,
                                unsigned int child_index,
                                resource_database::size_type& child_index_out,
                                const resource_database& db)
        {
            auto& res = db.at(resource_index);
            auto it = res.begin();
            unsigned int n_child = 0U;
            while (it != res.end() && n_child < child_index) {
                ++it;
                ++n_child;
            }

            child_index_out = index(it);
            return (it != res.end());
        }

        void create_resource(const json& resource_document,
                            resource_database::size_type parent_index,
                            resource_database::size_type& new_root_index_out,
                            view_database& db)
        {
            resource_database::size_type new_root_index = resource_database::value_type::npos;
            if (resource_document.count("from_file")) {
                load_resources(resource_document.value("from_file", std::string()), new_root_index, db);
            } else {
                new_root_index = db.m_resources.insert(resource(), parent_index);
                user_id mesh_user_id = resource_document.value("mesh", nuser_id);
                mesh_map::iterator mit;
                if ((mit = mesh_ids.find(mesh_user_id)) != mesh_ids.end()) {
                    db.m_resources.at(new_root_index).m_elem.m_mesh = mit->second;
                }
                // materials are set later in a second traversal
            }
            auto& new_resource = db.m_resources.at(new_root_index);
            new_resource.m_elem.m_name = resource_document.value("name", std::string());
            new_resource.m_elem.m_user_id = resource_document.value("user_id", nuser_id);
            if (resource_document.count("user_id")) {
                resource_ids[resource_document.at("user_id").get<user_id>()] = new_root_index;
            }

            new_root_index_out = new_root_index;
        }

        void create_resource_tree(const json& resource_document,
                                    resource_database::size_type& new_root_index_out,
                                    view_database& db)
        {
            resource_database::size_type new_root_index = resource_database::value_type::npos;
            bool root_set = false;
            struct json_context { const json& doc; resource_database::size_type parent_index; unsigned int local_child_index; };
            std::vector<json_context> pending_nodes;
            pending_nodes.push_back({resource_document, resource_database::value_type::root, 0U});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.back();
                pending_nodes.pop_back();
                resource_database::size_type current_resource = resource_database::value_type::npos;
                if (current.parent_index == resource_database::value_type::npos
                    || !resource_has_child(current.parent_index, current.local_child_index, current_resource, db.m_resources)) {
                    create_resource(current.doc, current.parent_index, current_resource, db);
                    if (!root_set) {
                        new_root_index = current_resource; // only the first resource created is saved into new_root_index
                        root_set = true;
                    }
                }

                // We are using a stack to process depth-first, so in order for the children to be
                // processed in the order in which they appear we must push them in reverse order,
                // otherwise the last child will be processed first
                if (current.doc.count("children")) {
                    unsigned int n_child = 0;
                    auto& children_list = current.doc.at("children");
                    for (auto json_it = children_list.rbegin(); json_it != children_list.rend(); ++json_it) {
                        pending_nodes.push_back(json_context{*json_it, current_resource, n_child++});
                    }                    
                }
            }

            new_root_index_out = new_root_index;
        }

        void set_resource_tree_materials_and_names(const json& resource_document, resource_database::size_type root_index, resource_database& db)
        {
            struct json_context { const json& doc; resource_database::size_type resource_index; };
            std::vector<json_context> pending_nodes;
            pending_nodes.push_back({resource_document, root_index });
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.back();
                pending_nodes.pop_back();

                auto& res = db.at(current.resource_index);
                user_id material_user_id = current.doc.value("material", nuser_id);
                material_map::iterator mit;
                if ((mit = material_ids.find(material_user_id)) != material_ids.end()) {
                    res.m_elem.m_material = mit->second;
                }

                if (current.doc.count("name")) {
                    res.m_elem.m_name = current.doc.at("name").get<std::string>();
                }

                // Note that in this case it's not relevant in what order the children are processed,
                // but we still push them in reverse order for consistency
                if (current.doc.count("children")) {
                    auto resource_it = res.rbegin();
                    auto& children_list = current.doc.at("children");
                    for (auto json_it = children_list.rbegin(); json_it != children_list.rend(); ++json_it) {
                        pending_nodes.push_back({*json_it, index(resource_it)});
                        ++resource_it;
                    }
                }
            }
        }

        void load_resources(view_database& db)
        {
            for (auto& r : document.at("resources")) {
                resource_database::size_type added_root;
                create_resource_tree(r, added_root, db);
                set_resource_tree_materials_and_names(r, added_root, db.m_resources);
            }
        }

        void load_cubemaps(cubemap_database& db)
        {
            for (auto& cubemap_doc : document.at("cubemaps")) {
                auto new_cubemap_index = db.insert(cubemap());
                auto& new_cubemap = db.at(new_cubemap_index);
                for (auto& face_doc : cubemap_doc.at("faces")) {
                    new_cubemap.m_elem.m_faces.push_back(face_doc.get<std::string>());
                }

                if (cubemap_doc.count("user_id")) {
                    cubemap_ids[cubemap_doc.at("user_id").get<user_id>()] = new_cubemap_index;
                }
            }
        }

        bool node_has_child(node_database::size_type node_index,
                            unsigned int child_index,
                            node_database::size_type& child_index_out,
                            const node_database& db)
        {
            auto& res = db.at(node_index);
            auto it = res.begin();
            unsigned int n_child = 0U;
            while (it != res.end() && n_child < child_index) {
                ++it;
                ++n_child;
            }

            child_index_out = index(it);
            return (it != res.end());
        }

        void create_node(const json& node_document,
                        node_database::size_type parent_index,
                        node_database::size_type& new_root_index_out,
                        view_database& db)
        {
            node_database::size_type new_node_index = db.m_nodes.insert(node());
            auto& new_node = db.m_nodes.at(new_node_index);
            user_id resource_user_id = node_document.value("resource", nuser_id);
            resource_database::size_type resource_index = (resource_user_id == nuser_id ? resource_database::value_type::npos : resource_ids.at(resource_user_id));
            // resource_index can be resource_database::value_type::npos, in that case make_node() creates an empty node
            insert_node_tree(resource_index, parent_index, new_node_index, db);
            new_node.m_elem.m_name = node_document.value("name", std::string());
            new_node.m_elem.m_user_id = node_document.value("user_id", nuser_id);

            // The transform inherited from the resource_index is only overwritten if the document includes all required properties
            if (node_document.count("scale")
                    && node_document.count("rotation_angle")
                    && node_document.count("rotation_axis")
                    && node_document.count("translation")) {
                glm::mat4 scale = glm::scale(array_to_vec3(node_document.at("scale")));
                glm::mat4 rotation = glm::rotate(node_document.at("rotation_angle").get<float>(), array_to_vec3(node_document.at("rotation_axis")));;
                glm::mat4 translation = glm::translate(array_to_vec3(node_document.at("translation")));
                new_node.m_elem.m_local_transform = translation * rotation * scale;
            }
            // materials are set later in a second traversal

            new_root_index_out = new_node_index;
        }

        void create_node_tree(const json& node_document,
                            node_database::size_type scene_root_index,
                            node_database::size_type& new_root_index_out,
                            view_database& db)
        {
            bool root_set = false;
            node_database::size_type new_root_index = node_database::value_type::npos;
            struct json_context { const json& doc; node_database::size_type parent; unsigned int index; };
            std::vector<json_context> pending_nodes;
            pending_nodes.push_back({node_document, scene_root_index, 0U});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.back();
                pending_nodes.pop_back();
                node_database::size_type current_node_index = node_database::value_type::npos;
                if (current.parent == scene_root_index || !node_has_child(current.parent, current.index, current_node_index, db.m_nodes)) {
                    create_node(current.doc, current.parent, current_node_index, db);
                    if (!root_set) {
                        new_root_index  = current_node_index; // only the first node created is saved into new_root_index
                        root_set = true;
                    }
                }

                // We are using a stack to process depth-first, so in order for the children to be
                // processed in the order in which they appear we must push them in reverse order,
                // otherwise the last child will be processed first
                unsigned int n_child = 0;
                if (current.doc.count("children")) {
                    auto& children_list = current.doc.at("children");
                    for (auto child = children_list.rbegin(); child != children_list.rend(); ++child) {
                        pending_nodes.push_back({*child, current_node_index, n_child++});
                    }                    
                }
            }

            new_root_index_out = new_root_index;
        }

        void set_node_tree_materials_and_names(const json& node_document, node_database::size_type root_index, node_database& db)
        {
            struct json_context { const json& doc; node_database::size_type node_index; };
            std::vector<json_context> pending_nodes;
            pending_nodes.push_back({node_document, root_index });
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.back();
                pending_nodes.pop_back();
                auto& current_node = db.at(current.node_index);

                user_id material_user_id = current.doc.value("material", nuser_id);
                material_map::iterator mit;
                if ((mit = material_ids.find(material_user_id)) != material_ids.end()) {
                    current_node.m_elem.m_material = mit->second;
                }

                if (current.doc.count("name")) {
                    current_node.m_elem.m_name = current.doc.at("name").get<std::string>();
                }

                // Note that in this case it's not relevant in what order the children are processed,
                // but we still push them in reverse order for consistency
                if (current.doc.count("children")) {
                    auto& children_list = current.doc.at("children");
                    auto node_it = current_node.rbegin();
                    for (auto json_it = children_list.rbegin(); json_it != children_list.rend(); ++json_it) {
                        pending_nodes.push_back({*json_it, index(node_it)});
                        ++node_it;
                    }
                }
            }
        }

        void load_nodes(const json& scene_doc, node_database::size_type scene_root_index, view_database& db)
        {
            for (auto& node_document : scene_doc.at("nodes")) {
                node_database::size_type added_root = node_database::value_type::npos;
                create_node_tree(node_document, scene_root_index, added_root, db);
                set_node_tree_materials_and_names(node_document, added_root, db.m_nodes);
            }
        }

        void create_point_light(const json& point_light_document, view_database& db)
        {
             auto new_point_light_index = db.m_point_lights.insert(point_light(), point_light_database::value_type::root);
             auto& new_point_light = db.m_point_lights.at(new_point_light_index);
             new_point_light.m_elem.m_user_id = point_light_document.value("user_id", nuser_id);
             new_point_light.m_elem.m_position = array_to_vec3(point_light_document.at("position"));
             new_point_light.m_elem.m_ambient_color = array_to_vec3(point_light_document.at("ambient_color"));
             new_point_light.m_elem.m_diffuse_color = array_to_vec3(point_light_document.at("diffuse_color"));
             new_point_light.m_elem.m_specular_color = array_to_vec3(point_light_document.at("specular_color"));
             new_point_light.m_elem.m_constant_attenuation = point_light_document.at("constant_attenuation").get<float>();
             new_point_light.m_elem.m_linear_attenuation = point_light_document.at("linear_attenuation").get<float>();
             new_point_light.m_elem.m_quadratic_attenuation = point_light_document.at("quadratic_attenuation").get<float>();
        }

        void load_point_lights(const json& scene_doc, view_database& db)
        {
            for (auto& point_light_document : scene_doc.at("point_lights")) {
                create_point_light(point_light_document, db);
            }
        }

        void load_scenes(view_database& db)
        {
            for (auto& scene_doc : document.at("scenes")) {
                auto new_scene_index = db.m_scenes.insert(scene(), scene_database::value_type::root);
                auto& new_scene = db.m_scenes.at(new_scene_index);

                auto scene_root_node_index = db.m_nodes.insert(node(), node_database::value_type::root);
                new_scene.m_elem.m_root_node = scene_root_node_index;

                new_scene.m_elem.m_user_id = scene_doc.value("user_id", nuser_id);

                user_id skybox_user_id = scene_doc.value("skybox", nuser_id);
                cubemap_map::iterator mit;
                if ((mit = cubemap_ids.find(skybox_user_id)) != cubemap_ids.end()) {
                    new_scene.m_elem.m_skybox = mit->second;
                }

                new_scene.m_elem.m_dirlight.m_ambient_color = array_to_vec3(scene_doc.at("directional_light").at("ambient_color"));
                new_scene.m_elem.m_dirlight.m_diffuse_color = array_to_vec3(scene_doc.at("directional_light").at("diffuse_color"));
                new_scene.m_elem.m_dirlight.m_specular_color = array_to_vec3(scene_doc.at("directional_light").at("specular_color"));
                new_scene.m_elem.m_dirlight.m_direction = array_to_vec3(scene_doc.at("directional_light").at("direction"));

                load_nodes(scene_doc, scene_root_node_index, db);
                load_point_lights(scene_doc, db);
            }
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
            material_ids.clear();
            mesh_ids.clear();
            resource_ids.clear();
            cubemap_ids.clear();
            initialized = true;
            log(LOG_LEVEL_DEBUG, "database_loader: database loader initialized");
        }
    }

    void load_database(view_database& db)
    {
        if (!initialized) return;
        std::string filename = cmd_line_args_get_option_value("-config", "");

        view_database tmp_db;

        log(LOG_LEVEL_DEBUG, std::string("database_loader: loading database from file ") + filename);
        std::ifstream ifs(filename);
        document = json();
        ifs >> document;

        load_materials(tmp_db.m_materials);
        load_meshes(tmp_db.m_meshes);
        load_resources(tmp_db);
        load_cubemaps(tmp_db.m_cubemaps);
        load_scenes(tmp_db);

        db = std::move(tmp_db);
        document = json();
        log(LOG_LEVEL_DEBUG, "database_loader: database loaded successfully");
    }

    void log_database(const view_database& db)
    {
        log_materials(db);
        log_meshes(db);
        log_resources(db);
        log_cubemaps(db);
        log_scenes(db);
    }

    void database_loader_finalize()
    {
        if (initialized) {
            log(LOG_LEVEL_DEBUG, "database_loader: finalizing database loader");
            initialized = false;
            log(LOG_LEVEL_DEBUG, "database_loader: finalized");
        }
    }
} // namespace rte
