#include "IO.hpp"
#include "player.hpp"
#include "world.hpp"

#include <string>
#include <fstream>
#include <filesystem>
#include <exception>
#include <cstdint>

#include <glm/glm.hpp>

#define TWO_PI 6.283185307179586


player::player(string name,int position_x,int position_y,bool human,float _fov, float _range)
{

    me=IO::graphics::load_tex(fs::path("hide_and_shoot_players")/(name+".png"));
    position=vec2(position_x,position_y);
    look_direction=0;
    fov=_fov;
    range=_range;
    is_human=human;

    IO::graphics::set_animation(me,2,4);
}

player::player(player&& you)
{
    me=you.me;
    you.me=-1;
    position=you.position;
    look_direction=you.look_direction;
    fov=you.fov;
    range=you.range;
    is_human=you.is_human;
}

player::player(const player& you)
{

    me=you.me;
    IO::graphics::add_tex_user(me);
    position=you.position;
    look_direction=you.look_direction;
    fov=you.fov;
    range=you.range;
    is_human=you.is_human;
}

player& player::operator=(player&& you)
{

    me=you.me;
    you.me=-1;
    position=you.position;
    look_direction=you.look_direction;
    fov=you.fov;
    range=you.range;
    is_human=you.is_human;

    return *this;
}

player& player::operator=(const player& you)
{
    me=you.me;
    IO::graphics::add_tex_user(me);
    position=you.position;
    look_direction=you.look_direction;
    fov=you.fov;
    range=you.range;
    is_human=you.is_human;
    return *this;
}

player::~player()
{
    IO::graphics::delete_tex(me);
    me = -1;
}

void player::display(int camera_position_x,int camera_position_y) const
{
    IO::graphics::animate_sprite(position.x-camera_position_x-4,position.y-camera_position_y-8,me,(8*look_direction/TWO_PI+0.5) ,false,false);

    IO::graphics::draw_tex(position.x-camera_position_x,position.y-camera_position_y-4,9);
}

//Move around in the world
void player::move(const world& Mundus,int mouse_x, int mouse_y,float dt)
{

    if (is_human)
    {
        look_direction = atan2(mouse_y-position.y,mouse_x-position.x);
        if (look_direction<0)
        {
            look_direction =TWO_PI+look_direction ;
        }


        //Move with arrowkeys
        vec2 move=vec2(0);

        if (IO::input_devices::up_key() )
            move.y+=1;
        if (IO::input_devices::down_key() )
            move.y-=1;
        if (IO::input_devices::left_key() )
            move.x-=1;
        if (IO::input_devices::right_key())
            move.x+=1;

        //make sure to normalize
        float M2 = move.x*move.x+move.y*move.y;
        if (M2>0.f)
        {
            float invM = 1.f/sqrt(M2);
            move.x*=invM*dt*50.f;
            move.y*=invM*dt*50.f;

            //And check if we can even go there
            vec2 new_pos = position+move;

            if (Mundus.walkable(new_pos.x,new_pos.y))
                position = new_pos;
        }
    }


}

