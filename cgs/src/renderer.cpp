#include "cgs/resource_database.hpp"
#include "cgs/renderer.hpp"
#include "cgs/system.hpp"
#include "cgs/log.hpp"

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
        std::vector<node_id> nodes_to_render;
        glm::vec3            camera_position_worldspace;
        gl_driver            driver;
        gl_driver_context    driver_context;
        gl_texture_id        default_texture_id = 0U;
        gl_program_id        phong_program = 0U;
        gl_program_id        environment_mapping_program = 0U;
        gl_program_id        skybox_program = 0U;
        gl_buffer_id         skybox_position_buffer = 0U;
        gl_buffer_id         skybox_index_buffer = 0U;
        cubemap_id           skybox_id = ncubemap;
        bool                 gl_driver_set = false;

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        void finalize_shaders()
        {
            driver.delete_program(phong_program);
            driver.delete_program(environment_mapping_program);
            driver.delete_program(skybox_program);
        }

        bool initialize_shaders()
        {
            // Load our shaders
            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading shaders");
            driver.new_program(program_type::phong, &phong_program);
            driver.new_program(program_type::environment_mapping, &environment_mapping_program);
            driver.new_program(program_type::skybox, &skybox_program);
            if (!(phong_program && environment_mapping_program && skybox_program)) {
                log(LOG_LEVEL_ERROR, "initialize_renderer: could not load shaders");
                finalize_shaders();
                return false;
            }
            log(LOG_LEVEL_DEBUG, "initialize_renderer: shaders loaded successfully");
            return true;
        }

        void finalize_textures()
        {
            for (mat_id mat = get_first_material(); mat != nmat; mat = get_next_material(mat)) {
                gl_texture_id texture_id = get_material_texture_id(mat);
                if (texture_id) {
                    driver.delete_texture(texture_id);
                }
            }

            driver.delete_default_texture(default_texture_id);
        }

        bool initialize_textures()
        {
            // Create a default texture to use as diffuse map on objects that don't have a texture
            driver.new_default_texture(&default_texture_id);

            // Load all textures
            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading textures");
            for (mat_id mat = get_first_material(); mat != nmat; mat = get_next_material(mat)) {
                gl_texture_id texture_id = default_texture_id;
                if (!get_material_texture_path(mat).empty()) {
                    // Load the texture into memory            
                    image img;
                    img.load(get_material_texture_path(mat));
                    if (!img.ok()) {
                        std::ostringstream oss;
                        oss << "initialize_renderer: error loading texture from " << get_material_texture_path(mat);
                        log(LOG_LEVEL_ERROR, oss.str());
                        finalize_textures();
                        return false;
                    }
                    // Load the texture into the graphics API
                    driver.new_texture(img.get_width(), img.get_height(), img.get_format(), img.get_data(), &texture_id);
                }
                set_material_texture_id(mat, texture_id);
            }
            log(LOG_LEVEL_DEBUG, "initialize_renderer: textures loaded successfully");
            return true;
        }

        void finalize_meshes()
        {
            for (mesh_id mid = get_first_mesh(); mid != nmesh; mid = get_next_mesh(mid)) {
                gl_buffer_id positions_buffer_id = get_mesh_position_buffer_id(mid);
                if (positions_buffer_id) {
                    driver.delete_buffer(positions_buffer_id);
                }
                gl_buffer_id uv_buffer_id = get_mesh_uv_buffer_id(mid);
                if (uv_buffer_id) {
                    driver.delete_buffer(uv_buffer_id);
                }
                gl_buffer_id normal_buffer_id = get_mesh_normal_buffer_id(mid);
                if (normal_buffer_id) {
                    driver.delete_buffer(normal_buffer_id);
                }
                gl_buffer_id index_buffer_id = get_mesh_index_buffer_id(mid);
                if (index_buffer_id) {
                    driver.delete_buffer(index_buffer_id);
                }
            }
        }

        bool initialize_meshes()
        {
            // Load all meshes
            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading meshes");
            for (mesh_id mid = get_first_mesh(); mid != nmesh; mid = get_next_mesh(mid)) {
                gl_buffer_id position_buffer_id = 0;
                driver.new_3d_buffer(get_mesh_vertices(mid), &position_buffer_id);
                gl_buffer_id uv_buffer_id = 0;
                driver.new_2d_buffer(get_mesh_texture_coords(mid), &uv_buffer_id);
                gl_buffer_id normal_buffer_id = 0;
                driver.new_3d_buffer(get_mesh_normals(mid), &normal_buffer_id);
                gl_buffer_id index_buffer_id = 0;
                driver.new_index_buffer(get_mesh_indices(mid), &index_buffer_id);
                if (!(position_buffer_id && uv_buffer_id && normal_buffer_id && index_buffer_id)) {
                    std::ostringstream oss;
                    oss << "initialize_renderer: error loading meshes for mesh " << mid;
                    log(LOG_LEVEL_ERROR, oss.str());
                    finalize_meshes();
                    return false;
                }
                set_mesh_position_buffer_id(mid, position_buffer_id);
                set_mesh_uv_buffer_id(mid, uv_buffer_id);
                set_mesh_normal_buffer_id(mid, normal_buffer_id);
                set_mesh_index_buffer_id(mid, index_buffer_id);
            }
            log(LOG_LEVEL_DEBUG, "initialize_renderer: meshes loaded succesfully");
            return true;
        }

        void finalize_cubemaps()
        {
            for (cubemap_id cid = get_first_cubemap(); cid != ncubemap; cid = get_next_cubemap(cid)) {
                gl_cubemap_id glcid = get_cubemap_gl_cubemap_id(cid);
                if (glcid) {
                    driver.delete_cubemap(glcid);
                }
            }            
        }

        bool initialize_cubemaps()
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
            driver.new_3d_buffer(skybox_positions, &skybox_position_buffer);

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
            driver.new_index_buffer(skybox_indices, &skybox_index_buffer);

            log(LOG_LEVEL_DEBUG, "initialize_renderer: loading cubemaps");
            for (cubemap_id cid = get_first_cubemap(); cid != ncubemap; cid = get_next_cubemap(cid)) {
                std::vector<const unsigned char*> faces_data;
                std::vector<std::unique_ptr<image>> faces;
                for (auto path : get_cubemap_faces(cid)) {
                    auto face_ptr = std::make_unique<image>();
                    face_ptr->load(path);
                    if (!face_ptr->ok()) {
                        std::ostringstream oss;
                        oss << "initialize_renderer: error loading cubemap texture from " << path;
                        log(LOG_LEVEL_ERROR, oss.str());
                        finalize_cubemaps();
                        return false;
                    }
                    faces_data.push_back(face_ptr->get_data());
                    faces.push_back(std::move(face_ptr));
                }
                if (faces.size() < 6) {
                    log(LOG_LEVEL_ERROR, "initialize_renderer: invalid cubemap data, faces unavailable");
                    finalize_cubemaps();
                    return false;
                }
                gl_cubemap_id glcid = 0U;
                driver.new_cubemap(faces[0]->get_width(), faces[0]->get_height(), faces[0]->get_format(), faces_data, &glcid);
                set_cubemap_gl_cubemap_id(cid, glcid);
            }
            log(LOG_LEVEL_DEBUG, "initialize_renderer: cubemaps loaded successfully");
            return true;
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

    bool initialize_renderer()
    {
        if (!is_context_created()) {
            log(LOG_LEVEL_ERROR, "initialize_renderer: error, trying to initialize renderer but a context hasn't been created");
            return false;
        }

        if (!gl_driver_set) {
            log(LOG_LEVEL_ERROR, "initialize_renderer: error, trying to initialize renderer but a driver hasn't been set");
            return false;   
        }

        // Initialize the graphics API
        driver.gl_driver_init();
        bool ok = initialize_shaders();
        if (!ok) {
            return false;
        }
        
        ok = initialize_textures();
        if (!ok) {
            finalize_shaders();
            return false;
        }

        ok = initialize_meshes();
        if (!ok) {
            finalize_textures();
            finalize_shaders();
            return false;
        }

        ok = initialize_cubemaps();
        if (!ok) {
            finalize_meshes();
            finalize_textures();
            finalize_shaders();
            return false;
        }

        return true;
    }

    void finalize_renderer()
    {
        finalize_cubemaps();
        finalize_meshes();
        finalize_textures();
        finalize_shaders();
    }

    void get_layer_properties(layer_id l)
    {
        // Set view and projection matrices for the layer
        get_layer_projection_transform(l, &driver_context.m_projection);
        get_layer_view_transform(l, &driver_context.m_view);

        // Set directional light properties
        driver_context.m_dirlight.m_ambient_color = get_directional_light_ambient_color(l);
        driver_context.m_dirlight.m_diffuse_color = get_directional_light_diffuse_color(l);
        driver_context.m_dirlight.m_specular_color = get_directional_light_specular_color(l);
        // TODO is this correct? The conversion from vec4 to vec3 discards the w component, which is not necessarily 1
        glm::vec3 direction_cameraspace(driver_context.m_view * glm::vec4(get_directional_light_direction(l), 0.0f));
        driver_context.m_dirlight.m_direction_cameraspace = direction_cameraspace;

        // Set the cubemap texture to use
        driver_context.m_cubemap = 0U;
        skybox_id = get_layer_skybox(l);
        if (skybox_id != ncubemap) {
            driver_context.m_cubemap = get_cubemap_gl_cubemap_id(skybox_id);
        }

        // Set point light data
        for (point_light_id pl = get_first_point_light(l); pl != npoint_light; pl = get_next_point_light(l, pl)) {
            point_light_data pl_data;
            // TODO is this correct? The conversion from vec4 to vec3 discards the w component, which is not necessarily 1
            glm::vec3 position_cameraspace(driver_context.m_view * glm::vec4(get_point_light_position(l, pl), 0.0f));
            pl_data.m_position_cameraspace = position_cameraspace;
            pl_data.m_ambient_color = get_point_light_ambient_color(l, pl);
            pl_data.m_diffuse_color = get_point_light_diffuse_color(l, pl);
            pl_data.m_specular_color = get_point_light_specular_color(l, pl);
            pl_data.m_constant_attenuation = get_point_light_constant_attenuation(l, pl);
            pl_data.m_linear_attenuation = get_point_light_linear_attenuation(l, pl);
            pl_data.m_quadratic_attenuation = get_point_light_quadratic_attenuation(l, pl);
            driver_context.m_point_lights.push_back(pl_data);
        }

        // Set the depth func to use
        driver_context.m_depth_func = depth_func::less;
    }

    void get_node_properties(layer_id l, node_id n)
    {
        driver_context.m_node.m_texture = get_material_texture_id(get_node_material(l, n));
        mesh_id mid = get_node_mesh(l, n);
        driver_context.m_node.m_position_buffer = get_mesh_position_buffer_id(mid);
        driver_context.m_node.m_texture_coords_buffer = get_mesh_uv_buffer_id(mid);
        driver_context.m_node.m_normal_buffer = get_mesh_normal_buffer_id(mid);
        driver_context.m_node.m_index_buffer = get_mesh_index_buffer_id(mid);
        driver_context.m_node.m_num_indices = get_mesh_indices(mid).size();
        driver_context.m_node.m_material.m_diffuse_color = get_material_diffuse_color(get_node_material(l, n));
        driver_context.m_node.m_material.m_specular_color = get_material_specular_color(get_node_material(l, n));
        driver_context.m_node.m_material.m_smoothness = get_material_smoothness(get_node_material(l, n));
        driver_context.m_node.m_material.m_reflectivity = get_material_reflectivity(get_node_material(l, n));
        driver_context.m_node.m_material.m_translucency = get_material_translucency(get_node_material(l, n));
        driver_context.m_node.m_material.m_refractive_index = get_material_refractive_index(get_node_material(l, n));
        glm::mat4 local_transform;
        glm::mat4 accum_transform;
        get_node_transform(l, n, &local_transform, &accum_transform);
        driver_context.m_node.m_model = accum_transform;
    }

    void render(view_id v)
    {
        if (!is_view_enabled(v)) {
            log(LOG_LEVEL_ERROR, "render_old: view is not enabled"); return;
        }

        driver.initialize_frame();

        struct node_context{ node_id nid; };

        // For each layer in the view
        for (layer_id l = get_first_layer(v); l != nlayer && is_layer_enabled(l); l = get_next_layer(l)) {
            // Convert tree into list and filter out non-enabled nodes
            nodes_to_render = get_descendant_nodes(l, root_node);

            driver_context = gl_driver_context();

            get_layer_properties(l);

            // Render nodes that are neither reflective nor tranlucent with the phong model
            driver_context.m_program = phong_program;
            for (auto n : nodes_to_render) {
                mat_id mat = get_node_material(l, n);
                float reflectivity = get_material_reflectivity(mat);
                float translucency = get_material_translucency(mat);
                if (get_node_material(l, n) != nmat && reflectivity == 0.0f && translucency == 0.0f) {
                    driver_context.m_node = gl_node_context();
                    get_node_properties(l, n);
                    driver.draw(driver_context);
                }
            }

            // Render reflective or translucent nodes
            driver_context.m_program = environment_mapping_program;
            for (auto n : nodes_to_render) {
                mat_id mat = get_node_material(l, n);
                float reflectivity = get_material_reflectivity(mat);
                float translucency = get_material_translucency(mat);
                if (reflectivity > 0.0f || translucency > 0.0f) {
                    driver_context.m_node = gl_node_context();
                    get_node_properties(l, n);
                    driver.draw(driver_context);
                }
            }

            // Render the skybox
            if (skybox_id != ncubemap) {
                driver_context.m_program = skybox_program;
                driver_context.m_node = gl_node_context();
                // Change depth function so depth test passes when values are equal to depth buffer's content
                driver_context.m_depth_func = depth_func::lequal;
                // Remove translation from the view matrix
                driver_context.m_view = glm::mat4(glm::mat3(driver_context.m_view));
                driver_context.m_node.m_position_buffer = skybox_position_buffer;
                driver_context.m_node.m_index_buffer = skybox_index_buffer;
                driver_context.m_node.m_num_indices = 36U;
                driver.draw(driver_context);
            }
        }
    }
} // namespace cgs
