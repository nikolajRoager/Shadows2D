#version 400 core

uniform sampler2D colorSampler;

in vec2 fragment_uv;
out vec4 color;

void main()
{


    //It really doesn't get more basic than this.
    color =  texture(colorSampler,fragment_uv);
}

