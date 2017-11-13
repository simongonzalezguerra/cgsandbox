const char* environment_mapping_vertex_shader = R"glsl(
    #version 330 core

    // Input vertex data, different for all executions of this shader.
    layout(location = 0) in vec3 vertex_position_modelspace;
    layout(location = 1) in vec2 vertex_tex_coords;
    layout(location = 2) in vec3 vertex_direction_n_modelspace;

    // Output data ; will be interpolated for each fragment.
    out vec2 tex_coords;
    out vec3 position_worldspace;
    out vec3 direction_n_worldspace;

    // Values that stay constant for the whole mesh.
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 mvp;

    void main(){
        // Output position of the vertex, in clip space : mvp * position
        gl_Position =  mvp * vec4(vertex_position_modelspace, 1);

        // Position of the vertex, in worldspace : model * position
        vec4 position_worldspace4 = model * vec4(vertex_position_modelspace, 1);
        position_worldspace = position_worldspace4.xyz / position_worldspace4.w;

        direction_n_worldspace = mat3(transpose(inverse(model))) * vertex_direction_n_modelspace;

        // Texture coordinates of the vertex. No special space for this one.
        tex_coords = vertex_tex_coords;
    }
)glsl";

const char* environment_mapping_fragment_shader = R"glsl(
    // Here lighting computations are performed in world space because we need to sample the
    // cubemap. If we did this in camera space we would always get the same result when the
    // camera moves, since the cubemap is fixed with respect to the camera

    #version 330 core

    #define MAX_POINT_LIGHTS 10

    struct material_data
    {
        sampler2D diffuse_sampler;
        vec3      diffuse_color;
        vec3      specular_color; 
        float     smoothness;
        float     reflectivity;
        float     translucency;
        float     refractive_index;
    };

    struct dirlight_data
    {
        vec3 ambient_color;
        vec3 diffuse_color;
        vec3 direction_cameraspace;
    };

    struct point_light_data
    {
        vec3  position_cameraspace;
        vec3  ambient_color;
        vec3  diffuse_color;
        float constant_attenuation;
        float linear_attenuation;
        float quadratic_attenuation;
    };

    // Interpolated values from the vertex shaders
    in vec2 tex_coords;
    in vec3 position_worldspace;
    in vec3 position_cameraspace;
    in vec3 direction_n_worldspace;

    // Ouput data
    out vec3 color;

    // Values that stay constant for the whole mesh.
    uniform material_data     material;
    uniform dirlight_data     dirlight;
    uniform point_light_data  point_lights[MAX_POINT_LIGHTS];
    uniform uint              npoint_lights;
    uniform vec3              camera_position_worldspace;
    uniform samplerCube       cubemap;

    // Calculates the contribution of the directional light
    vec3 calc_dirlight(dirlight_data dirlight,
                        vec3 n_cameraspace,
                        material_data material,
                        vec2 tex_coords)
    {
        vec3 l_cameraspace = normalize(-dirlight.direction_cameraspace);
        // diffuse shading
        float cos_theta_diff = clamp(dot(n_cameraspace, l_cameraspace), 0, 1);
        // combine results
        vec3 ambient = dirlight.ambient_color * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
        vec3 diffuse = dirlight.diffuse_color * cos_theta_diff * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
        return (ambient + diffuse);
    }

    vec3 calc_point_light(point_light_data point_light,
                        vec3 n_cameraspace,
                        vec3 position_cameraspace,
                        material_data material,
                        vec2 tex_coords)
    {
        vec3 l_cameraspace = normalize(point_light.position_cameraspace - position_cameraspace);
        // diffuse shading
        float cos_theta_diff = clamp(dot(n_cameraspace, l_cameraspace), 0, 1);
        // attenuation
        float distance = length(point_light.position_cameraspace - position_cameraspace);
        float attenuation = 1.0 / ( point_light.constant_attenuation
                                   + point_light.linear_attenuation * distance
                                   + point_light.quadratic_attenuation * (distance * distance));    
        // combine results
        vec3 ambient  = point_light.ambient_color * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
        vec3 diffuse  = point_light.diffuse_color * cos_theta_diff * vec3(texture(material.diffuse_sampler, tex_coords)) * material.diffuse_color;
        ambient *= attenuation;
        diffuse *= attenuation;
        return (ambient + diffuse);
    }

    void main()
    {
        // Normal of the computed fragment, in camera space
        vec3 n_cameraspace = normalize(direction_n_worldspace);
        // Phase 1: directional lighting
        color = calc_dirlight(dirlight, n_cameraspace, material, tex_coords);
        // Phase 2: point lights
        for (uint i = 0U; i < npoint_lights; i++) {
            color += calc_point_light(point_lights[i], n_cameraspace, position_cameraspace, material, tex_coords); 
        }
        // Phase 3: reflective component
        vec3 i_worldspace = normalize(position_worldspace - camera_position_worldspace);
        vec3 reflection_worldspace = reflect(i_worldspace, normalize(direction_n_worldspace));
        vec3 specular = vec3(texture(cubemap, reflection_worldspace)) * material.specular_color * material.reflectivity;
        color += specular;
        // Phase 4: refraction component
        vec3 refraction_worldspace = refract(i_worldspace, normalize(direction_n_worldspace), 1.0 / material.refractive_index);
        vec3 refraction = vec3(texture(cubemap, refraction_worldspace)) * material.translucency;
        color += refraction;
    }
)glsl";
