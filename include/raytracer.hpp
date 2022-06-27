#pragma once

#include <string>
#include <vector>
#include<filesystem>
#include<fstream>
#include<cstdint>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "mesh2D.hpp"
#include "IO.hpp"
#include <GL/glew.h>

#define TWO_PI 6.28318531
#define PI 3.14159265


#define DEBUG_OUTLINE
#define DEBUG_VERTICES
//#define DEBUG_NO_TRIANGLES


using namespace std;
using namespace glm;
using uchar = uint8_t;
using uint = uint32_t;
using ulong = uint64_t;

namespace fs = std::filesystem;

//Any object which can cast rays for any reason

class raytracer
{
private:


    vec2 V0,V1;//Two vertices defining the bounding box of this raytracer, likely set to the screen boundaries or some other border (box from V0.x,V0.y to V1.x,V0.y to V1.x,V1.y to V0.x,V1.y and back, the order of the vertices does not matter)
    uint my_tex;

    vector<vec2> triangle_fan;

    GLuint Buffer;
    uint draw_size;

    //Looking direction and angle of the lens
    float theta = 0;
    float lens_angle = TWO_PI;

    //Actually, I don't want to have to deal with the angle, then I would need to consider where to wrap around, let us instead use a viewing direction and the dot product of the extreme angles
    vec2 dir = vec2(0);
    vec2 extreme_left =vec2(0);
    vec2 extreme_right = vec2(0);
    float extreme_dot = 0;

    bool limit_lens = false;

    #ifdef DEBUG_VERTICES
    GLuint Vertices_Buffer=-1;
//    vector<uint> debug_numbers;
    #endif
    #ifdef DEBUG_OUTLINE
    GLuint Outline_Buffer=-1;
    #endif

public:
    raytracer(vec2 origin, uint tex,bool do_display);
    raytracer(raytracer&& that);//Just to be safe, define this
    ~raytracer();
    void set_origin(vec2 origin) {triangle_fan[0]= origin;

    //Easy fix, now the light source will not line up EXACTLY with the vertices, and most problems just won't happen now
 //   triangle_fan[0]+=vec2(0.000013f,0.000007f);//as small offset as I can get away with
    }

    void update(const vector<mesh2D>& meshes,bool do_display=true);

    void set_bounds(vec2 _V0,vec2 _V1) {V0=_V0; V1=_V1;}

    void screen_bounds()
    {//Update bounds based on screen, requires openGL to be turned on
        V0.x = -IO::graphics::get_w()/100.f;
        V1.x =  IO::graphics::get_w()/100.f;
        V0.y = -IO::graphics::get_h()/100.f;
        V1.y =  IO::graphics::get_h()/100.f;

    }
    void display() const;

    void set_angle(float theta, float D);
};
