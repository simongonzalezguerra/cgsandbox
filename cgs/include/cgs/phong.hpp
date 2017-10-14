const char* phong_vertex_shader = R"glsl(
    #version 330 core

    // Input vertex data, different for all executions of this shader.
    layout(location = 0) in vec3 vertex_position_modelspace;
    layout(location = 1) in vec2 vertex_tex_coords;
    layout(location = 2) in vec3 vertex_direction_n_modelspace;

    // Output data ; will be interpolated for each fragment.
    out vec2 tex_coords;
    out vec3 position_worldspace;
    out vec3 position_cameraspace;
    out vec3 direction_n_cameraspace;
    out vec3 direction_v_cameraspace;

    // Values that stay constant for the whole mesh.
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 mvp;

    void main(){
        // Output position of the vertex, in clip space : mvp * position
        gl_Position =  mvp * vec4(vertex_position_modelspace,1);

        // Position of the vertex, in worldspace : model * position
        position_worldspace = (model * vec4(vertex_position_modelspace, 1)).xyz;

        // Vector that goes from the vertex to the camera, in camera space.
        // In camera space, the camera is at the origin (0,0,0).
        position_cameraspace = (view * model * vec4(vertex_position_modelspace, 1)).xyz;
        direction_v_cameraspace = vec3(0,0,0) - position_cameraspace;

        // Normal of the the vertex, in camera space. Note this is only correct if the model
        // transform does not scale the model in a way that is non-uniform accross all axes! If not
        // you can use its inverse transpose, but keep in mind that computing the inverse is expensive
        // (direction_n_cameraspace = mat3(transpose(inverse(model))) * vertex_direction_n_modelspace;)
        direction_n_cameraspace = ( view * model * vec4(vertex_direction_n_modelspace,0)).xyz;

        // Texture coordinates of the vertex. No special space for this one.
        tex_coords = vertex_tex_coords;
    }
)glsl";

const char* phong_fragment_shader = R"glsl(
    // Lighting computations are performed in camera space. This can also be done in worlspace with the same
    // result, what matters is that all vectors are expressed in the same coordinate system. Regardless of this,
    // there are precision advantages to computing in view space (worldspace can have coordinates with large
    // values that introduce precision issues).
    // Source:
    // https://www.opengl.org/discussion_boards/showthread.php/168104-lighting-in-eye-space-or-in-world-space

    #version 330 core

    #define MAX_POINT_LIGHTS 10

    struct material_data
    {
        sampler2D diffuse_sampler;
        vec3      diffuse_color;
        vec3      specular_color; 
        float     smoothness;
    };

    struct dirlight_data
    {
        vec3 ambient_color;
        vec3 diffuse_color;
        vec3 specular_color;
        vec3 direction_cameraspace;
    };

    struct point_light_data
    {
        vec3  position_cameraspace;
        vec3  ambient_color;
        vec3  diffuse_color;
        vec3  specular_color;
        float constant_attenuation;
        float linear_attenuation;
        float quadratic_attenuation;
    };

    // Interpolated values from the vertex shaders
    in vec2 tex_coords;
    in vec3 position_worldspace;
    in vec3 position_cameraspace;
    in vec3 direction_n_cameraspace;
    in vec3 direction_v_cameraspace;

    // Ouput data
    out vec3 color;

    // Values that stay constant for the whole mesh.
    uniform material_data     material;
    uniform dirlight_data     dirlight;
    uniform point_light_data  point_lights[MAX_POINT_LIGHTS];
    uniform uint              npoint_lights;

    // Calculates the contribution of the directional light
    vec3 calc_dirlight(dirlight_data dirlight,
                        vec3 n_cameraspace,
                        vec3 v_cameraspace,
                        material_data material,
                        vec2 tex_coords)
    {
        vec3 l_cameraspace = normalize(-dirlight.direction_cameraspace);
        // diffuse shading
        float cos_theta_diff = clamp(dot(n_cameraspace, l_cameraspace), 0, 1);
        // specular shading
        vec3 r_cameraspace = reflect(-l_cameraspace, n_cameraspace);
        float cos_alpha_spec = clamp(dot(v_cameraspace, r_cameraspace), 0, 1);
        // combine results
        vec3 ambient = dirlight.ambient_color * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
        vec3 diffuse = dirlight.diffuse_color * cos_theta_diff * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
        vec3 specular = dirlight.specular_color * pow(cos_alpha_spec, material.smoothness) * material.specular_color;
        return (ambient + diffuse + specular);
    }

    vec3 calc_point_light(point_light_data point_light,
                        vec3 n_cameraspace,
                        vec3 position_cameraspace,
                        vec3 v_cameraspace,
                        material_data material,
                        vec2 tex_coords)
    {
        vec3 l_cameraspace = normalize(point_light.position_cameraspace - position_cameraspace);
        // diffuse shading
        float cos_theta_diff = clamp(dot(n_cameraspace, l_cameraspace), 0, 1);
        // specular shading
        vec3 r_cameraspace = reflect(-l_cameraspace, n_cameraspace);
        float cos_alpha_spec = clamp(dot(v_cameraspace, r_cameraspace), 0, 1);
        // attenuation
        float distance = length(point_light.position_cameraspace - position_cameraspace);
        float attenuation = 1.0 / (point_light.constant_attenuation
                                   + point_light.linear_attenuation * distance
                                   + point_light.quadratic_attenuation * (distance * distance));    
        // combine results
        vec3 ambient  = point_light.ambient_color * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
        vec3 diffuse  = point_light.diffuse_color * cos_theta_diff * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
        vec3 specular = point_light.specular_color * pow(cos_alpha_spec, material.smoothness) * material.specular_color;
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;
        return (ambient + diffuse + specular);
    }

    void main()
    {
        // Normal of the computed fragment, in camera space
        vec3 n_cameraspace = normalize(direction_n_cameraspace);
        // Eye vector (towards the camera)
        vec3 v_cameraspace = normalize(direction_v_cameraspace);
        // Phase 1: directional lighting
        color = calc_dirlight(dirlight, n_cameraspace, v_cameraspace, material, tex_coords);
        // Phase 2: point lights
        for (uint i = 0U; i < npoint_lights; i++) {
            color += calc_point_light(point_lights[i], n_cameraspace, position_cameraspace, v_cameraspace, material, tex_coords); 
        }
    }
)glsl";
