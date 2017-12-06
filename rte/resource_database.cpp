#include "resource_database.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>
#include <queue>

namespace rte
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal declarations
        //---------------------------------------------------------------------------------------------
        struct material
        {
            material() :
                m_color_diffuse({1.0f, 1.0f, 1.0f}),
                m_color_spec({1.0f, 1.0f, 1.0f}),
                m_smoothness(1.0f),
                m_texture_path(),
                m_reflectivity(0.0f),
                m_translucency(0.0f),
                m_refractive_index(1.0f),
                m_texture_id(0U),
                m_user_id(nuser_id),
                m_name(),
                m_used(true) {}

            glm::vec3     m_color_diffuse;
            glm::vec3     m_color_spec;
            float         m_smoothness;
            std::string   m_texture_path;
            float         m_reflectivity;
            float         m_translucency;
            float         m_refractive_index;
            gl_texture_id m_texture_id;
            user_id       m_user_id;
            std::string   m_name;
            bool          m_used;
        };

        typedef std::vector<material> material_vector;

        struct mesh
        {
            mesh() :
                m_vertices(),
                m_texture_coords(),
                m_normals(),
                m_indices(),
                m_position_buffer_id(0U),
                m_uv_buffer_id(0U),
                m_normal_buffer_id(0U),
                m_index_buffer_id(0U),
                m_user_id(nuser_id),
                m_name(),
                m_used(true) {}

            std::vector<glm::vec3> m_vertices;            //!< vertex coordinates
            std::vector<glm::vec2> m_texture_coords;      //!< texture coordinates for each vertex
            std::vector<glm::vec3> m_normals;             //!< normals of the mesh
            std::vector<vindex>    m_indices;             //!< faces, as a sequence of indexes over the logical vertex array
            gl_buffer_id           m_position_buffer_id;  //!< id of the position buffer in the graphics API
            gl_buffer_id           m_uv_buffer_id;        //!< id of the uv buffer in the graphics API
            gl_buffer_id           m_normal_buffer_id;    //!< id of the normal buffer in the graphics API
            gl_buffer_id           m_index_buffer_id;     //!< id of the index buffer in the graphics API
            user_id                m_user_id;             //!< user id of this mesh
            std::string            m_name;                //!< name of this mesh
            bool                   m_used;                //!< is this entry in the vector used?
        };

        typedef std::vector<mesh> mesh_vector;

        struct resource
        {
            resource() :
                m_mesh(nmesh),
                m_material(nmat),
                m_local_transform(1.0f),
                m_first_child(nresource),
                m_next_sibling(nresource),
                m_user_id(nuser_id),
                m_used(true) {}

            mesh_id     m_mesh;               //!< mesh contained in this resource
            mat_id      m_material;           //!< material of this resource
            glm::mat4   m_local_transform;    //!< resource transform relative to the parent's reference frame
            resource_id m_first_child;        //!< first child resource of this resource
            resource_id m_next_sibling;       //!< next sibling resource of this resource
            user_id     m_user_id;            //!< user id of this resource
            std::string m_name;               //!< name of this resource
            bool        m_used;               //!< is this entry in the vector used?
        };

        typedef std::vector<resource> resource_vector;

        struct cubemap
        {
            cubemap() :
                m_faces(),
                m_gl_cubemap_id(0U),
                m_user_id(nuser_id),
                m_used(true) {}

            std::vector<std::string> m_faces;          //!< paths to image files containing the six faces of the cubemap
            gl_cubemap_id            m_gl_cubemap_id;  //!< id of this cubemap in the graphics API
            user_id                  m_user_id;        //!< user id of this cubemap
            std::string              m_name;           //!< name of this resource
            bool                     m_used;           //!< is this entry in the vector used?
        };

        typedef std::vector<cubemap> cubemap_vector;


        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        material_vector materials;                  //!< collection of all the materials
        mesh_vector meshes;                         //!< collection of all the meshes
        resource_vector resources;                  //!< collection of all the resources
        cubemap_vector cubemaps;                    //!< collection of all the cubemaps
    } // anonymous namespace

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------------------------
    void resource_database_init()
    {
        materials.clear();
        materials.reserve(100);
        meshes.clear();
        meshes.reserve(100);
        resources.clear();
        resources.reserve(2048);
        cubemaps.clear();
        cubemaps.reserve(10);
    }

    mat_id new_material()
    {
        mat_id m = std::find_if(materials.begin(), materials.end(), [](const material& m) { return !m.m_used; }) - materials.begin();
        if (m == materials.size()) {
            materials.push_back(material{});
        } else {
            materials[m] = material{};
        }

        return m;
    }

    void delete_material(mat_id m)
    {
        if (m < materials.size() && materials[m].m_used) {
            materials[m].m_used = false; // soft removal
        }
    }

    void set_material_diffuse_color(mat_id m, glm::vec3 diffuse_color)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_diffuse_color error: invalid material id");
        }

        materials[m].m_color_diffuse = diffuse_color;
    }

    void set_material_specular_color(mat_id m, glm::vec3 specular_color)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_specular_color error: invalid material id");
        }

        materials[m].m_color_spec = specular_color;
    }

    void set_material_smoothness(mat_id m, float smoothness)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_smoothness error: invalid material id");
        }

        materials[m].m_smoothness = smoothness;
    }

    void set_material_texture_path(mat_id m, const std::string& texture_path)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_texture_path error: invalid material id");
        }

        materials[m].m_texture_path = texture_path;
    }

    void set_material_reflectivity(mat_id m, float reflectivity)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_reflectivity error: invalid material id");
        }

        materials[m].m_reflectivity = reflectivity;
    }

    void set_material_translucency(mat_id m, float translucency)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_translucency error: invalid material id");
        }

        materials[m].m_translucency = translucency;
    }

    void set_material_refractive_index(mat_id m, float refractive_index)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_refractive_index error: invalid material id");
        }

        materials[m].m_refractive_index = refractive_index;
    }

    void set_material_texture_id(mat_id m, gl_texture_id texture_id)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_texture_id error: invalid material id");
        }

        materials[m].m_texture_id = texture_id;
    }

    void set_material_user_id(mat_id m, user_id uid)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_user_id error: invalid material id");
        }

        materials[m].m_user_id = uid;
    }

    void set_material_name(mat_id m, const std::string& name)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("set_material_name error: invalid material id");
        }

        materials[m].m_name = name;
    }

    glm::vec3 get_material_diffuse_color(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_diffuse_color error: invalid material id");
        }

        return materials[m].m_color_diffuse;    
    }

    glm::vec3 get_material_specular_color(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_specular_color error: invalid material id");
        }

        return materials[m].m_color_spec;
    }

    float get_material_smoothness(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_smoothness error: invalid material id");
        }

        return materials[m].m_smoothness;
    }

    std::string get_material_texture_path(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_texture_path error: invalid material id");
        }

        return materials[m].m_texture_path;
    }

    float get_material_reflectivity(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_reflectivity error: invalid material id");
        }

        return materials[m].m_reflectivity;
    }


    float get_material_translucency(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_translucency error: invalid material id");
        }
 
        return materials[m].m_translucency;
    }

    float get_material_refractive_index(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_refractive_index error: invalid material id");
        }

        return materials[m].m_refractive_index;
    }

    gl_texture_id get_material_texture_id(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_texture_id error: invalid material id");
        }

        return materials[m].m_texture_id;
    }

    user_id get_material_user_id(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_user_id error: invalid material id");
        }

        return materials[m].m_user_id;
    }

    std::string get_material_name(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            throw std::logic_error("get_material_name error: invalid material id");
        }

        return materials[m].m_name;
    }

    mat_id get_first_material()
    {
        auto it = std::find_if(materials.begin(), materials.end(), [](const material& m) { return m.m_used; });
        return (it != materials.end() ? it - materials.begin() : nmat);
    }

    mat_id get_next_material(mat_id m)
    {
        auto it = std::find_if(materials.begin() + m + 1, materials.end(), [](const material& m) { return m.m_used; });
        return (it != materials.end() ? it - materials.begin() : nmat);
    }

    unique_material make_material()
    {
        return unique_material(new_material());
    }

    void log_materials()
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: materials begin");
        if (materials.size()) {
            for (unsigned int m = 0; m < materials.size() && materials[m].m_used; m++) {
                glm::vec3 diffuse_color = get_material_diffuse_color(m);
                glm::vec3 specular_color = get_material_specular_color(m);
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed;
                oss << "    id: " << m;
                oss << ", user id: " << get_material_user_id(m);
                oss << ", name: " << get_material_name(m);
                oss << ", diffuse color: ";
                print_sequence((float*) &diffuse_color, 3U, oss);
                oss << ", color specular: ";
                print_sequence((float*) &specular_color, 3U, oss);
                oss << ", smoothness: " << get_material_smoothness(m);
                oss << ", texture path: " << get_material_texture_path(m);
                oss << ", reflectivity: " << get_material_reflectivity(m);
                oss << ", translucency: " << get_material_translucency(m);
                oss << ", refractive_index: " << get_material_refractive_index(m);
                log(LOG_LEVEL_DEBUG, oss.str().c_str());
            }
        } else {
            log(LOG_LEVEL_DEBUG, "    no materials found");
        }
        log(LOG_LEVEL_DEBUG, "resource_database: materials end");
    }

    mesh_id new_mesh()
    {
        mesh_id m = std::find_if(meshes.begin(), meshes.end(), [](const mesh& m) { return !m.m_used; }) - meshes.begin();
        if (m == meshes.size()) {
            meshes.push_back(mesh{});
        } else {
            meshes[m] = mesh{};
        }

        return m;
    }

    void delete_mesh(mesh_id m)
    {
        if (m < meshes.size() && meshes[m].m_used) {
            meshes[m].m_used = false; // soft removal;
        }
    }

    void set_mesh_vertices(mesh_id m, const std::vector<glm::vec3>& vertices)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_vertices error: invalid arguments");
        }

        meshes[m].m_vertices = vertices;
    }

    void set_mesh_texture_coords(mesh_id m, const std::vector<glm::vec2>& texture_coords)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_texture_coords error: invalid arguments");
        }

        meshes[m].m_texture_coords = texture_coords;
    }

    void set_mesh_normals(mesh_id m, const std::vector<glm::vec3>& normals)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_normals error: invalid arguments");
        }

        meshes[m].m_normals = normals;
    }

    void set_mesh_indices(mesh_id m, const std::vector<vindex>& indices)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_indices error: invalid arguments");
        }

        meshes[m].m_indices = indices;
    }

    void set_mesh_position_buffer_id(mesh_id m, gl_buffer_id id)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_position_buffer_id error: invalid arguments");
        }

        meshes[m].m_position_buffer_id = id;
    }

    void set_mesh_uv_buffer_id(mesh_id m, gl_buffer_id id)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_uv_buffer_id error: invalid arguments");
        }

        meshes[m].m_uv_buffer_id = id;
    }

    void set_mesh_normal_buffer_id(mesh_id m, gl_buffer_id id)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_normal_buffer_id error: invalid arguments");
        }

        meshes[m].m_normal_buffer_id = id;
    }

    void set_mesh_index_buffer_id(mesh_id m, gl_buffer_id id)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_index_buffer_id error: invalid arguments");
        }

        meshes[m].m_index_buffer_id = id;
    }

    void set_mesh_user_id(mesh_id m, user_id uid)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_user_id error: invalid arguments");
        }

        meshes[m].m_user_id = uid;
    }

    void set_mesh_name(mesh_id m, const std::string& name)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("set_mesh_name error: invalid arguments");
        }

        meshes[m].m_name = name;
    }

    std::vector<glm::vec3> get_mesh_vertices(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_vertices error: invalid arguments");
        }

        return meshes[m].m_vertices;
    }

    std::vector<glm::vec2> get_mesh_texture_coords(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_texture_coords error: invalid arguments");
        }

        return meshes[m].m_texture_coords;
    }

    std::vector<glm::vec3> get_mesh_normals(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_normals error: invalid arguments");
        }

        return meshes[m].m_normals;
    }

    std::vector<vindex> get_mesh_indices(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_indices error: invalid arguments");
        }

        return meshes[m].m_indices;
    }

    gl_buffer_id get_mesh_position_buffer_id(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_position_buffer_id error: invalid arguments");
        }

        return meshes[m].m_position_buffer_id;
    }

    gl_buffer_id get_mesh_uv_buffer_id(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_uv_buffer_id error: invalid arguments");
        }

        return meshes[m].m_uv_buffer_id;
    }

    gl_buffer_id get_mesh_normal_buffer_id(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_normal_buffer_id error: invalid arguments");
        }

        return meshes[m].m_normal_buffer_id;
    }

    gl_buffer_id get_mesh_index_buffer_id(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_index_buffer_id error: invalid arguments");
        }

        return meshes[m].m_index_buffer_id;
    }

    user_id get_mesh_user_id(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_user_id error: invalid arguments");
        }

        return meshes[m].m_user_id;
    }

    std::string get_mesh_name(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            throw std::logic_error("get_mesh_name error: invalid arguments");
        }

        return meshes[m].m_name;
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

    void log_mesh(mesh_id m)
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
        if (indices.size()) {
            preview_sequence(&indices[0], indices.size(), oss);        
        }
        log(LOG_LEVEL_DEBUG, oss.str().c_str());

        oss.str("");
        oss << "        normals: ";
        if (normals.size()) {
            preview_sequence(&normals[0][0], 3 * normals.size(), oss);
        }
        log(LOG_LEVEL_DEBUG, oss.str().c_str());
    }

    void log_meshes()
    {
        log(LOG_LEVEL_DEBUG, "---------------------------------------------------------------------------------------------------");
        log(LOG_LEVEL_DEBUG, "resource_database: meshes begin");
        if (meshes.size()) {
            for (unsigned int m = 0; m < meshes.size() && meshes[m].m_used; m++) {
                log_mesh(m);
            }
        } else {
            log(LOG_LEVEL_DEBUG, "    no meshes found");
        }

        log(LOG_LEVEL_DEBUG, "resource_database: meshes end");
    }

    mesh_id get_first_mesh()
    {
        auto it = std::find_if(meshes.begin(), meshes.end(), [](const mesh& m) { return m.m_used; });
        return (it != meshes.end() ? it - meshes.begin() : nmesh);
    }

    mesh_id get_next_mesh(mesh_id m)
    {
        auto it = std::find_if(meshes.begin() + m + 1, meshes.end(), [](const mesh& m) { return m.m_used; });
        return (it != meshes.end() ? it - meshes.begin() : nmesh);
    }

    unique_mesh make_mesh()
    {
        return unique_mesh(new_mesh());
    }

    resource_id new_resource()
    {
        // Allocate a vector entry for the new resource
        resource_id new_res = std::find_if(resources.begin(), resources.end(), [](const resource& r) { return !r.m_used; }) - resources.begin();
        if (new_res == resources.size()) {
            resources.push_back(resource{});
        } else {
            resources[new_res] = resource{};
        }

        return new_res;
    }

    resource_id new_resource(resource_id p)
    {
        if (!(p < resources.size() && resources[p].m_used)) {
            log(LOG_LEVEL_ERROR, "new_resource error: invalid resource id for parent"); return nresource;
        }

        // Allocate a vector entry for the new resource
        resource_id new_res = new_resource();

        // Link the new resource as last child of p
        resource_id r = resources[p].m_first_child;
        resource_id next = nresource;
        while (r != nresource && ((next = resources[r].m_next_sibling) != nresource)) {
            r = next;
        }
        (r == nresource? resources[p].m_first_child : resources[r].m_next_sibling) = new_res;

        return new_res;
    }

    void delete_resource(resource_id r)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            log(LOG_LEVEL_ERROR, "delete_resource error: invalid parameters");
        }

        // Remove the reference to the resource in its parent or previous sibling
        auto rit = resources.begin();
        while (rit != resources.end()) {
            if (rit->m_used && rit->m_first_child == r) {
                rit->m_first_child = resources[r].m_next_sibling;
                break;
            } else if (rit->m_used && rit->m_next_sibling == r) {
                rit->m_next_sibling = resources[r].m_next_sibling;
                break;
            }
            rit++;
        }

        // Set all members to default values
        resources[r] = resource{};
        // Mark the vector entry as not used
        resources[r].m_used = false; // soft removal
    }

    void set_resource_local_transform(resource_id r, const glm::mat4& local_transform)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("set_resource_local_transform error: invalid arguments");
        }

        resources[r].m_local_transform = local_transform;
    }

    glm::mat4 get_resource_local_transform(resource_id r)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("get_resource_local_transform error: invalid arguments");
        }

        return resources[r].m_local_transform;
    }

    void set_resource_mesh(resource_id r, mesh_id m)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("set_resource_mesh error: invalid arguments");
        }

        resources[r].m_mesh = m;
    }

    mesh_id get_resource_mesh(resource_id r)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("get_resource_mesh error: invalid arguments");
        }

        return resources[r].m_mesh;
    }

    void set_resource_material(resource_id r, mat_id mat)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("set_resource_material error: invalid arguments");
        }

        resources[r].m_material = mat;
    }

    mat_id get_resource_material(resource_id r)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("get_resource_material error: invalid arguments");
        }

        return resources[r].m_material;
    }

    void set_resource_user_id(resource_id r, user_id uid)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("set_resource_user_id error: invalid arguments");
        }

        resources[r].m_user_id = uid;
    }

    user_id get_resource_user_id(resource_id r)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("get_resource_user_id error: invalid arguments");
        }

        return resources[r].m_user_id;
    }

    void set_resource_name(resource_id r, const std::string& name)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("set_resource_name error: invalid arguments");
        }

        resources[r].m_name = name;
    }

    std::string get_resource_name(resource_id r)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("get_resource_name error: invalid arguments");
        }

        return resources[r].m_name;
    }

    resource_id get_first_child_resource(resource_id r)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("get_first_child_resource error: invalid arguments");
        }

        return resources[r].m_first_child;
    }

    resource_id get_next_sibling_resource(resource_id r)
    {
        if (!(r < resources.size() && resources[r].m_used)) {
            throw std::logic_error("get_next_sibling_resource error: invalid arguments");
        }

        return resources[r].m_next_sibling;
    }

    unique_resource make_resource()
    {
        return unique_resource(new_resource());
    }

    unique_resource make_resource(resource_id p)
    {
        return unique_resource(new_resource(p));
    }

    cubemap_id new_cubemap()
    {
        cubemap_id c = std::find_if(cubemaps.begin(), cubemaps.end(), [](const cubemap& c) { return !c.m_used; }) - cubemaps.begin();
        if (c == cubemaps.size()) {
            cubemaps.push_back(cubemap{});
        } else {
            cubemaps[c] = cubemap{};
        }

        return c;
    }

    void delete_cubemap(cubemap_id c)
    {
        if (c < cubemaps.size() && cubemaps[c].m_used) {
            cubemaps[c].m_used = false; // soft removal
        }
    }

    void set_cubemap_faces(cubemap_id cid, const std::vector<std::string>& faces)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("set_cubemap_faces error: invalid arguments");
        }

        cubemaps[cid].m_faces = faces;
    }

    void set_cubemap_gl_cubemap_id(cubemap_id cid, gl_cubemap_id gl_id)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("set_cubemap_gl_cubemap_id error: invalid arguments");
        }

        cubemaps[cid].m_gl_cubemap_id = gl_id;
    }

    void set_cubemap_user_id(cubemap_id cid, user_id uid)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("set_cubemap_user_id error: invalid arguments");
        }

        cubemaps[cid].m_user_id = uid;
    }

    void set_cubemap_name(cubemap_id cid, const std::string& name)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("set_cubemap_name error: invalid arguments");
        }

        cubemaps[cid].m_name = name;
    }

    std::vector<std::string> get_cubemap_faces(cubemap_id cid)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("get_cubemap_faces error: invalid arguments");
        }

        return cubemaps[cid].m_faces;
    }

    gl_cubemap_id get_cubemap_gl_cubemap_id(cubemap_id cid)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("get_cubemap_gl_cubemap_id error: invalid arguments");
        }

        return cubemaps[cid].m_gl_cubemap_id;
    }

    cubemap_id get_first_cubemap()
    {
        auto it = std::find_if(cubemaps.begin(), cubemaps.end(), [](const cubemap& m) { return m.m_used; });
        return (it != cubemaps.end() ? it - cubemaps.begin() : ncubemap);
    }

    cubemap_id get_next_cubemap(cubemap_id cid)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("get_next_cubemap error: invalid arguments");
        }

        auto it = std::find_if(cubemaps.begin() + cid + 1, cubemaps.end(), [](const cubemap& c) { return c.m_used; });
        return (it != cubemaps.end() ? it - cubemaps.begin() : ncubemap);
    }

    user_id get_cubemap_user_id(cubemap_id cid)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("get_cubemap_user_id error: invalid arguments");
        }

        return cubemaps[cid].m_user_id;
    }

    std::string get_cubemap_name(cubemap_id cid)
    {
        if (!(cid < cubemaps.size() && cubemaps[cid].m_used)) {
            throw std::logic_error("get_cubemap_name error: invalid arguments");
        }

        return cubemaps[cid].m_name;
    }

    unique_cubemap make_cubemap()
    {
        return unique_cubemap(new_cubemap());
    }
} // namespace rte
