#pragma once


#include<string>
#include<iostream>
#include<fstream>
#include<filesystem>
#include <chrono>
#include <algorithm>
#include <exception>

#include"IO.hpp"
#include "mesh2D.hpp"

#include<cstdint>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

//I use these types so much that these aliases are well worth it
using uint = uint32_t;
using ulong = uint64_t;//I need to be sure that this is 32 bit or more, because I use it for keeping time, and 16 bit will break in 1 minute and 5 seconds, 32 and 64 bits will last a few years

using namespace std;
using namespace glm;


class world
{
private:
    vector<tex_index> textures;
    uint w,h;

public:
    world(fs::path world_folder);
    ~world();
    uint get_w() const {return w;}
    uint get_h() const {return h;}
    void display(int offset_x,int offset_y) const;
};
