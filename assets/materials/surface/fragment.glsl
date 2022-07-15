#version 400 core

uniform sampler2D colorSampler;
uniform sampler2D lightSampler;
in vec2 fragment_pos_uv;

in vec2 fragment_uv;
out vec4 color;

uniform int dynamic_light;
//0 no light, 1 dynamic light from light sampler

//If we use dynamic light, what color should we blend to if in shadow
uniform float shadow_blend;
uniform vec4 shadow_color;

void main()
{


    //It really doesn't get more basic than this.
    color =  texture(colorSampler,fragment_uv);


    if (dynamic_light>0)
    {
        vec2 poissonDisk[8] = vec2[](
          vec2( 0.0, -1.0/180 ),
          vec2( 0.0,  1.0/180 ),
          vec2(-1.0/320, 0.0 ),
          vec2( 1.0/320, 0.0 ),
          vec2( 0.0, -2.0/180 ),
          vec2( 0.0,  2.0/180 ),
          vec2(-2.0/320, 0.0 ),
          vec2( 2.0/320, 0.0 )
        );


        float lightness = 0; texture(lightSampler,fragment_pos_uv).r;
        for (int i=0;i<4;i++)
        {
           lightness+=0.15*texture( lightSampler, fragment_pos_uv + poissonDisk[i] ).r;
           lightness+=0.10*texture( lightSampler, fragment_pos_uv + poissonDisk[i+4] ).r;
        }

        //mode 2, blend if in light
        if (dynamic_light ==2)
            lightness = 1-lightness;


        if (shadow_color.r>=0)//-1 is used as shorthand for: do not blend this
            color.r= color.r*lightness+(1-lightness)*(shadow_color.r*shadow_blend+color.r*(1-shadow_blend));
        if (shadow_color.g>=0)
            color.g= color.g*lightness+(1-lightness)*(shadow_color.g*shadow_blend+color.g*(1-shadow_blend));
        if (shadow_color.b>=0)
            color.b= color.b*lightness+(1-lightness)*(shadow_color.b*shadow_blend+color.b*(1-shadow_blend));
        if (shadow_color.a>=0)
            color.a= min( color.a*lightness+(1-lightness)*(shadow_color.a*shadow_blend+color.a*(1-shadow_blend)),color.a);
    }


}

