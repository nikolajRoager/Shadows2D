#version 400 core

//Wait aren't transformation matrices 4D? yes, normally, but this is a 2D application, so I can chop of one dimension, alas, this does mean that I won't be using any of glm's build in transform functions, but nevermind, the matrices are pretty simple
uniform mat3 UV_to_DC;//Transformation from uv texture coordinates to device coordinates. The projection matrix is, in this case, just scaling from 0,1 by 0,1 ti -1,1 by -1,1
uniform mat3 UV_transform;//For animations we might transform the texture coordinates once

in vec2 vertex_uv;

out vec2 fragment_uv;
out vec2 fragment_pos_uv;

void main()
{
    fragment_uv=(UV_transform*vec3(vertex_uv,1)).xy;
    vec2 fragment_pos =(UV_to_DC*vec3(vertex_uv,1)).xy;
    fragment_pos_uv =fragment_pos*0.5+vec2(0.5);
    gl_Position = vec4(fragment_pos,0,1);//Should draw a square on the screen, to verrify all libraries and buffers are set up correctly
}

