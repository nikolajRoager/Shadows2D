#version 400 core


in vec2 vertex_worldspace;

uniform mat3 worldspace_to_DC;

void main()
{
    vec2 fragment_pos =(worldspace_to_DC*vec3(vertex_worldspace,1)).xy;
    gl_Position = vec4(fragment_pos,0,1);//Should draw a square on the screen, to verrify all libraries and buffers are set up correctly
}

