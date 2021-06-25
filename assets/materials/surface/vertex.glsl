#version 400 core

uniform mat4 MVP;

in vec2 vertex_location_modelspace;
in vec2 vertex_uv;

out vec2 fragment_uv;

void main()
{
    fragment_uv=vertex_uv;

    //It really doesn't get more basic than this.
    gl_Position =  MVP * vec4(vertex_location_modelspace,0,1);
}

