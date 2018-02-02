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
#include <stack>
#include <map>

using json = nlohmann::json;

namespace rte
{
    namespace
    {
        typedef std::map<user_id, mat_id>      material_map;
        typedef std::map<user_id, mesh_id>     mesh_map;
        typedef std::map<user_id, resource_id> resource_map;
        typedef std::map<user_id, cubemap_id>  cubemap_map;

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
        material_map            material_ids;
        mesh_map                mesh_ids;
        resource_map            resource_ids;
        cubemap_map             cubemap_ids;
        scene_id                current_scene = nscene;

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        glm::vec3 array_to_vec3(const json& array) {
            return glm::vec3(array.at(0).get<float>(), array.at(1).get<float>(), array.at(2).get<float>());
        }

        void load_materials()
        {
            material_vector added_materials;
            for (auto& m : document.at("materials")) {
                auto mat = make_material();
                user_id material_user_id = m.value("user_id", nuser_id);
                set_material_diffuse_color(mat.get(), array_to_vec3(m.at("diffuse_color")));
                set_material_specular_color(mat.get(), array_to_vec3(m.at("specular_color")));
                set_material_smoothness(mat.get(), m.value("smoothness", 1.0f));
                set_material_texture_path(mat.get(), m.value("texture_path", std::string("")));
                set_material_reflectivity(mat.get(), m.value("reflectivity", 0.0f));
                set_material_translucency(mat.get(), m.value("translucency", 0.0f));
                set_material_refractive_index(mat.get(), m.value("refractive_index", 1.0f));
                set_material_name(mat.get(), m.value("name", std::string("")));
                if (material_user_id != nuser_id) {
                    set_material_user_id(mat.get(), material_user_id);
                    material_ids[material_user_id] = mat.get();
                }
                added_materials.push_back(std::move(mat));
            }

            materials.insert(materials.end(), make_move_iterator(added_materials.begin()), make_move_iterator(added_materials.end()));
        }

        void fill_index_vector(const json& mesh_document, std::vector<vindex>* out, const std::string& field_name)
        {
            for (auto& index : mesh_document.at(field_name)) {
                out->push_back(index);
            }
        }

