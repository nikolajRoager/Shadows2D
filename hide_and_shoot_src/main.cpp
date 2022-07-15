//NOTE I use the glm vector math library, which USES SINGLE PRECISION FLOATS BY DEFAULT, for compatibility with OpenGL, I will use OpenGL for graphics, and besides I am not doing science, I am making something for a computer game, single precision is all I need in almost all cases ... the angle of the points which go into the sorting algorithm later is a douple, but that is the only place where that was important enough. note that I do actually need to explicitly define that ... literally everywhere, that is why I write numbers like 0.5f or 1.f

//This is multiply defined a lot of places, the compiler gets real mad but I prefer to do this explicitly whenever I need it
#define TWO_PI 6.283185307179586
//Yeah let us keep this douple precision, if I need to use single precision float(TWO_PI) will be interpreted like 6.283...f that by the compiler ... it is not literally calling a double to float function every time, that would be crazy.

#include<string>
#include<iostream>
#include<fstream>
#include<filesystem>
#include <chrono>
#include <algorithm>
#include <exception>

#include"raytracer.hpp"
#include"IO.hpp"
#include "mesh2D.hpp"
#include "world.hpp"
#include "player.hpp"

#include<cstdint>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>



//I use these types so much that these aliases are well worth it
//I have never come across any system where this was an issue, but I am horrified by the fact that the c++ standards technically allows for integers of different sizes to technically be used, I want to be absolutely sure that I know where every bit is ... ok, in this project I don't do any bit manipulation, but I do that on a regular basis in the game this is meant for. I personally prefer to use the smallest type I can get away with, and never use signed if unsigned will suffice.
using uint = uint32_t;
using ulong = uint64_t;//I need to be sure that this is 32 bit or more, because I use it for keeping time, and 16 bit will break in 1 minute and 5 seconds, 32 and 64 bits will last a few years

using namespace std;
using namespace glm;

namespace fs = std::filesystem;

//The main loop, I try to keep anything graphics related out of this.
int main(int argc, char* argv[])
{

    fs::path assets = "assets";

    tex_index cursor=-1;

    cout<<"Starting main display"<<endl;
    bool devmode=false;
    try
    {
        if (argc==3)
            if (string(argv[2]).compare("--devmode")==0)
                devmode = true;
        IO::init(false,"I can write whatever I want here, and nobody can stop me",assets/"textures",assets/"audio",assets/"fonts",assets/"materials",assets/"keymap.txt", devmode, 320,180);
        cursor = IO::graphics::load_tex(fs::path("ui")/"cursor.png");
        cout<<cursor<<endl;

    }
    catch(string error)
    {
        IO::end();
        cout<<"couldn't set up graphics engine: "<<error<<endl;
        return 0;
    }


    mesh2D::toggle_graphics(devmode);
    vector<mesh2D> mess;


    uint meshes = mess.size();

    uint active_mesh = meshes-1;//NEVER MIND THE UNDERFLOW! I will check that meshes>0 before calling anything, and once I increase meshes, we will overflow back where we started.

//    raytracer reynold(vec2(0),do_display);

//    float this_theta = 2.8;
//    float this_Dtheta=TWO_PI;

    world Mundus(assets/fs::path("hide_and_shoot_maps")/(argc > 1 ? fs::path(argv[1]) : fs::path("default")));




    player seeker = player("red",50,102,true);

    double dt = 0;

    ulong millis=0;
    ulong pmillis = IO::input_devices::get_millis();


    int mouse_x_px=0;
    int mouse_y_px=0;

    //The camera should follow the human player around
    int cam_x=0;
    int cam_y=0;
    vector<string> help_text;
    help_text.push_back("lightmap : show player lightmap");
    help_text.push_back("polysave : save map of polygons");
    help_text.push_back("polyadd  : add new polygon");
    do
    {
        millis = IO::input_devices::get_millis();
        IO::input_devices::get_mouse(mouse_x_px,mouse_y_px);
        int scroll = IO::input_devices::get_scroll();


        dt = (millis-pmillis)*0.001;

        //Without devmode, this is just turned off
        string text_input = "null";
        if (IO::input_devices::get_command(text_input))
        {
            if (text_input.compare("lightmap")==0)
            {
                IO::graphics::debug_showlightmap();
            }
            else if (text_input.compare("polyw")==0)
            {
                Mundus.save_polygons();
            }
            else if (text_input.compare("polyadd")==0)
            {
                Mundus.new_polygon();
            }
            else if (text_input.compare("help")==0)
            {
                IO::input_devices::print_lines(help_text);//Mostly used to print list of commands when given :help
            }
            else
            {
                string what = "Whaaaat (try help)";
                IO::input_devices::print(what);
            }
        }
        if (IO::input_devices::mouse_click(false))//Left click to add new vertex to old object
        {
            Mundus.add_vertex(mouse_x_px+cam_x,mouse_y_px+cam_y);
        }


        seeker.move(Mundus,mouse_x_px+cam_x,mouse_y_px+cam_y,dt);
        //I want the camera to center the player, but actually  the "camera" is the lowest left coordinates
        cam_x=seeker.get_x()-160;
        cam_y=seeker.get_y()-90;

        //And do make sure we clip to the edge of the screen
        if (cam_x<0)
            cam_x=0;
        else if(cam_x+IO::graphics::get_w()>Mundus.get_w())
            cam_x=Mundus.get_w()-IO::graphics::get_w();

        if (cam_y<0)
            cam_y=0;
        else if(cam_y+IO::graphics::get_h()>Mundus.get_h())
            cam_y=Mundus.get_h()-IO::graphics::get_h();

        //The player sees only what the player sees
        seeker.bake_lightmap(cam_x,cam_y);

        Mundus.display_background(cam_x,cam_y);
        seeker.display(cam_x,cam_y);

        Mundus.display_top(cam_x,cam_y,devmode);

//            IO::graphics::activate_Lightmap();
//            reynold.bake_to_shadowmap(vec3(1),500);
//            IO::graphics::activate_Display();//To be fair, texture rendering and ending the loop auto-jumps back to display, but lets just do it explicitly

        IO::graphics::draw_tex(mouse_x_px,mouse_y_px,cursor);



        IO::post_loop();

        pmillis = millis;

    }
    while (!IO::input_devices::should_quit() && !IO::input_devices::esc_key());

    IO::end();

    cout<<"Stop"<<endl;
    return 0;
    //All is gone, the program has quit.
}
