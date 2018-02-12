#include "serialization_utils.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "rte_domain.hpp"
#include "glm/glm.hpp"
#include "log.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace rte
{
    namespace
    {
        std::string format_mesh_id(mesh_database::size_type m)
        {    
            std::ostringstream oss; 
            if (m != mesh_database::value_type::npos) {
                oss << m;
            } else {
                oss << "nmesh";
            }

            return oss.str();
        }

        std::string format_material_id(material_database::size_type m)
        {
            std::ostringstream oss;
            if (m != material_database::value_type::npos) {
                oss << m;
            } else {
                oss << "nmat";
            }
    
            return oss.str();
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

        void log_mesh(mesh_database::size_type mesh_index, const view_database& db)
        {
            auto& m = db.m_meshes.at(mesh_index).m_elem;
            std::ostringstream oss;
            oss << std::setprecision(2) << std::fixed;
            oss << "    " << "index: " << mesh_index;
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        m.m_vertices: " << m.m_vertices.size();
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        user id: " << format_user_id(m.m_user_id);
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        vertex base: ";
            if (m.m_vertices.size()) {
                preview_sequence(&m.m_vertices[0][0], 3 * m.m_vertices.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        texture coords: ";
            if (m.m_texture_coords.size()) {
                preview_sequence(&m.m_texture_coords[0][0], 2 * m.m_texture_coords.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        m.m_indices: ";
            if (m.m_indices.size()) {
                preview_sequence(&m.m_indices[0], m.m_indices.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        m.m_normals: ";
            if (m.m_normals.size()) {
                preview_sequence(&m.m_normals[0][0], 3 * m.m_normals.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
        }

        void log_resource(resource_database::value_type::npos root_index, const resource_database& db)
        {
            // Iterate the resource tree with a depth-first search printing resources
            struct context{ resource_database::value_type::npos resource_index; unsigned int indentation; };
            std::vector<context, std::vector<context>> pending_nodes;
            pending_nodes.push_back({root_index, 1U});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.back();
                pending_nodes.pop_back();
                visited_resources.insert(current.resource_index);
    
                auto& res = db.at(current.resource_index);
    
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed;
                for (unsigned int i = 0; i < current.indentation; i++) {
                    oss << "    ";
                }
    
                oss << "[ ";
                oss << "resource index: " << current.resource_index;
                oss << ", user id: " << format_user_id(res.m_elem.m_user_id);
                oss << ", name: " << res.m_elem.m_name;
                oss << ", mesh: " << format_mesh_id(res.m_elem.m_mesh);
                oss << ", material: " << format_material_id(res.m_elem.m_material);
                oss << ", local transform: " << res.m_elem.m_local_transform;
                oss << " ]";
                log(LOG_LEVEL_DEBUG, oss.str().c_str());

                // We are using a stack to process depth-first, so in order for the children to be
                // processed in the order in which they appear we must push them in reverse order,
                // otherwise the last child will be processed first    
                for (auto& it = res.rbegin(); it != res.rend(); ++it) {
                    pending_nodes.push_back({index(it), current.indentation + 1});
                }
            }
        }
    } // Anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void log_materials(const view_database& db)
    {    
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: materials begin");
        auto& material_list = db.m_materials.at(0);
        for (auto& it = material_list.begin(); it != material_list.end(); ++it) {
            auto& mat = it->m_elem;
            std::ostringstream oss; 
            oss << std::setprecision(2) << std::fixed;
            oss << "    material index: " << m;
            oss << ", user id: " << format_user_id(mat.m_user_id);
            oss << ", name: " << mat.m_name;
            oss << ", diffuse color: " << mat.m_diffuse_color;
            oss << ", color specular: " << mat.m_specular_color;
            oss << ", smoothness: " << mat.m_smoothness;
            oss << ", texture path: " << mat.m_texture_path;
            oss << ", reflectivity: " << mat.m_reflectivity;
            oss << ", translucency: " << mat.m_translucency;
            oss << ", refractive_index: " << mat.m_refractive_index;
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
        }
        log(LOG_LEVEL_DEBUG, "resource_database: materials end");
    }

    void log_meshes(const view_database& db)
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: meshes begin");
        auto& mesh_list = db.m_meshes.at(0);
        for (auto& it : mesh_list.begin(); it != mesh_list.end(); ++it) {
            log_mesh(index(it), db);
        }

        log(LOG_LEVEL_DEBUG, "resource_database: meshes end");
    }


    void log_resources(const view_database& db)
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: resources begin");
        auto& resource_list = db.m_resources.at(0);
        for (auto& it = resource_list.begin(); it != resource_list.end(); ++it) {
            log_resource(index(it), db.m_resources);
        }

        log(LOG_LEVEL_DEBUG, "resource_database: resources end");
    }

    void log_cubemaps(const view_database& db)
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: cubemaps begin");
        auto& cubemap_list = db.m_cubemaps.at(0);
        for (auto& it = cubemap_list.begin(); it != cubemap_list.end(); ++it) {
            auto& cube = *it;
            std::ostringstream oss;

            oss.str("");
            oss << "    index: " << index(it);
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            oss.str("");
            oss << "    faces:";
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            for (auto& f : cube.m_elem.m_faces) {
                oss.str("");
                oss << "        " << f;
                log(LOG_LEVEL_DEBUG, oss.str().c_str());
            }
        }
        log(LOG_LEVEL_DEBUG, "resource_database: cubemaps end");
    }

    void log_node(node_database::size_type root, const node_database& db)
    {
        // Iterate the node tree with a depth-first search printing nodes
        struct context{ node_database::size_type node_index; unsigned int indentation; };
        std::vector<context> pending_nodes;
        pending_nodes.push_back({root, 3U});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.back();
            pending_nodes.pop_back();

            auto& n = db.at(current.node_index);

            std::ostringstream oss;
            oss << std::setprecision(2) << std::fixed;
            for (unsigned int i = 0; i < current.indentation; i++) {
                oss << "    ";
            }

            oss << "[ ";
            oss << "node id: " << current.node_index;
            oss << ", user id: " << format_user_id(n.m_user_id);
            oss << ", name: " << n.m_name;
            oss << ", mesh: " << format_mesh_id(n.m_mesh);
            oss << ", material: " << format_material_id(n.m_material);
            oss << ", local transform: " << n.m_local_transform;
            oss << " ]";
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            // We are using a stack to process depth-first, so in order for the children to be
            // processed in the order in which they appear we must push them in reverse order,
            // otherwise the last child will be processed first
            for (auto it = n.rbegin(); it != n.rend(); ++it) {
                pending_nodes.push_back({index(it), current.indentation + 1});
            }
        }
    }

    void log_directional_light(const scene_database::value_type& s)
    {
        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        oss << "        directional light: ";
        oss << "[ ambient color : " << s.m_elem.m_dirlight.m_ambient_color;
        oss << ", diffuse color : " << s.m_elem.m_dirlight.m_diffuse_color;
        oss << ", specular color : " << s.m_elem.m_dirlight.m_specular_color;
        oss << ", direction : " << s.m_elem.m_dirlight.m_direction;
        oss << " ]";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }

    void log_point_light(point_ligth_database::size_type point_light_index, const point_light_database& db)
    {
        auto& pl = db.at(point_light_index);
        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        oss << "            [ point light index: " << point_light_index;
        oss << ", user_id : " << format_user_id(pl.m_elem.m_user_id);
        oss << ", position : " << pl.m_elem.m_position;
        oss << ", ambient color : " << pl.m_elem.m_ambient_color;
        oss << ", diffuse color : " << pl.m_elem.m_diffuse_color;
        oss << ", specular color : " << pl.m_elem.m_specular_color;
        oss << ", constant_attenuation : " << pl.m_elem.m_constant_attenuation;
        oss << ", linear_attenuation : " << pl.m_elem.m_linear_attenuation;
        oss << ", quadratic_attenuation : " << pl.m_elem.m_quadratic_attenuation;
        oss << " ]";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }

    void log_scene(scene_database::size_type scene_index, const view_database& db)
    {
        auto& s = db.m_scenes.at(scene_index);

        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        oss << "    scene index: " << scene_index;
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        oss.str("");
        oss << "        user_id : " << format_user_id(s.m_user_id);
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        log_directional_light(s);

        oss.str("");
        oss << "        point lights :";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        auto& point_light_list = db.m_point_lights.at(s.m_elem.m_point_lights);
        for (auto it = point_light_list.begin(); it != point_light_list.end(); ++it) {
            log_point_light(index(it), db.m_point_lights);
        }

        oss.str("");
        node_database::size_type root = get_scene_root_node(s);
        oss << "        root node :";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        node_database::size_type root_node_index = s.m_elem.m_root_node;
        if (root_node_index != node_database::value_type::npos) {
            log_node(root_node_index, db.m_nodes);
        }
    }

    void log_scenes(const view_database& db)
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "scenegraph: scenes begin");
        auto& scene_list = db.m_scenes(0);
        for (auto it = scene_list.begin(); it != scene_list.end(); ++it) {
            log_scene(index(it), db.m_scens);
        }
        log(LOG_LEVEL_DEBUG, "scenegraph: scenes end");
    }
} // namespace rte
