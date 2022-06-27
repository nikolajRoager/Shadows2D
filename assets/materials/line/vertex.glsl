#version 400 core


in vec2 vertex_uv;

uniform mat3 UV_to_DC;//Transformation from uv texture coordinates to device coordinates. The projection matrix is, in this case, just scaling from 0,1 by 0,1 ti -1,1 by -1,1

void main()
{
    vec2 fragment_pos =(UV_to_DC*vec3(vertex_uv,1)).xy;
    gl_Position = vec4(fragment_pos,0,1);//Should draw a square on the screen, to verrify all libraries and buffers are set up correctly
}

