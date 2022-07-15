#pragma once


#include<string>
#include<iostream>
#include<fstream>
#include "my_filesystem.hpp"
#include <chrono>
#include <algorithm>
#include <exception>

#include"IO.hpp"
#include "mesh2D.hpp"
#include "raytracer.hpp"

#include<cstdint>
#include <random>

#include <glm/glm.hpp>

//I use these types so much that these aliases are well worth it
using uint = uint32_t;
using ulong = uint64_t;//I need to be sure that this is 32 bit or more, because I use it for keeping time, and 16 bit will break in 1 minute and 5 seconds, 32 and 64 bits will last a few years

using namespace std;
using namespace glm;


class world
{
private:
    tex_index bottom_textures;//A texture which you walk on, always visible if not obscured but "darkened" if not in view
    tex_index top_textures;   //And one which obscures it, roof's etc. which disapear if in view
    uint w,h;

    tex_index walk_tile_texture;//Mainly for debugging, a tile texture, for marking what can and can not be walked on

    //For walking around, you walk on a simple grid, yes a grid, I am very sorry about not using actual collision with the meshes
    uint tile_w;
    uint tile_h;
    uint grid_m;
    uint grid_n;
    vector<bool > walkable_grid;

    my_path polygon_file;
    vector<mesh2D> mess;
    uint active_mesh = -1;//-1 shorthand for non selected
public:
    world(my_path world_folder);

    ~world();
    uint get_w() const {return w;}
    uint get_h() const {return h;}
    void display_background(int offset_x,int offset_y) const;
    void display_top(int offset_x,int offset_y, bool grid=false) const;

    bool walkable(uint x, uint y) const;

    void save_polygons() const;

    void add_vertex(uint x, uint y);
    void new_polygon();

    void update_rc(raytracer& Reynold) const;

};
