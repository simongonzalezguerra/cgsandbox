#include "serialization_utils.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "sparse_list.hpp"
#include "rte_domain.hpp"
#include "glm/glm.hpp"
#include "log.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace rte
{
    namespace
    {
        std::string format_mesh_id(index_type m)
        {    
            std::ostringstream oss; 
            if (m != npos) {
                oss << m;
            } else {
                oss << "nmesh";
            }

            return oss.str();
        }

        std::string format_material_id(index_type m)
        {
            std::ostringstream oss;
            if (m != npos) {
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

        void log_mesh(index_type mesh_index, const view_database& db)
        {
            auto& m = db.m_meshes.at(mesh_index);
            std::ostringstream oss;
            oss << std::setprecision(2) << std::fixed;
            oss << "    " << "index: " << mesh_index;
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            auto mesh_buffer_it = std::find_if(list_begin(db.m_mesh_buffers, 0), list_end(db.m_mesh_buffers, 0), [mesh_index](const mesh_buffer& mf) {
                return mf.m_mesh == mesh_index;
            });
            auto& mf = *mesh_buffer_it;

            oss.str("");
            oss << "        vertices: " << mf.m_vertices.size();
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        user id: " << format_user_id(m.m_user_id);
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        vertex base: ";
            if (mf.m_vertices.size()) {
                preview_sequence(&mf.m_vertices[0][0], 3 * mf.m_vertices.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        texture coords: ";
            if (mf.m_texture_coords.size()) {
                preview_sequence(&mf.m_texture_coords[0][0], 2 * mf.m_texture_coords.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        indices: ";
            if (mf.m_indices.size()) {
                preview_sequence(&mf.m_indices[0], mf.m_indices.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
    
            oss.str("");
            oss << "        normals: ";
            if (mf.m_normals.size()) {
                preview_sequence(&mf.m_normals[0][0], 3 * mf.m_normals.size(), oss);
            }
            log(LOG_LEVEL_DEBUG, oss.str().c_str());
        }

        void log_resource(index_type root_index, const resource_database& db)
        {
            // Iterate the resource tree with a depth-first search printing resources
            struct context{ index_type resource_index; unsigned int indentation; };
            std::vector<context> pending_nodes;
            pending_nodes.push_back({root_index, 1U});
            while (!pending_nodes.empty()) {
                auto current = pending_nodes.back();
                pending_nodes.pop_back();
    
                auto& res = db.at(current.resource_index);
    
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed;
                for (unsigned int i = 0; i < current.indentation; i++) {
                    oss << "    ";
                }
    
                oss << "[ ";
                oss << "index: " << current.resource_index;
                oss << ", user id: " << format_user_id(res.m_user_id);
                oss << ", name: " << res.m_name;
                oss << ", mesh: " << format_mesh_id(res.m_mesh);
                oss << ", material: " << format_material_id(res.m_material);
                oss << ", local transform: " << res.m_local_transform;
                oss << " ]";
                log(LOG_LEVEL_DEBUG, oss.str().c_str());

                // We are using a stack to process depth-first, so in order for the children to be
                // processed in the order in which they appear we must push them in reverse order,
                // otherwise the last child will be processed first    
                for (auto it = tree_begin(db, current.resource_index); it != tree_end(db, current.resource_index); ++it) {
                    pending_nodes.push_back({index(it), current.indentation + 1});
                }
            }
        }

        struct node_context{ index_type node_index; };
        typedef std::vector<node_context> node_context_vector;

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------    
        node_context_vector      pending_nodes;   //!< helper structure used in get_descendant_nodes
    } // Anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void log_materials(const view_database& db)
    {    
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: materials begin");
        for (auto it = list_begin(db.m_materials, 0); it != list_end(db.m_materials, 0); ++it) {
            auto& mat = *it;
            std::ostringstream oss; 
            oss << std::setprecision(2) << std::fixed;
            oss << "    index: " << index(it);
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
        for (auto it = list_begin(db.m_meshes, 0); it != list_end(db.m_meshes, 0); ++it) {
            log_mesh(index(it), db);
        }

        log(LOG_LEVEL_DEBUG, "resource_database: meshes end");
    }

    void log_resources(const view_database& db)
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: resources begin");
        for (auto it = tree_begin(db.m_resources, 0); it != tree_end(db.m_resources, 0); ++it) {
            log_resource(index(it), db.m_resources);
        }

        log(LOG_LEVEL_DEBUG, "resource_database: resources end");
    }

    void log_cubemaps(const view_database& db)
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: cubemaps begin");
        for (auto it = list_begin(db.m_cubemaps, 0); it != list_end(db.m_cubemaps, 0); ++it) {
            auto& cube = *it;
            std::ostringstream oss;

            oss.str("");
            oss << "    index: " << index(it);
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            oss.str("");
            oss << "    faces:";
            log(LOG_LEVEL_DEBUG, oss.str().c_str());

            for (auto& f : cube.m_faces) {
                oss.str("");
                oss << "        " << f;
                log(LOG_LEVEL_DEBUG, oss.str().c_str());
            }
        }
        log(LOG_LEVEL_DEBUG, "resource_database: cubemaps end");
    }

    void log_node(index_type root, const node_database& db)
    {
        // Iterate the node tree with a depth-first search printing nodes
        struct context{ index_type node_index; unsigned int indentation; };
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
            oss << "index: " << current.node_index;
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
            for (auto it = tree_begin(db, current.node_index); it != tree_end(db, current.node_index); ++it) {
                pending_nodes.push_back({index(it), current.indentation + 1});
            }
        }
    }

    void log_nodes(const view_database& db)
    {
        std::ostringstream oss;
        oss << "        root node :";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        index_type root_node_index = db.m_root_node;
        if (root_node_index != npos) {
            log_node(root_node_index, db.m_nodes);
        }
    }

    void log_directional_light(const view_database& db)
    {
        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        oss << "        directional light: ";
        oss << "[ ambient color : " << db.m_dirlight.m_ambient_color;
        oss << ", diffuse color : " << db.m_dirlight.m_diffuse_color;
        oss << ", specular color : " << db.m_dirlight.m_specular_color;
        oss << ", direction : " << db.m_dirlight.m_direction;
        oss << " ]";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }

    void log_point_light(index_type point_light_index, const point_light_database& db)
    {
        auto& pl = db.at(point_light_index);
        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed;
        oss << "            [ index: " << point_light_index;
        oss << ", user_id : " << format_user_id(pl.m_user_id);
        oss << ", position : " << pl.m_position;
        oss << ", ambient color : " << pl.m_ambient_color;
        oss << ", diffuse color : " << pl.m_diffuse_color;
        oss << ", specular color : " << pl.m_specular_color;
        oss << ", constant_attenuation : " << pl.m_constant_attenuation;
        oss << ", linear_attenuation : " << pl.m_linear_attenuation;
        oss << ", quadratic_attenuation : " << pl.m_quadratic_attenuation;
        oss << " ]";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }

    void log_point_lights(const view_database& db)
    {
        std::ostringstream oss;
        oss << "        point lights :";
        log(LOG_LEVEL_DEBUG, oss.str().c_str());
        for (auto it = list_begin(db.m_point_lights, 0); it != list_end(db.m_point_lights, 0); ++it) {
            log_point_light(index(it), db.m_point_lights);
        }        
    }

    void log_database(const view_database& db)
    {
        log_materials(db);
        log_meshes(db);
        log_resources(db);
        log_cubemaps(db);
        log_directional_light(db);
        log_point_lights(db);
        log_nodes(db);
    }

    void insert_node_tree(index_type root_resource_index,
                        index_type parent_index,
                        index_type& node_index_out,
                        view_database& db)
    {
        if (root_resource_index == npos) {
            node_index_out = tree_insert(db.m_nodes, node(), parent_index);
            return;
        }

        node_database tmp_db;
        tree_init(tmp_db);
        struct context{ index_type resource_index; index_type parent_node; };
        std::vector<context> pending_nodes;
        pending_nodes.push_back({root_resource_index, npos});
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.back();
            pending_nodes.pop_back();

            index_type new_node_index = tree_insert(tmp_db, node(), current.parent_node);
            auto& new_node = tmp_db.at(new_node_index);
            auto& res = db.m_resources.at(current.resource_index);
            new_node.m_local_transform = res.m_local_transform;
            new_node.m_mesh = res.m_mesh;
            new_node.m_material = res.m_material;

            for (auto it = tree_begin(db.m_resources, current.resource_index); it != tree_end(db.m_resources, current.resource_index); ++it) {
                pending_nodes.push_back({index(it), new_node_index});
            }
        }

        node_index_out = tree_insert(tmp_db, 0, db.m_nodes, parent_index);
    }

    void get_descendant_nodes(index_type root_index,
                        std::vector<index_type>& nodes_out,
                        const view_database& db)
    {
        pending_nodes.clear();
        pending_nodes.push_back({root_index});

        while (!pending_nodes.empty()) {
            auto current = pending_nodes.back();
            pending_nodes.pop_back();

            auto& current_node = db.m_nodes.at(current.node_index);

            // If a node is not enabled, all its subtree is pruned
            if (current_node.m_enabled) {
                // If a node doesn't have any meshes or materials it is ignored, but its children are processed
                if (current_node.m_mesh != npos
                        && current_node.m_material != npos) {
                    nodes_out.push_back(current.node_index);
                }
    
                for (auto it = tree_rbegin(db.m_nodes, current.node_index); it != tree_rend(db.m_nodes, current.node_index); ++it) {
                    pending_nodes.push_back({index(it)});
                }            
            }
        }
    }
} // namespace rte
