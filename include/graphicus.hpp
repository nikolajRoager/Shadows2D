//A namespace collecting all functions and variables for the 2D sdl graphics
//This is a namespace and NOT a class, because this should only be initialized exactly ONCE

#ifndef GRAPHICUS_HPP
#define GRAPHICUS_HPP

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include<string>
#include<vector>
#include<cstdint>

#include<filesystem>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
//The underlying math won't be explained, I will just use it.


using namespace glm;

//I use these types so much that these aliases are well worth it
using ushort = uint16_t;
using uint = uint32_t;
using uchar = uint8_t;
using ulong = uint64_t;//I need to be sure that this is 32 bit or more, because I use it for keeping time, and 16 bit will break in 1 minute and 5 seconds, 32 bits will last a few years

using namespace std;

namespace fs = std::filesystem;

namespace graphicus
{
    bool is_active();

    void init(bool fullscreen, fs::path tex, fs::path audio, fs::path scripture, fs::path materia);


    ushort load_sound(string name);//Load this sound effect, and return its ID
    void play_sound(ushort ID);//Play this sound effect




    void pre_loop();//Updates window, both polling events and resetting the image, must be called each loop
    void flush();//Actually display all the things we made ready to display


    ulong get_millis();//Get current milli seconds of the start of this loop since start (not to this very moment, because I don't want things to experiance the little delay while I update everything else)


    void end();

    bool up_key(bool clicked);
    bool down_key(bool clicked);
    bool left_key(bool clicked);
    bool right_key(bool clicked);
    bool ctrl_key(bool clicked);
    bool shift_key(bool clicked);


    bool should_quit();

    vec2 get_mouse_pos();//Get mouse position in the world

    void debug_print_mouse_pos();

    bool mouse_click(bool right = false);//Was the mouse just clicked
    bool mouse_press(bool right = false);//Is the mouse being held down
    int get_scroll();

    void get_wh(vec2& V0, vec2& V1);


    void draw_lines(GLuint buffer, ushort size,vec3 color);
    void draw_triangles(GLuint buffer, ushort size,vec3 color);
    void draw_triangles(GLuint buffer, ushort size,vec3 color,vec2 origin);
    void draw_segments(GLuint buffer, ushort size,vec3 color);



    ushort load_tex(fs::path path, ushort& w, ushort& h);
    ushort load_tex(fs::path path);

    //Functions for loading a text with a certain ID, overwriting that specific ID and marking that ID as unused
    ushort set_text(const string text);
    void delete_text(ushort ID);
    void overwrite_text(ushort ID,const string text);

    void draw_tex(ushort texID, vec2 pos);
    void draw_text(ushort tex, vec2 pos);
}

#endif
