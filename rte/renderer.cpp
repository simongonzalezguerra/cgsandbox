#include "resource_database.hpp"
#include "math_utils.hpp"
#include "renderer.hpp"
#include "system.hpp"
#include "log.hpp"

#include <algorithm>
#include <sstream>
#include <memory>
#include <vector>

namespace rte
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        typedef std::vector<node_database::size_type> node_vector;

        node_vector                      nodes_to_render;
        glm::vec3                        camera_position_worldspace;
        gl_driver                        driver;
        gl_driver_context                driver_context;
        cubemap_database::size_type      skybox_id = cubemap_database::value_type::npos;
        default_texture_vector           default_textures;                    // placeholder, only contains one element
        texture_vector                   textures;
        buffer_vector                    buffers;
        gl_cubemap_vector                gl_cubemaps;
        buffer_vector                    gl_cubemap_position_buffers;         // placeholder, only contains one element
        buffer_vector                    gl_cubemap_index_buffers;            // placeholder, only contains one element
        program_vector                   phong_programs;                      // placeholder, only contains one element
        program_vector                   environment_mapping_programs;        // placeholder, only contains one element
        program_vector                   skybox_programs;                     // placeholder, only contains one element
        bool                             gl_driver_set = false;

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        void initialize_shaders()
        {
            // Load our shaders
            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading shaders");
            if (phong_programs.empty()) {
                phong_programs.push_back(make_program(driver, program_type::phong));
            }

            if (environment_mapping_programs.empty()) {
                environment_mapping_programs.push_back(make_program(driver, program_type::environment_mapping));
            }
            if (skybox_programs.empty()) {
                skybox_programs.push_back(make_program(driver, program_type::skybox));
            }

            log(LOG_LEVEL_DEBUG, "initialize_renderer: shaders loaded successfully");
        }

        void initialize_textures(view_database& db)
        {
            // Create a default texture to use as diffuse map on objects that don't have a texture
            // The custom deleter in unique_default_texture makes it impossible to declare an empty
            // handlers (there would no driver to initialize the deleter with) so we declare a vector
            // and only insert one element
            if (default_textures.empty()) {
                default_textures.push_back(make_default_texture(driver));
            }

            // Load all textures
            texture_vector new_textures;
            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading textures");
            auto& root_material = db.m_materials.at(material_database::value_type::root);
            for (auto it = root_material.begin(); it != root_material.end(); ++it) {
                auto& mat = *it;
                mat.m_elem.m_texture_id = default_textures[0].get();
                if (!mat.m_elem.m_texture_path.empty()) {
                    // Load the texture into memory            
                    image img;
                    img.load(mat.m_elem.m_texture_path);
                    // Load the texture into the graphics API
                    auto tex = make_texture(driver, img.get_width(), img.get_height(), img.get_format(), img.get_data());
                    mat.m_elem.m_texture_id = tex.get();
                    new_textures.push_back(std::move(tex));
                }
            }

            textures.insert(textures.end(), make_move_iterator(new_textures.begin()), make_move_iterator(new_textures.end()));
            log(LOG_LEVEL_DEBUG, "initialize_renderer: textures loaded successfully");
        }

        void initialize_meshes(view_database& db)
        {
            // Load all meshes
            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading meshes");
            auto& root_mesh = db.m_meshes.at(mesh_database::value_type::root);
            for (auto it = root_mesh.begin(); it != root_mesh.end(); ++it) {
                auto& m = *it;

                auto position_buffer = make_3d_buffer(driver, m.m_elem.m_vertices);
                auto uv_buffer = make_2d_buffer(driver, m.m_elem.m_texture_coords);
                auto normal_buffer = make_3d_buffer(driver, m.m_elem.m_normals);
                auto index_buffer = make_index_buffer(driver, m.m_elem.m_indices);

                m.m_elem.m_position_buffer_id = position_buffer.get();
                m.m_elem.m_uv_buffer_id = uv_buffer.get();
                m.m_elem.m_normal_buffer_id = normal_buffer.get();
                m.m_elem.m_index_buffer_id = index_buffer.get();

                buffers.push_back(std::move(position_buffer));
                buffers.push_back(std::move(uv_buffer));
                buffers.push_back(std::move(normal_buffer));
                buffers.push_back(std::move(index_buffer));
            }

            log(LOG_LEVEL_DEBUG, "initialize_renderer: meshes loaded succesfully");
        }

        std::vector<glm::vec3> make_skybox_positions()
        {
            std::vector<glm::vec3> skybox_positions =
            {
                {-1.0f,  1.0f, -1.0f},
                {-1.0f, -1.0f, -1.0f},
                { 1.0f, -1.0f, -1.0f},
                { 1.0f,  1.0f, -1.0f},
                {-1.0f, -1.0f,  1.0f},
                {-1.0f,  1.0f,  1.0f},
                { 1.0f, -1.0f,  1.0f},
                { 1.0f,  1.0f,  1.0f}
            };
            
            return skybox_positions;
        }

        std::vector<unsigned short> make_skybox_indices()
        {
            std::vector<unsigned short> skybox_indices =
            {
                0, 1, 2,
                2, 3, 0,
                4, 1, 0,
                0, 5, 4,
                2, 6, 7,
                7, 3, 2,
                4, 5, 7,
                7, 6, 4,
                0, 3, 7,
                7, 5, 0,
                1, 4, 2,
                2, 4, 6
            };
            
            return skybox_indices;
        }

        void initialize_gl_cubemaps(view_database& db)
        {
            if (gl_cubemap_position_buffers.empty()) {
                gl_cubemap_position_buffers.push_back(make_3d_buffer(driver, make_skybox_positions()));
            }

            if (gl_cubemap_index_buffers.empty()) {
                gl_cubemap_index_buffers.push_back(make_index_buffer(driver, make_skybox_indices()));
            }

            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading cubemaps");
            auto& root_cubemap = db.m_cubemaps.at(cubemap_database::value_type::root);
            for (auto it = root_cubemap.begin(); it != root_cubemap.end(); ++it) {
                auto& cm = *it;
                std::vector<const unsigned char*> faces_data;
                std::vector<std::unique_ptr<image>> faces;
                for (auto path : cm.m_elem.m_faces) {
                    auto face_ptr = std::make_unique<image>();
                    face_ptr->load(path);
                    faces.push_back(std::move(face_ptr));
                }
                if (faces.size() < 6) {
                    throw std::runtime_error("load_cubemap_faces: invalid cubemap data, faces unavailable");
                }
                std::for_each(faces.begin(), faces.end(), [&](const std::unique_ptr<image>& i) {
                    faces_data.push_back(i->get_data());
                });
                auto gl_cubemap = make_gl_cubemap(driver, faces[0]->get_width(), faces[0]->get_height(), faces[0]->get_format(), faces_data);
                cm.m_elem.m_gl_cubemap_id = gl_cubemap.get();
                gl_cubemaps.push_back(std::move(gl_cubemap));
            }

            log(LOG_LEVEL_DEBUG, "initialize_renderer: cubemaps loaded successfully");
        }
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void set_gl_driver(const gl_driver& d)
    {
        driver = d;
        gl_driver_set = true;
    }

    void initialize_renderer(view_database& db)
    {
        if (get_first_window() == nwindow) {
            throw std::logic_error("initialize_renderer: error, trying to initialize renderer but a context hasn't been created");
        }

        if (!gl_driver_set) {
            throw std::logic_error("initialize_renderer: error, trying to initialize renderer but a driver hasn't been set");
        }

        // Initialize the graphics API
        driver.gl_driver_init();
        initialize_shaders();
        initialize_textures(db);
        initialize_meshes(db);
        initialize_gl_cubemaps(db);
    }

    void finalize_renderer()
    {
        default_textures.clear();
        textures.clear();
        buffers.clear();
        gl_cubemaps.clear();
        gl_cubemap_position_buffers.clear();
        gl_cubemap_index_buffers.clear();
        phong_programs.clear();
        environment_mapping_programs.clear();
        skybox_programs.clear();
    }

    void get_scene_properties(const view_database& db)
    {
        // Set view and projection matrices for the scene
        auto& root_scene = db.m_scenes.at(scene_database::value_type::root);
        auto& current_scene = *(root_scene.begin());

        // Set projection and view transforms
        driver_context.m_projection = current_scene.m_elem.m_projection_transform;
        driver_context.m_view = current_scene.m_elem.m_view_transform;

        // Set directional light properties
        driver_context.m_dirlight.m_ambient_color = current_scene.m_elem.m_dirlight.m_ambient_color;
        driver_context.m_dirlight.m_diffuse_color = current_scene.m_elem.m_dirlight.m_diffuse_color;
        driver_context.m_dirlight.m_specular_color = current_scene.m_elem.m_dirlight.m_specular_color;
        driver_context.m_dirlight.m_direction_cameraspace = from_homogenous_coords(driver_context.m_view * direction_to_homogenous_coords(current_scene.m_elem.m_dirlight.m_direction));

        // Set the cubemap texture to use
        driver_context.m_gl_cubemap = 0U;
        skybox_id = current_scene.m_elem.m_skybox;
        if (skybox_id != cubemap_database::value_type::npos) {
            driver_context.m_gl_cubemap = db.m_cubemaps.at(skybox_id).m_elem.m_gl_cubemap_id;
        }

        // Set point light data
        auto& point_light_list = db.m_point_lights.at(current_scene.m_elem.m_point_lights);
        for (auto it = point_light_list.begin(); it != point_light_list.end(); ++it) {
            auto& pl = *it;
            point_light_data pl_data;
            pl_data.m_position_cameraspace = from_homogenous_coords(driver_context.m_view * position_to_homogenous_coords(pl.m_elem.m_position));
            pl_data.m_ambient_color = pl.m_elem.m_ambient_color;
            pl_data.m_diffuse_color = pl.m_elem.m_diffuse_color;
            pl_data.m_specular_color = pl.m_elem.m_specular_color;
            pl_data.m_constant_attenuation = pl.m_elem.m_constant_attenuation;
            pl_data.m_linear_attenuation = pl.m_elem.m_linear_attenuation;
            pl_data.m_quadratic_attenuation = pl.m_elem.m_quadratic_attenuation;
            driver_context.m_point_lights.push_back(pl_data);
        }

        // Set the depth func to use
        driver_context.m_depth_func = depth_func::less;
    }

    void get_node_properties(node_database::size_type node_index, const view_database& db)
    {
        auto& current_node = db.m_nodes.at(node_index);
        auto& current_material = db.m_materials.at(current_node.m_elem.m_material);
        auto& current_mesh = db.m_meshes.at(current_node.m_elem.m_mesh);

        driver_context.m_node.m_texture = current_material.m_elem.m_texture_id;

        driver_context.m_node.m_position_buffer = current_mesh.m_elem.m_position_buffer_id;
        driver_context.m_node.m_texture_coords_buffer = current_mesh.m_elem.m_uv_buffer_id;
        driver_context.m_node.m_normal_buffer = current_mesh.m_elem.m_normal_buffer_id;
        driver_context.m_node.m_index_buffer = current_mesh.m_elem.m_index_buffer_id;
        driver_context.m_node.m_num_indices = current_mesh.m_elem.m_indices.size();

        driver_context.m_node.m_material.m_diffuse_color = current_material.m_elem.m_diffuse_color;
        driver_context.m_node.m_material.m_specular_color = current_material.m_elem.m_specular_color;
        driver_context.m_node.m_material.m_smoothness = current_material.m_elem.m_smoothness;
        driver_context.m_node.m_material.m_reflectivity = current_material.m_elem.m_reflectivity;
        driver_context.m_node.m_material.m_translucency = current_material.m_elem.m_translucency;
        driver_context.m_node.m_material.m_refractive_index = current_material.m_elem.m_refractive_index;

        driver_context.m_node.m_model = current_node.m_elem.m_accum_transform;
    }

    void render_phong_nodes(const view_database& db)
    {
        // Render nodes that are neither reflective nor tranlucent with the phong model
        driver_context.m_program = phong_programs[0].get();
        for (auto node_index : nodes_to_render) {
            auto& current_node = db.m_nodes.at(node_index);
            auto& current_material = db.m_materials.at(current_node.m_elem.m_material);
            if (current_node.m_elem.m_material != material_database::value_type::npos
                    && current_material.m_elem.m_reflectivity == 0.0f
                    && current_material.m_elem.m_translucency == 0.0f) {
                driver_context.m_node = gl_node_context();
                get_node_properties(node_index, db);
                driver.draw(driver_context);
            }
        }
    }

    void render_environment_mapping_nodes(const view_database& db)
    {
        // Render reflective or translucent nodes
        driver_context.m_program = environment_mapping_programs[0].get();
        for (auto node_index : nodes_to_render) {
            auto& current_node = db.m_nodes.at(node_index);
            auto& current_material = db.m_materials.at(current_node.m_elem.m_material);
            if (current_material.m_elem.m_reflectivity > 0.0f
                    || current_material.m_elem.m_translucency > 0.0f) {
                driver_context.m_node = gl_node_context();
                get_node_properties(node_index, db);
                driver.draw(driver_context);
            }
        }
    }

    void render_skybox()
    {
        // Render the skybox
        if (skybox_id != cubemap_database::value_type::npos) {
            driver_context.m_program = skybox_programs.at(0).get();
            driver_context.m_node = gl_node_context();
            // Change depth function so depth test passes when values are equal to depth buffer's content
            driver_context.m_depth_func = depth_func::lequal;
            // Remove translation from the view matrix
            driver_context.m_view = glm::mat4(glm::mat3(driver_context.m_view));
            driver_context.m_node.m_position_buffer = gl_cubemap_position_buffers.at(0).get();
            driver_context.m_node.m_index_buffer = gl_cubemap_index_buffers.at(0).get();
            driver_context.m_node.m_num_indices = 36U;
            driver.draw(driver_context);
        }
    }

    void render(const view_database& db)
    {
        driver.initialize_frame();
        // Convert tree into list and filter out non-enabled nodes
        nodes_to_render.clear();
        auto& root_scene = db.m_scenes.at(scene_database::value_type::root);
        auto& current_scene = *(root_scene.begin());
        get_descendant_nodes(current_scene.m_elem.m_root_node, nodes_to_render, db);
        driver_context = gl_driver_context();
        get_scene_properties(db);
        render_phong_nodes(db);
        render_environment_mapping_nodes(db);
        render_skybox();
    }
} // namespace rte
