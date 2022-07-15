/*THIS FILE
Definition to the master IO functions (those which secretly call the other namespaces behind the scenes)
These are put in the same header, so that we only need to include one header, the definitions are separate files though
*/


//openGL Extension Wrangler library
#include <GL/glew.h>
//Main sdl
#include <SDL2/SDL.h>
//loading of png and jpg
#include <SDL2/SDL_image.h>
//openGL functionalities
#include <SDL2/SDL_opengl.h>
//sound
#include <SDL2/SDL_mixer.h>


//std string
#include <string>
//string-stream, for turning a string into a "stream" object, useful for loading plain text files
#include <sstream>
//text output, should never be used in looping functions
#include <iostream>
//operating independent file-system functions
#include"my_filesystem.hpp"
//file streams, we use filesystem for paths, and fstream to actually stream the data from files in.
#include <fstream>


//Dynamic sized list
#include <vector>

//Not so dynamic sized lists
#include <array>


//I use these types so much that these aliases are well worth it
using tex_index = uint16_t;
using uint = uint32_t;
using ulong = uint64_t;

using namespace std;


//The separate init or end functions should not be visible for anyone who includes IO.hpp, but I need to see them here, so declare them here, the linker will link them to their definitions
//I don't think this is great style, but I do not see any usable alternative


namespace IO::graphics
{
    // ---- one time functions ----
    void init(bool fullscreen,string window_header, my_path tex, my_path scripture,my_path shaders, uint _w=320, uint _h=180);
    void end();


        // ---- Functions which must be called once per loop ----
        void clear();//Updates window  and resets the image, must be called each loop
        /*
        Anything which should go onto the screen should go between pre loop and flush.
        */
        void flush();//Actually display all the things we made ready to display



        //---Functions for getting information about the graphical setup ar currently not public---


        //---Print ram ----
        void print_ram();//Print currently loaded textures to the external terminal (usually doesn't fit in the developer commandline)

}
namespace IO::audio
{
    // ---- one time functions ----
    void init(my_path audio);
    void end();
        // ---- Functions which must be called once per loop ----
        void update_loops();//Update looping audio
}

namespace IO::input_devices
{
    // ---- one time functions ----
    void init(my_path keymaps, bool devmode);
    void end();
    //Enable calling developer commandline
    void set_devmode(bool dev);
    // ---- Functions which must be called once per loop ----
    //Polling and updating before and after ensures we can keep track of not only what is being done, but whether something has just changed
    void poll_events();
    //NOTE all inputs are updated at poll_events. Hence calling any of these input functions returns the same result regardless of when and how many times you call it
    void print_commandline();//If developer commandline is turned of, this does nothing.
}

#include "IO.hpp"


namespace IO
{
    // Some internal variables
    //All the paths we need to know
    my_path texture_path;
    my_path sound_path;
    my_path fonts_path;
    my_path shader_path;




    void init(bool fullscreen,string window_header,  my_path tex, my_path audio, my_path scripture, my_path shaders,my_path keymaps, bool devmode, uint _w, uint _h)
    {
        texture_path = tex;
        sound_path = audio;
        fonts_path = scripture;
        shader_path = shaders;

        //If this is a restart command, actually just stop and start again
        end();



        //Get the error ready, just in case
        //TBD use std exception instead
        string error = "";

        //Initialize SDL globally, it does not make sense to include in any specific subspace
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        {
            error.append("Could not initialize SDL:\n\t\t");
            error.append(SDL_GetError());
            throw std::runtime_error(error);
        }

        graphics::init(fullscreen,window_header, tex, scripture, shaders, _w, _h);
        audio::init(audio);



        input_devices::init(keymaps, devmode);

        //Do one false loop, this is just to make the screen all black while we load
        graphics::clear();
        post_loop();


    }

    bool should_quit()
    {
        //The little x on the top of the screen has been pressed, the program can also be quit if this is not set
        return input_devices::should_quit();//The poor program, it does not know that returning true will have it murdered instantly
    }



    //the part where all we have done this frame actually ends up on the screen, this function both posts the now finished frame to the screen, and makes ready for the next frame
    void post_loop()
    {
        input_devices::print_commandline();
        //Reset events, and write current key-presses to int term memory (for detecting when buttons have just been released and pressed)

        //Now update looping soun, it really does not matter if we do this before or after the graphics is flushed.
        audio::update_loops();
        //Any drawing until now have been done to the internal framebuffer, now flush this onto the screen, after this point and until we call clear the drawing functions do not work.
        graphics::flush();

        //TBD, Opengl VSync is not reliable on all machines, implement a custom waiting function to not exceed 60 Hz. ... right here is where we need it


        graphics::clear();//Alright, now we have officially started next frame, doing this in one function ensures we never have any draw functions between flush and clear
        input_devices::poll_events();




    }//ends post_loop



    void print_ram()
    {
        cout<<"===Texture and Audio memory report==="<<endl;
        graphics::print_ram();
        audio::print_ram();
        cout<<"====================================="<<endl;
    }

    void end()
    {
        //Quit input devices before graphics, because input devices did load some textures (text boxes)
        input_devices::end();
        graphics::end();
        audio::end();

        //Close SDL
        SDL_Quit();
    }

}//ends namespace IO