        void fill_2d_vector(const json& mesh_document, std::vector<glm::vec2>* out, const std::string& field_name)
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
                    out->push_back(glm::vec2(u, v));
                }
            }
        }

        void fill_3d_vector(const json& mesh_document, std::vector<glm::vec3>* out, const std::string& field_name)
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
                    out->push_back(glm::vec3(x, y, z));
                }
            }
        }

        void load_meshes()
        {
            mesh_vector added_meshes;
            for (auto& m : document.at("meshes")) {
                auto mesh = make_mesh();
                user_id mesh_user_id = m.value("user_id", nuser_id);
                if (mesh_user_id != nuser_id) {
                    set_mesh_user_id(mesh.get(), mesh_user_id);
                    mesh_ids[mesh_user_id] = mesh.get();
                }
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

        bool resource_has_child(resource_id r, unsigned int index, resource_id* child_out)
        {
            resource_id child = get_first_child_resource(r);
            unsigned int n_child = 0U;
            while (child != nresource && n_child++ < index) {
                child = get_next_sibling_resource(child);
            }

            *child_out = child;
            return (child != nresource);
        }

        void create_resource(const json& resource_document,
                            resource_id parent,
                            resource_vector* resources_out,
                            material_vector* materials_out,
                            mesh_vector* meshes_out,
                            resource_id* root_out)
        {
            resource_vector added_resources;
            material_vector added_materials;
            mesh_vector added_meshes;
            if (resource_document.count("from_file")) {
                load_resources(resource_document.value("from_file", std::string()), root_out, &added_resources, &added_materials, &added_meshes);
            } else {
                unique_resource new_resource = make_resource(parent);
                user_id mesh_user_id = resource_document.value("mesh", nuser_id);
                mesh_map::iterator mit;
                if ((mit = mesh_ids.find(mesh_user_id)) != mesh_ids.end()) {
                    set_resource_mesh(new_resource.get(), mit->second);
                }
                *root_out = new_resource.get();
                added_resources.push_back(std::move(new_resource));
                // materials are set later in a second traversal
            }
            set_resource_name(*root_out, resource_document.value("name", std::string()));
            set_resource_user_id(*root_out, resource_document.value("user_id", nuser_id));
            if (resource_document.count("user_id")) {
                resource_ids[resource_document.at("user_id").get<user_id>()] = *root_out;
            }

            resources_out->insert(resources_out->end(), make_move_iterator(added_resources.begin()), make_move_iterator(added_resources.end()));
            materials_out->insert(materials_out->end(), make_move_iterator(added_materials.begin()), make_move_iterator(added_materials.end()));
            meshes_out->insert(meshes_out->end(), make_move_iterator(added_meshes.begin()), make_move_iterator(added_meshes.end()));
        }

        void create_resource_tree(const json& resource_document,
                            resource_vector* resources_out,
                            material_vector* materials_out,
                            mesh_vector* meshes_out,
                            resource_id* root)
        {
            resource_vector added_resources;
            material_vector added_materials;
            mesh_vector added_meshes;
            bool root_set = false;
            *root = nresource;
            struct json_context { const json& doc; resource_id parent; unsigned int index; };
            std::stack<json_context, std::vector<json_context>> pending_nodes;
            pending_nodes.push({resource_document, nresource, 0U});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.top();
                pending_nodes.pop();
                resource_id current_resource = nresource;
                if (current.parent == nresource || !resource_has_child(current.parent, current.index, &current_resource)) {
                    create_resource(current.doc, current.parent, &added_resources, &added_materials, &added_meshes, &current_resource);
                    if (!root_set) {
                        *root = current_resource; // only the first resource created is saved into root
                        root_set = true;
                    }
                }

                // We are using a stack to process depth-first, so in order for the children to be
                // processed in the order in which they appear we must push them in reverse order,
                // otherwise the last child will be processed first
                std::vector<json_context> children_list;
                unsigned int n_child = 0;
                if (current.doc.count("children")) {
                    for (auto& child : current.doc.at("children")) {
                        children_list.push_back({child, current_resource, n_child++});
                    }                    
                }

                for (auto cit = children_list.rbegin(); cit != children_list.rend(); cit++) {
                    pending_nodes.push(*cit);
                }
            }

            resources_out->insert(resources_out->end(), make_move_iterator(added_resources.begin()), make_move_iterator(added_resources.end()));
            materials_out->insert(materials_out->end(), make_move_iterator(added_materials.begin()), make_move_iterator(added_materials.end()));
            meshes_out->insert(meshes_out->end(), make_move_iterator(added_meshes.begin()), make_move_iterator(added_meshes.end()));
        }

        void set_resource_tree_materials_and_names(const json& resource_document, resource_id root)
        {
            struct json_context { const json& doc; resource_id rid; };
            std::stack<json_context, std::vector<json_context>> pending_nodes;
            pending_nodes.push({resource_document, root });
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.top();
                pending_nodes.pop();

                user_id material_user_id = current.doc.value("material", nuser_id);
                material_map::iterator mit;
                if ((mit = material_ids.find(material_user_id)) != material_ids.end()) {
                    set_resource_material(current.rid, mit->second);
                }

                if (current.doc.count("name")) {
                    set_resource_name(current.rid, current.doc.at("name").get<std::string>());
                }

                // We are using a stack to process depth-first, so in order for the children to be
                // processed in the order in which they appear we must push them in reverse order,
                // otherwise the last child will be processed first
                std::vector<json_context> children_list;
                resource_id child = get_first_child_resource(current.rid);
                if (current.doc.count("children")) {
                    for (auto& json_child : current.doc.at("children")) {
                        children_list.push_back({json_child, child});
                        child = get_next_sibling_resource(child);
                    }                    
                }

                for (auto cit = children_list.rbegin(); cit != children_list.rend(); cit++) {
                    pending_nodes.push(*cit);
                }
            }
        }

        void load_resources()
        {
            resource_vector added_resources;
            material_vector added_materials;
            mesh_vector added_meshes;
            for (auto& r : document.at("resources")) {
                resource_id added_root;
                create_resource_tree(r, &added_resources, &added_materials, &added_meshes, &added_root);
                set_resource_tree_materials_and_names(r, added_root);
            }

            resources.insert(resources.end(), make_move_iterator(added_resources.begin()), make_move_iterator(added_resources.end()));
            materials.insert(materials.end(), make_move_iterator(added_materials.begin()), make_move_iterator(added_materials.end()));
            meshes.insert(meshes.end(), make_move_iterator(added_meshes.begin()), make_move_iterator(added_meshes.end()));
        }

        void load_cubemaps()
        {
            cubemap_vector added_cubemaps;
            for (auto& cubemap_doc : document.at("cubemaps")) {
                unique_cubemap cubemap = make_cubemap();
                std::vector<std::string> cubemap_faces;
                for (auto& face_doc : cubemap_doc.at("faces")) {
                    cubemap_faces.push_back(face_doc.get<std::string>());
                }

                set_cubemap_faces(cubemap.get(), cubemap_faces);
                if (cubemap_doc.count("user_id")) {
                    cubemap_ids[cubemap_doc.at("user_id").get<user_id>()] = cubemap.get();
                }
                added_cubemaps.push_back(std::move(cubemap));
            }

            cubemaps.insert(cubemaps.end(), make_move_iterator(added_cubemaps.begin()), make_move_iterator(added_cubemaps.end()));
        }

        bool node_has_child(node_id n, unsigned int index, node_id* child_out)
        {
            node_id child = get_first_child_node(n);
            unsigned int n_child = 0U;
            while (child != nnode && n_child++ < index) {
                child = get_next_sibling_node(child);
            }

            *child_out = child;
            return (child != nnode);
        }

        void create_node(const json& node_document, node_id parent, node_vector* nodes_out, node_id* root_out)
        {
            node_id new_node = nnode;
            node_vector added_nodes;
            user_id resource_user_id = node_document.value("resource", nuser_id);
            resource_id resource = (resource_user_id == nuser_id ? nresource : resource_ids.at(resource_user_id));
            // resource can be nresource, in that case make_node() creates an empty node
            make_node(parent, resource, &new_node, &added_nodes);

            set_node_name(new_node, node_document.value("name", std::string()));
            set_node_user_id(new_node, node_document.value("user_id", nuser_id));

            // The transform inherited from the resource is only overwritten if the document includes all required properties
            if (node_document.count("scale") && node_document.count("rotation_angle") && node_document.count("rotation_axis") && node_document.count("translation")) {
                glm::mat4 scale = glm::scale(array_to_vec3(node_document.at("scale")));
                glm::mat4 rotation = glm::rotate(node_document.at("rotation_angle").get<float>(), array_to_vec3(node_document.at("rotation_axis")));;
                glm::mat4 translation = glm::translate(array_to_vec3(node_document.at("translation")));
                set_node_transform(new_node, translation * rotation * scale);
            }
            // materials are set later in a second traversal

            *root_out = new_node;
            nodes_out->insert(nodes_out->end(), make_move_iterator(added_nodes.begin()), make_move_iterator(added_nodes.end()));
        }

        void create_node_tree(const json& node_document, node_id scene_root, node_vector* nodes_out, node_id* root)
        {
            node_vector added_nodes;
            material_vector added_materials;
            mesh_vector added_meshes;
            bool root_set = false;
            *root = nnode;
            struct json_context { const json& doc; node_id parent; unsigned int index; };
            std::stack<json_context, std::vector<json_context>> pending_nodes;
            pending_nodes.push({node_document, scene_root, 0U});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.top();
                pending_nodes.pop();
                node_id current_node = nnode;
                if (current.parent == scene_root || !node_has_child(current.parent, current.index, &current_node)) {
                    create_node(current.doc, current.parent, &added_nodes, &current_node);
                    if (!root_set) {
                        *root  = current_node; // only the first node created is saved into root
                        root_set = true;
                    }
                }

                // We are using a stack to process depth-first, so in order for the children to be
                // processed in the order in which they appear we must push them in reverse order,
                // otherwise the last child will be processed first
                std::vector<json_context> children_list;
                unsigned int n_child = 0;
                if (current.doc.count("children")) {
                    for (auto& child : current.doc.at("children")) {
                        children_list.push_back({child, current_node, n_child++});
                    }                    
                }

                for (auto cit = children_list.rbegin(); cit != children_list.rend(); cit++) {
                    pending_nodes.push(*cit);
                }
            }

            nodes_out->insert(nodes_out->end(), make_move_iterator(added_nodes.begin()), make_move_iterator(added_nodes.end()));
        }

        void set_node_tree_materials_and_names(const json& node_document, node_id root)
        {
            struct json_context { const json& doc; node_id nid; };
            std::stack<json_context, std::vector<json_context>> pending_nodes;
            pending_nodes.push({node_document, root });
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.top();
                pending_nodes.pop();

                user_id material_user_id = current.doc.value("material", nuser_id);
                material_map::iterator mit;
                if ((mit = material_ids.find(material_user_id)) != material_ids.end()) {
                    set_node_material(current.nid, mit->second);
                }

                if (current.doc.count("name")) {
                    set_node_name(current.nid, current.doc.at("name").get<std::string>());
                }

                // We are using a stack to process depth-first, so in order for the children to be
                // processed in the order in which they appear we must push them in reverse order,
                // otherwise the last child will be processed first
                std::vector<json_context> children_list;
                node_id child = get_first_child_node(current.nid);
                if (current.doc.count("children")) {
                    for (auto& json_child : current.doc.at("children")) {
                        children_list.push_back({json_child, child});
                        child = get_next_sibling_node(child);
                    }
                }

                for (auto cit = children_list.rbegin(); cit != children_list.rend(); cit++) {
                    pending_nodes.push(*cit);
                }
            }
        }

        void load_nodes(const json& scene_doc)
        {
            node_vector added_nodes;
            for (auto& n : scene_doc.at("nodes")) {
                node_id added_root = nnode;
                create_node_tree(n, get_scene_root_node(current_scene), &added_nodes, &added_root);
                set_node_tree_materials_and_names(n, added_root);
            }

            nodes.insert(nodes.end(), make_move_iterator(added_nodes.begin()), make_move_iterator(added_nodes.end()));
        }

        void load_directional_light(const json& scene_doc)
        {
            set_directional_light_ambient_color(current_scene, array_to_vec3(scene_doc.at("directional_light").at("ambient_color")));
            set_directional_light_diffuse_color(current_scene, array_to_vec3(scene_doc.at("directional_light").at("diffuse_color")));
            set_directional_light_specular_color(current_scene, array_to_vec3(scene_doc.at("directional_light").at("specular_color")));
            set_directional_light_direction(current_scene, array_to_vec3(scene_doc.at("directional_light").at("direction")));
        }

        void create_point_light(const json& point_light_document, point_light_vector* point_lights)
        {
            unique_point_light pl = make_point_light(current_scene);
            set_point_light_user_id(pl.get(), point_light_document.value("user_id", nuser_id));
            set_point_light_position(pl.get(), array_to_vec3(point_light_document.at("position")));
            set_point_light_ambient_color(pl.get(), array_to_vec3(point_light_document.at("ambient_color")));
            set_point_light_diffuse_color(pl.get(), array_to_vec3(point_light_document.at("diffuse_color")));
            set_point_light_specular_color(pl.get(), array_to_vec3(point_light_document.at("specular_color")));
            set_point_light_constant_attenuation(pl.get(), point_light_document.at("constant_attenuation").get<float>());
            set_point_light_linear_attenuation(pl.get(), point_light_document.at("linear_attenuation").get<float>());
            set_point_light_quadratic_attenuation(pl.get(), point_light_document.at("quadratic_attenuation").get<float>());
            point_lights->push_back(std::move(pl));
        }

        void load_point_lights(const json& scene_doc)
        {
            point_light_vector added_point_lights;
            for (auto& point_light_document : scene_doc.at("point_lights")) {
                create_point_light(point_light_document, &added_point_lights);
            }
            point_lights.insert(point_lights.end(), make_move_iterator(added_point_lights.begin()), make_move_iterator(added_point_lights.end()));
        }

        void load_scenes()
        {
            scene_vector added_scenes;
            for (auto& scene_doc : document.at("scenes")) {
                unique_scene scene = make_scene();
                set_scene_user_id(scene.get(), scene_doc.value("user_id", nuser_id));
                current_scene = scene.get();

                user_id skybox_user_id = scene_doc.value("skybox", nuser_id);
                cubemap_map::iterator mit;
                if ((mit = cubemap_ids.find(skybox_user_id)) != cubemap_ids.end()) {
                    set_scene_skybox(scene.get(), mit->second);
                }

                load_nodes(scene_doc);
                load_directional_light(scene_doc);
                load_point_lights(scene_doc);

                added_scenes.push_back(std::move(scene));
                current_scene = nscene;
            }

            scenes.insert(scenes.end(), make_move_iterator(added_scenes.begin()), make_move_iterator(added_scenes.end()));
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
            resource_ids.clear();
            initialized = true;
            log(LOG_LEVEL_DEBUG, "database_loader: database loader initialized");
        }
    }

    void load_database()
    {
        if (!initialized) return;
        std::string filename = cmd_line_args_get_option_value("-config", "");

        log(LOG_LEVEL_DEBUG, std::string("database_loader: loading database from file ") + filename);
        std::ifstream ifs(filename);
        document = json();
        ifs >> document;

        load_materials();
        load_meshes();
        load_resources();
        load_cubemaps();
        load_scenes();

        document = json();
        log(LOG_LEVEL_DEBUG, "database_loader: database loaded successfully");
    }

    void log_database()
    {
        log_materials();
        log_meshes();
        log_resources();
        log_cubemaps();
        log_scenes();
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
            log(LOG_LEVEL_DEBUG, "database_loader: finalized");
        }
    }
} // namespace rte
