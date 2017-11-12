#include "cgs/resource_database.hpp"
#include "cgs/renderer.hpp"
#include "cgs/system.hpp"
#include "cgs/log.hpp"

#include <algorithm>
#include <sstream>
#include <memory>
#include <vector>

namespace cgs
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        std::vector<node_id>   nodes_to_render;
        glm::vec3              camera_position_worldspace;
        gl_driver              driver;
        gl_driver_context      driver_context;
        cubemap_id             skybox_id = ncubemap;
        default_texture_vector default_textures;                    // placeholder, only contains one element
        texture_vector         textures;
        buffer_vector          buffers;
        gl_cubemap_vector         gl_cubemaps;
        buffer_vector          gl_cubemap_position_buffers;            // placeholder, only contains one element
        buffer_vector          gl_cubemap_index_buffers;               // placeholder, only contains one element
        program_vector         phong_programs;                      // placeholder, only contains one element
        program_vector         environment_mapping_programs;        // placeholder, only contains one element
        program_vector         skybox_programs;                     // placeholder, only contains one element
        bool                   gl_driver_set = false;
        scene_id               current_scene = nscene;

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

        void initialize_textures()
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
            for (mat_id mat = get_first_material(); mat != nmat; mat = get_next_material(mat)) {
                set_material_texture_id(mat, default_textures[0].get());
                if (!get_material_texture_path(mat).empty()) {
                    // Load the texture into memory            
                    image img;
                    img.load(get_material_texture_path(mat));
                    // Load the texture into the graphics API
                    auto tex = make_texture(driver,img.get_width(), img.get_height(), img.get_format(), img.get_data());
                    set_material_texture_id(mat, tex.get());
                    new_textures.push_back(std::move(tex));
                }
            }

            textures.insert(textures.end(), make_move_iterator(new_textures.begin()), make_move_iterator(new_textures.end()));
            log(LOG_LEVEL_DEBUG, "initialize_renderer: textures loaded successfully");
        }

        void initialize_meshes()
        {
            // Load all meshes
            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading meshes");
            for (mesh_id mid = get_first_mesh(); mid != nmesh; mid = get_next_mesh(mid)) {
                auto position_buffer = make_3d_buffer(driver, get_mesh_vertices(mid));
                auto uv_buffer = make_2d_buffer(driver, get_mesh_texture_coords(mid));
                auto normal_buffer = make_3d_buffer(driver, get_mesh_normals(mid));
                auto index_buffer = make_index_buffer(driver, get_mesh_indices(mid));

                set_mesh_position_buffer_id(mid, position_buffer.get());
                set_mesh_uv_buffer_id(mid, uv_buffer.get());
                set_mesh_normal_buffer_id(mid, normal_buffer.get());
                set_mesh_index_buffer_id(mid, index_buffer.get());

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


        void initialize_gl_cubemaps()
        {
            if (gl_cubemap_position_buffers.empty()) {
                gl_cubemap_position_buffers.push_back(make_3d_buffer(driver, make_skybox_positions()));
            }

            if (gl_cubemap_index_buffers.empty()) {
                gl_cubemap_index_buffers.push_back(make_index_buffer(driver, make_skybox_indices()));
            }

            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading cubemaps");
            for (cubemap_id cid = get_first_cubemap(); cid != ncubemap; cid = get_next_cubemap(cid)) {
                std::vector<const unsigned char*> faces_data;
                std::vector<std::unique_ptr<image>> faces;
                for (auto path : get_cubemap_faces(cid)) {
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
                set_cubemap_gl_cubemap_id(cid, gl_cubemap.get());
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

    void initialize_renderer()
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
        initialize_textures();
        initialize_meshes();
        initialize_gl_cubemaps();
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

    void get_scene_properties()
    {
        // Set view and projection matrices for the scene
        get_scene_projection_transform(current_scene, &driver_context.m_projection);
        get_scene_view_transform(current_scene, &driver_context.m_view);

        // Set directional light properties
        driver_context.m_dirlight.m_ambient_color = get_directional_light_ambient_color(current_scene);
        driver_context.m_dirlight.m_diffuse_color = get_directional_light_diffuse_color(current_scene);
        driver_context.m_dirlight.m_specular_color = get_directional_light_specular_color(current_scene);
        // TODO is this correct? The conversion from vec4 to vec3 discards the w component, which is not necessarily 1
        glm::vec3 direction_cameraspace(driver_context.m_view * glm::vec4(get_directional_light_direction(current_scene), 0.0f));
        driver_context.m_dirlight.m_direction_cameraspace = direction_cameraspace;

        // Set the cubemap texture to use
        driver_context.m_gl_cubemap = 0U;
        skybox_id = get_scene_skybox(current_scene);
        if (skybox_id != ncubemap) {
            driver_context.m_gl_cubemap = get_cubemap_gl_cubemap_id(skybox_id);
        }

        // Set point light data
        for (point_light_id pl = get_first_point_light(); pl != npoint_light; pl = get_next_point_light(pl)) {
            point_light_data pl_data;
            // TODO is this correct? The conversion from vec4 to vec3 discards the w component, which is not necessarily 1
            glm::vec3 position_cameraspace(driver_context.m_view * glm::vec4(get_point_light_position(pl), 0.0f));
            pl_data.m_position_cameraspace = position_cameraspace;
            pl_data.m_ambient_color = get_point_light_ambient_color(pl);
            pl_data.m_diffuse_color = get_point_light_diffuse_color(pl);
            pl_data.m_specular_color = get_point_light_specular_color(pl);
            pl_data.m_constant_attenuation = get_point_light_constant_attenuation(pl);
            pl_data.m_linear_attenuation = get_point_light_linear_attenuation(pl);
            pl_data.m_quadratic_attenuation = get_point_light_quadratic_attenuation(pl);
            driver_context.m_point_lights.push_back(pl_data);
        }

        // Set the depth func to use
        driver_context.m_depth_func = depth_func::less;
    }

    void get_node_properties(node_id n)
    {
        driver_context.m_node.m_texture = get_material_texture_id(get_node_material(n));
        mesh_id mid = get_node_mesh(n);
        driver_context.m_node.m_position_buffer = get_mesh_position_buffer_id(mid);
        driver_context.m_node.m_texture_coords_buffer = get_mesh_uv_buffer_id(mid);
        driver_context.m_node.m_normal_buffer = get_mesh_normal_buffer_id(mid);
        driver_context.m_node.m_index_buffer = get_mesh_index_buffer_id(mid);
        driver_context.m_node.m_num_indices = get_mesh_indices(mid).size();
        driver_context.m_node.m_material.m_diffuse_color = get_material_diffuse_color(get_node_material(n));
        driver_context.m_node.m_material.m_specular_color = get_material_specular_color(get_node_material(n));
        driver_context.m_node.m_material.m_smoothness = get_material_smoothness(get_node_material(n));
        driver_context.m_node.m_material.m_reflectivity = get_material_reflectivity(get_node_material(n));
        driver_context.m_node.m_material.m_translucency = get_material_translucency(get_node_material(n));
        driver_context.m_node.m_material.m_refractive_index = get_material_refractive_index(get_node_material(n));
        glm::mat4 local_transform;
        glm::mat4 accum_transform;
        get_node_transform(n, &local_transform, &accum_transform);
        driver_context.m_node.m_model = accum_transform;
    }

    void render_phong_nodes()
    {
        // Render nodes that are neither reflective nor tranlucent with the phong model
        driver_context.m_program = phong_programs[0].get();
        for (auto n : nodes_to_render) {
            mat_id mat = get_node_material(n);
            float reflectivity = get_material_reflectivity(mat);
            float translucency = get_material_translucency(mat);
            if (get_node_material(n) != nmat && reflectivity == 0.0f && translucency == 0.0f) {
                driver_context.m_node = gl_node_context();
                get_node_properties(n);
                driver.draw(driver_context);
            }
        }
    }

    void render_environment_mapping_nodes()
    {
        // Render reflective or translucent nodes
        driver_context.m_program = environment_mapping_programs[0].get();
        for (auto n : nodes_to_render) {
            mat_id mat = get_node_material(n);
            float reflectivity = get_material_reflectivity(mat);
            float translucency = get_material_translucency(mat);
            if (reflectivity > 0.0f || translucency > 0.0f) {
                driver_context.m_node = gl_node_context();
                get_node_properties(n);
                driver.draw(driver_context);
            }
        }
    }

    void render_skybox()
    {
        // Render the skybox
        if (skybox_id != ncubemap) {
            driver_context.m_program = skybox_programs[0].get();
            driver_context.m_node = gl_node_context();
            // Change depth function so depth test passes when values are equal to depth buffer's content
            driver_context.m_depth_func = depth_func::lequal;
            // Remove translation from the view matrix
            driver_context.m_view = glm::mat4(glm::mat3(driver_context.m_view));
            driver_context.m_node.m_position_buffer = gl_cubemap_position_buffers[0].get();
            driver_context.m_node.m_index_buffer = gl_cubemap_index_buffers[0].get();
            driver_context.m_node.m_num_indices = 36U;
            driver.draw(driver_context);
        }
    }

    void render()
    {
        driver.initialize_frame();
        // For each scene in the view
        for (scene_id s = get_first_scene(); s != nscene && is_scene_enabled(s); s = get_next_scene(s)) {
            // Convert tree into list and filter out non-enabled nodes
            current_scene = s;
            nodes_to_render = get_descendant_nodes(get_scene_root_node(s));
            driver_context = gl_driver_context();
            get_scene_properties();
            render_phong_nodes();
            render_environment_mapping_nodes();
            render_skybox();
        }
    }
} // namespace cgs
