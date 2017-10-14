const char* skybox_vertex_shader = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 a_pos;

    out vec3 tex_coords;

    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        tex_coords = a_pos;
        vec4 pos = projection * view * vec4(a_pos, 1.0);
        gl_Position = pos.xyww;
    }
)glsl";

const char* skybox_fragment_shader = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec3 tex_coords;

    uniform samplerCube cubemap;

    void main()
    {    
        FragColor = texture(cubemap, tex_coords);
    }
)glsl";
