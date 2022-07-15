/*THIS FILE
//This is a sound wrapper class, which allows our audio program to easilly interface with SDL sound, this is really a much much simpler version of the texwrap class
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <string>
#include <vector>
#include"my_filesystem.hpp"
#include<cstdint>
#include<iostream>


#include"SFXwrap.hpp"

using namespace std;
using uchar = uint8_t;
using uint = uint32_t;
using ulong = uint64_t;


namespace IO::audio
{
        SFX::SFX(string Name)
        {
            sound = nullptr;
            name = Name;
        }

        //destructor
        SFX::~SFX()
        {
            unload();//forward to this unloading function
        }
        //Move constructor, copy the data from that sound into this sound, then delete that sound safely
        SFX::SFX(SFX&& that)
        {
            name = that.name;
            sound = that.sound;
            that.sound = nullptr;//Important! if we did not set this to null, that sound would call its own destructor and delete the sound we just copied
        }

        //The name is used to manage which sounds are already loaded
        void SFX::set_name(string n)
        {
            name = n;
        }

        string SFX::get_name()
        {
            return name;
        }



        bool SFX::is(const string& S) { return (S.compare(name) == 0); }//Used for checking if a sound has already been loaded

        //check if this sound has not been unloaded
        bool SFX::is_good() { return sound != nullptr; }

        //TBD, the argument sounds should be read as a namespace variable
        void SFX::loadWAV(const my_path& sounds)
        {
            unload();
            my_path SND = sounds / my_path(name + ".wav");
            //SDL wants this to be a waveform sound
            sound = Mix_LoadWAV(SND.String().c_str());
        }

        void SFX::unload()
        {
            if (sound != nullptr)
                Mix_FreeChunk(sound);
            sound = nullptr;
        }

        //Play, and return what channel we are playing at
        int SFX::play()
        {
            //-1 is a flag to use any available channel (if any)
            if (sound != nullptr)
                return Mix_PlayChannel(-1, sound, 0);
            return 0;//Will likely be ignored
        }
}
