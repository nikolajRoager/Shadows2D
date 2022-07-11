#version 400 core

uniform vec3 color;
uniform vec2 origin;
uniform float range;

in vec2 fragment_pos_worldspace;

void main()
{

    vec2 o_f = fragment_pos_worldspace-origin;
    if (range<0)//Negative range is understood as infinite
    {
        gl_FragColor =  vec4(color,1);
    }
    else if (range==0)//Divide by 0
    {
        gl_FragColor =  vec4(color,0.0);
    }
    else
    {
        float dist = 1-(sqrt(dot(o_f,o_f)))/(range);

        if (dist>0.1 || range<0)
            gl_FragColor =  vec4(color,1);
        else if (dist>0)
            gl_FragColor =  vec4(color,dist*dist*100);
        else
            gl_FragColor =  vec4(color,0.0);
    }
}

