#pragma once

#include <string>
#include <vector>
#include<filesystem>
#include<fstream>
#include<cstdint>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "mesh2D.hpp"
#include "graphicus.hpp"
#include <GL/glew.h>

#define TWO_PI 6.28318531
#define PI 3.14159265
using namespace std;
using namespace glm;
using uchar = uint8_t;
using uint = uint32_t;
using ulong = uint64_t;

namespace fs = std::filesystem;

//For comparisons sake, this raycaster works differently from (and is much simpler than) the raytracer, but the output and input is the same format

class raycaster
{
private:


    vec2 V0,V1;//Two vertices defining the bounding box of this raycaster, likely set to the screen boundaries or some other border (box from V0.x,V0.y to V1.x,V0.y to V1.x,V1.y to V0.x,V1.y and back, the order of the vertices does not matter)
    uint my_tex;

    vector<vec2> triangle_fan;
    vector<vec2> unit_circle_reference;

    GLuint Buffer;
    uint draw_size;//Essentially resolution, except includes the center point and the start of the circle twice to loop around... we need draw size every time we display, but only resolution when we re calculate so I store this ... ok to be fair, if we have all display turned off, or only render every update it doesn't matter what we save

    //Looking direction and angle of the lens
    float theta = 0;
    float lens_angle = TWO_PI;

    //Actually, I don't want to have to deal with the angle, then I would need to consider where to wrap around, let us instead use a viewing direction and the dot product of the extreme angles
    vec2 dir = vec2(0);
    vec2 extreme_left =vec2(0);
    vec2 extreme_right = vec2(0);
    float extreme_dot = 0;

    bool limit_lens = false;
public:
    raycaster(vec2 origin, uint tex,uint res,bool do_display);
    raycaster(raycaster&& that);//Just to be safe, define this
    ~raycaster();
    void set_origin(vec2 origin) {triangle_fan[0]= origin;

    //Easy fix, now the light source will not line up EXACTLY with the vertices, and most problems just won't happen now
    triangle_fan[0]+=vec2(0.000013,0.000007);
    }

    void update(const vector<mesh2D>& meshes,bool do_display=true);

    void set_bounds(vec2 _V0,vec2 _V1) {V0=_V0; V1=_V1;}

    void screen_bounds()
    {//Update bounds based on screen, requires openGL to be turned on
        graphicus::get_wh(V0,V1);
    }
    void display() const;


    void set_angle(float theta, float D);
};
