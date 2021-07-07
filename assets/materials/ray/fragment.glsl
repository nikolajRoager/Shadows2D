#version 400 core


out vec4 gl_FragColor;
uniform vec3 color;

uniform int mode;//0 or other no falloff, 1 quadratic falloff (useful for light)

uniform vec2 origin_worldspace;
in vec2 fragment_location_worldspace;
void main()
{
    //It really doesn't get more basic than this.

    float alpha = 0.0;
    if (mode ==1)
    {
        vec2 Dvec = fragment_location_worldspace-origin_worldspace;
        alpha = 1.f/(sqrt(dot(Dvec,Dvec))+1);//What is this madness? it certainly is not physical falloff, but I found that this ended up looking better, +1 to avoid that crazy tending towards infinity, and linear falloff because quadratic is just too fast
    }
    gl_FragColor =  vec4(color,alpha);
}

