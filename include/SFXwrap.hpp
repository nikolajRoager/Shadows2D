/*THIS FILE
//This is a sound wrapper class, which allows our audio program to easilly interface with SDL sound, this is really a much much simpler version of the texwrap class
*/

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <string>
#include <vector>
#include"my_filesystem.hpp"
#include<cstdint>
#include<iostream>


using namespace std;
using uchar = uint8_t;
using uint = uint32_t;
using ulong = uint64_t;






namespace IO::audio
{

    //The class storing individual sound effects
    class SFX
    {
    private:
        string name;//For identifying sounds, so that we don't load the same multiple times
        Mix_Chunk* sound;

    public:
        SFX(string Name);
        ~SFX();
        //Move constructor, copy the data from that sound into this sound, then delete that sound safely
        SFX(SFX&& that);

        //Copy constructor is not needed, explicitly declare it deleted (actually since we have a move constructor, it is deleted anyway, I just like to be explicit)
        SFX(const SFX& that) = delete;

        void set_name(string n);

        string get_name();



        bool is(const string& S);//Used for checking if a sound has already been loaded

        //check if this sound has not been unloaded
        bool is_good() ;

        void loadWAV(const my_path& sounds);

        void unload();

        //Play, and return what channel we are playing at
        int play();
    };

}
