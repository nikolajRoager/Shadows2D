#version 400 core

uniform vec3 color;
void main()
{
    //It really doesn't get more basic than this.
    gl_FragColor =  vec4(color,1);
}

