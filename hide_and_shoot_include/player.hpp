#pragma once

#include "IO.hpp"
#include "world.hpp"

#include <string>
#include <fstream>
#include <filesystem>
#include <exception>
#include <cstdint>

#include <glm/glm.hpp>

using namespace glm;
using namespace std;

class player
{
private:
    tex_index me;//What is my texture

    //Where am I
    vec2 position;

    //And what can I see
    float look_direction;
    float fov;
    float range;

    //Am I a human?
    bool is_human;

public:

    player(string name,int position_x,int position_y,bool human=false,float _fov=3.0, float _range=256.0);
    player(player&& you);
    player(const player& you);
    player& operator=(player&& you);
    player& operator=(const player& you);
    ~player();

    void display(int position_x,int position_y) const;

    //Move around in the world
    void move(const world& Mundus,int mouse_x, int mouse_y,float dt);

    int get_x() const {return position.x;}
    int get_y() const {return position.y;}
};
