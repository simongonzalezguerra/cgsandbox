#include "cgs/resource_database.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cgs/log.hpp"

#include <algorithm>
#include <vector>
#include <queue>

namespace cgs
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
                m_used(true) {}

            glm::vec3     m_color_diffuse;
            glm::vec3     m_color_spec;
            float         m_smoothness;
            std::string   m_texture_path;
            float         m_reflectivity;
            float         m_translucency;
            float         m_refractive_index;
            gl_texture_id m_texture_id;
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
                m_used(true) {}

            std::vector<glm::vec3> m_vertices;            //!< vertex coordinates
            std::vector<glm::vec2> m_texture_coords;      //!< texture coordinates for each vertex
            std::vector<glm::vec3> m_normals;             //!< normals of the mesh
            std::vector<vindex>    m_indices;             //!< faces, as a sequence of indexes over the logical vertex array
            gl_buffer_id           m_position_buffer_id;  //!< id of the position buffer in the graphics API
            gl_buffer_id           m_uv_buffer_id;        //!< id of the uv buffer in the graphics API
            gl_buffer_id           m_normal_buffer_id;    //!< id of the normal buffer in the graphics API
            gl_buffer_id           m_index_buffer_id;     //!< id of the index buffer in the graphics API
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
                m_used(true) {}

            mesh_id     m_mesh;               //!< mesh contained in this resource
            mat_id      m_material;           //!< material of this resource
            glm::mat4   m_local_transform;    //!< resource transform relative to the parent's reference frame
            resource_id m_first_child;        //!< first child resource of this resource
            resource_id m_next_sibling;       //!< next sibling resource of this resource
            bool        m_used;               //!< is this entry in the vector used?
        };

        typedef std::vector<resource> resource_vector;

        struct cubemap
        {
            cubemap() : mfaces() {}

            std::vector<std::string> mfaces;          //!< paths to image files containing the six faces of the cubemap
            gl_cubemap_id            m_gl_cubemap_id;  //!< id of this cubemap in the graphics API
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
        resources.push_back(resource{});
        cubemaps.clear();
        cubemaps.reserve(10);
    }

    mat_id add_material()
    {
        mat_id m = std::find_if(materials.begin(), materials.end(), [](const material& m) { return !m.m_used; }) - materials.begin();
        if (m == materials.size()) {
            materials.push_back(material{});
        } else {
            materials[m] = material{};
        }

        return m;
    }

    void remove_material(mat_id m)
    {
        if (m < materials.size() && materials[m].m_used) {
            materials[m].m_used = false; // soft removal
        }
    }

    void set_material_diffuse_color(mat_id m, glm::vec3 diffuse_color)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            log(LOG_LEVEL_ERROR, "set_material_diffuse_color error: invalid material id"); return;
        }

        materials[m].m_color_diffuse = diffuse_color;
    }

    void set_material_specular_color(mat_id m, glm::vec3 specular_color)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            log(LOG_LEVEL_ERROR, "set_material_specular_color error: invalid material id"); return;
        }

        materials[m].m_color_spec = specular_color;
    }

    void set_material_smoothness(mat_id m, float smoothness)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            log(LOG_LEVEL_ERROR, "set_material_smoothness error: invalid material id"); return;
        }

        materials[m].m_smoothness = smoothness;
    }

    void set_material_texture_path(mat_id m, const std::string& texture_path)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            log(LOG_LEVEL_ERROR, "set_material_texture_path error: invalid material id"); return;
        }

        materials[m].m_texture_path = texture_path;
    }

    void set_material_reflectivity(mat_id m, float reflectivity)
    {
        if (m < materials.size() && materials[m].m_used) {
            materials[m].m_reflectivity = reflectivity;
        }
    }

    void set_material_translucency(mat_id m, float translucency)
    {
        if (m < materials.size() && materials[m].m_used) {
            materials[m].m_translucency = translucency;
        }
    }

    void set_material_refractive_index(mat_id m, float refractive_index)
    {
        if (m < materials.size() && materials[m].m_used) {
            materials[m].m_refractive_index = refractive_index;
        }
    }

    void set_material_texture_id(mat_id m, gl_texture_id texture_id)
    {
        if (m < materials.size() && materials[m].m_used) {
            materials[m].m_texture_id = texture_id;
        }
    }

    glm::vec3 get_material_diffuse_color(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            log(LOG_LEVEL_ERROR, "get_material_diffuse_color error: invalid material id"); return glm::vec3{1.0f};
        }

        return materials[m].m_color_diffuse;    
    }

    glm::vec3 get_material_specular_color(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            log(LOG_LEVEL_ERROR, "get_material_specular_color error: invalid material id"); return glm::vec3{1.0f};
        }

        return materials[m].m_color_spec;
    }

    float get_material_smoothness(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            log(LOG_LEVEL_ERROR, "get_material_smoothness error: invalid material id"); return 0.0f;
        }

        return materials[m].m_smoothness;
    }

    std::string get_material_texture_path(mat_id m)
    {
        if (!(m < materials.size() && materials[m].m_used)) {
            log(LOG_LEVEL_ERROR, "get_material_texture_path error: invalid material id"); return "";
        }

        return materials[m].m_texture_path;
    }

    float get_material_reflectivity(mat_id m)
    {
        return (m < materials.size() && materials[m].m_used)? materials[m].m_reflectivity : 0.0f;
    }


    float get_material_translucency(mat_id m)
    {
        return (m < materials.size() && materials[m].m_used)? materials[m].m_translucency : 0.0f;
    }

    float get_material_refractive_index(mat_id m)
    {
        return (m < materials.size() && materials[m].m_used)? materials[m].m_refractive_index : 0.0f;
    }

    gl_texture_id get_material_texture_id(mat_id m)
    {
        return (m < materials.size() && materials[m].m_used)? materials[m].m_texture_id : 0U;
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
        return unique_material(add_material());
    }

    mesh_id add_mesh()
    {
        mesh_id m = std::find_if(meshes.begin(), meshes.end(), [](const mesh& m) { return !m.m_used; }) - meshes.begin();
        if (m == meshes.size()) {
            meshes.push_back(mesh{});
        } else {
            meshes[m] = mesh{};
        }

        return m;
    }

    void remove_mesh(mesh_id m)
    {
        if (m < meshes.size() && meshes[m].m_used) {
            meshes[m].m_used = false; // soft removal;
        }
    }

    void set_mesh_vertices(mesh_id m, const std::vector<glm::vec3>& vertices)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            log(LOG_LEVEL_ERROR, "set_mesh_vertices error: invalid arguments"); return;
        }

        meshes[m].m_vertices = vertices;
    }

    void set_mesh_texture_coords(mesh_id m, const std::vector<glm::vec2>& texture_coords)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            log(LOG_LEVEL_ERROR, "set_mesh_texture_coords error: invalid arguments"); return;
        }

        meshes[m].m_texture_coords = texture_coords;
    }

    void set_mesh_normals(mesh_id m, const std::vector<glm::vec3>& normals)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            log(LOG_LEVEL_ERROR, "set_mesh_normals error: invalid arguments"); return;
        }

        meshes[m].m_normals = normals;
    }

    void set_mesh_indices(mesh_id m, const std::vector<vindex>& indices)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            log(LOG_LEVEL_ERROR, "set_mesh_indices error: invalid arguments"); return;
        }

        meshes[m].m_indices = indices;
    }

    void set_mesh_position_buffer_id(mesh_id m, gl_buffer_id id)
    {
        if (m < meshes.size() && meshes[m].m_used) {
            meshes[m].m_position_buffer_id = id;
        }
    }

    void set_mesh_uv_buffer_id(mesh_id m, gl_buffer_id id)
    {
        if (m < meshes.size() && meshes[m].m_used) {
            meshes[m].m_uv_buffer_id = id;
        }
    }

    void set_mesh_normal_buffer_id(mesh_id m, gl_buffer_id id)
    {
        if (m < meshes.size() && meshes[m].m_used) {
            meshes[m].m_normal_buffer_id = id;
        }
    }

    void set_mesh_index_buffer_id(mesh_id m, gl_buffer_id id)
    {
        if (m < meshes.size() && meshes[m].m_used) {
            meshes[m].m_index_buffer_id = id;
        }
    }

    std::vector<glm::vec3> get_mesh_vertices(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            log(LOG_LEVEL_ERROR, "get_mesh_vertices error: invalid arguments"); return std::vector<glm::vec3>();
        }

        return meshes[m].m_vertices;
    }

    std::vector<glm::vec2> get_mesh_texture_coords(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            log(LOG_LEVEL_ERROR, "get_mesh_texture_coords error: invalid arguments"); return std::vector<glm::vec2>();
        }

        return meshes[m].m_texture_coords;
    }

    std::vector<glm::vec3> get_mesh_normals(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            log(LOG_LEVEL_ERROR, "get_mesh_normals error: invalid arguments"); return std::vector<glm::vec3>();
        }

        return meshes[m].m_normals;
    }

    std::vector<vindex> get_mesh_indices(mesh_id m)
    {
        if (!(m < meshes.size() && meshes[m].m_used)) {
            log(LOG_LEVEL_ERROR, "get_mesh_indices error: invalid arguments"); return std::vector<vindex>();
        }

        return meshes[m].m_indices;
    }

    gl_buffer_id get_mesh_position_buffer_id(mesh_id m)
    {
        return ((m < meshes.size() && meshes[m].m_used) ? meshes[m].m_position_buffer_id : 0U);
    }

    gl_buffer_id get_mesh_uv_buffer_id(mesh_id m)
    {
        return ((m < meshes.size() && meshes[m].m_used)? meshes[m].m_uv_buffer_id : 0U);
    }

    gl_buffer_id get_mesh_normal_buffer_id(mesh_id m)
    {
        return ((m < meshes.size() && meshes[m].m_used)? meshes[m].m_normal_buffer_id : 0U);
    }

    gl_buffer_id get_mesh_index_buffer_id(mesh_id m)
    {
        return ((m < meshes.size() && meshes[m].m_used)? meshes[m].m_index_buffer_id : 0U);
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
        return unique_mesh(add_mesh());
    }

    resource_id add_resource(resource_id p)
    {
        if (!(p < resources.size())) {
            log(LOG_LEVEL_ERROR, "add_resource error: invalid resource id for parent"); return nresource;
        }

        // Create a new resource and link it as last child of p
        resources.push_back(resource{});
        resource_id r = resources[p].m_first_child;
        resource_id next = nresource;
        while (r != nresource && ((next = resources[r].m_next_sibling) != nresource)) {
            r = next;
        }
        (r == nresource? resources[p].m_first_child : resources[r].m_next_sibling) = resources.size() - 1;

        return resources.size() - 1;
    }

    void remove_resource(resource_id r)
    {
        if (!(r < resources.size() && r != root_resource && resources[r].m_used)) {
            throw std::logic_error("remove_resource error: invalid parameters");
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

        // Mark the vector entry as not used
        resources[r].m_used = false; // soft removal

        // Mark all its descendants as not used
        std::queue<resource_id> pending_nodes;
        pending_nodes.push(r);
        while (!pending_nodes.empty()) {
            auto current = pending_nodes.front();
            pending_nodes.pop();
            resources[current] = resource(); // reset all fields back to default state
            resources[current].m_used = false; // soft removal
            for (auto child = get_first_child_resource(current); child != nresource; child = get_next_sibling_resource(child)) {
                pending_nodes.push(child);
            }
        }
    }

    void set_resource_local_transform(resource_id r, const glm::mat4& local_transform)
    {
        if (!(r < resources.size())) {
            log(LOG_LEVEL_ERROR, "set_resource_local_transform error: invalid arguments"); return;
        }

        resources[r].m_local_transform = local_transform;
    }

    glm::mat4 get_resource_local_transform(resource_id r)
    {
        if (!(r < resources.size())) {
            log(LOG_LEVEL_ERROR, "get_resource_local_transform error: invalid arguments"); return glm::mat4{1.0f};
        }

        return resources[r].m_local_transform;
    }

    void set_resource_mesh(resource_id r, mesh_id m)
    {
        if (r < resources.size() && m < meshes.size()) {
            resources[r].m_mesh = m;
        }
    }

    mesh_id get_resource_mesh(resource_id r)
    {
        return (r < resources.size()? resources[r].m_mesh : nmesh);
    }

    void set_resource_material(resource_id r, mat_id mat)
    {
        if (r < resources.size() && mat < materials.size()) {
            resources[r].m_material = mat;
        }
    }

    mat_id get_resource_material(resource_id r)
    {
        return (r < resources.size()? resources[r].m_material : nmat);
    }

    resource_id get_first_child_resource(resource_id r)
    {
        if (!(r < resources.size())) {
            log(LOG_LEVEL_ERROR, "get_first_child_resource error: invalid resource id"); return nresource;
        }

        return resources[r].m_first_child;
    }

    resource_id get_next_sibling_resource(resource_id r)
    {
        if (!(r < resources.size())) {
            log(LOG_LEVEL_ERROR, "get_next_sibling_resource error: invalid resource id"); return nresource;
        }

        return resources[r].m_next_sibling;
    }

    unique_resource make_resource(resource_id p)
    {
        return unique_resource(add_resource(p));
    }

    cubemap_id add_cubemap()
    {
        cubemaps.push_back(cubemap{});
        return cubemaps.size() - 1;
    }

    void set_cubemap_faces(cubemap_id id, const std::vector<std::string>& faces)
    {
        if (!(id < cubemaps.size())) {
            log(LOG_LEVEL_ERROR, "set_cubemap_faces error: invalid cubemap id"); return;
        }

        cubemaps[id].mfaces = faces;
    }

    void set_cubemap_gl_cubemap_id(cubemap_id cid, gl_cubemap_id gl_id)
    {
        if (cid < cubemaps.size()) {
            cubemaps[cid].m_gl_cubemap_id = gl_id;
        }
    }

    std::vector<std::string> get_cubemap_faces(cubemap_id id)
    {
        if (!(id < cubemaps.size())) {
            log(LOG_LEVEL_ERROR, "get_cubemap_faces error: invalid cubemap id"); return std::vector<std::string>();
        }

        return cubemaps[id].mfaces;
    }

    gl_cubemap_id get_cubemap_gl_cubemap_id(cubemap_id cid)
    {
        return (cid < cubemaps.size() ? cubemaps[cid].m_gl_cubemap_id : 0U);
    }

    cubemap_id get_first_cubemap()
    {
        return (cubemaps.size() ? 0 : ncubemap);
    }

    cubemap_id get_next_cubemap(cubemap_id id)
    {
        return (id + 1 < cubemaps.size()? id + 1 : ncubemap);
    }

} // namespace cgs
