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

using namespace std;
using namespace glm;
using uchar = uint8_t;
using ushort= uint16_t;
using uint = uint32_t;
using ulong = uint64_t;

namespace fs = std::filesystem;

//Any object which can cast rays for any reason

class raycaster
{
private:

    ushort my_tex;

    vector<vec2> triangle_fan;

    GLuint Buffer;
    ushort draw_size;

    #ifdef DEBUG_VERTICES
    GLuint Vertices_Buffer=-1;
    vector<ushort> debug_numbers;
    #endif
    #ifdef DEBUG_OUTLINE
    GLuint Outline_Buffer=-1;
    #endif
    #ifdef DEBUG_RAYS
    GLuint Rays_Buffer=-1;
    #endif

public:
    raycaster(vec2 origin, ushort tex);
    raycaster(raycaster&& that);//Just to be safe, define this
    ~raycaster();
    void set_origin(vec2 origin) {triangle_fan[0]= origin;}

    void update(const vector<mesh2D>& meshes);

    void display() const;

};
