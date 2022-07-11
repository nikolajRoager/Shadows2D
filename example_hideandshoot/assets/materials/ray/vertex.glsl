#version 400 core

uniform mat4 VP;

in vec2 vertex_location_worldspace;

uniform vec2 origin_worldspace;
uniform  int mode;//0 or other no falloff, 1 quadratic falloff (useful for light)

out vec2 fragment_location_worldspace;

void main()
{
    fragment_location_worldspace = vertex_location_worldspace;
    //It really doesn't get more basic than this.
    gl_Position =  VP * vec4(vertex_location_worldspace,0,1);
}

