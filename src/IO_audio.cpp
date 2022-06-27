/*THIS FILE

The definition for the IO::audio namespace, which handles sound effects, can be used to play looping or one-time sound effects.:w
*/

//Main SDL library
#include <SDL2/SDL.h>
//SDL sound
#include <SDL2/SDL_mixer.h>

#include "IO.hpp"

//std string
#include <string>
//text output, should never be used in looping functions
#include <iostream>
//operating independent file-system functions
#include<filesystem>
//file streams, we use filesystem for paths, and fstream to actually stream the data from files in.
#include <fstream>

//Dynamic sized list
#include <vector>

//Not so dynamic sized lists
#include <array>

//Hopefully there will not be more than 16 loops
#define MAX_LOOP_SOUNDS 16


#include"SFXwrap.hpp"

namespace IO::audio
{
    // --- internal variables and constants ---
    fs::path sound_path;

    //Same procedure as with textures: the sound effects themself, how many people use them, and how many there are
    vector<SFX> sounds;
    vector<uint> sounds_users;
    tex_index sounds_size=0;


    //-- Looping sounds --
    //What sound (ID) is looping at what channel? and should it be renewed
    struct sound_loop
    {
        tex_index ID=-1;
        int channel;
        bool renew = false;

    };
    array<sound_loop,  MAX_LOOP_SOUNDS> sound_loops;
    // ---- one time functions ----
    void init(fs::path audio)
    {
        sound_path = audio;
        //Initialize sound
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
        {//If it did not work, return error
            throw std::runtime_error(Mix_GetError());
        }

    }

    void end()
    {

        bool orphaned_res;
        for (tex_index i = 0; i < sounds_size; ++i)
        {
            tex_index I =sounds_users[i];
            if (I>0)
            {
                cout<<I<<" users | sound : \""<<sounds[i].get_name()<<"\""<<endl;
                orphaned_res=true;
            }
        }

        if (!orphaned_res)
            cout<<"All sound already unloaded"<<endl;
        sounds = vector<SFX>();
        sounds_users =vector<uint>();
        sounds_size=0;

    }

    // ---- Functions which must be called once per loop ----
    void update_loops()//Update looping audio
    {

        //now check which looping sounds should be played
        for (sound_loop& s : sound_loops)
        {//reset all currently active sounds
            if(s.ID != (tex_index ) -1)
            {

                if(s.renew)
                {
//check if sound has stopped playing, if yes, restart it
                    if (!Mix_Playing(s.channel))
                    {
                        s.channel = sounds[s.ID].play();

                    }
                    s.renew=false;//If no play command is sent until we get here again, then the sound will be stopped
                }
                else
                {
//do not renew this sound? then delete it
                    Mix_HaltChannel(s.channel);
                    delete_sound(s.ID);
                    s.ID = -1;//now this is unset

                }

            }
        }


        for (sound_loop& S : sound_loops)
        {//Reset all currently looping sounds (if they are not refreshed each loop they will not be renewed
            S.renew=false;
        }
    }

    // --- Sound loading ---
    //TBD should be moved to another namespace
    tex_index load_sound(string name)//Load this sound effect, and return its ID
    {

        for (tex_index s = 0; s < sounds_size; ++s)
        {
            SFX& S = sounds[s];
            if (S.is(name))
            {
                ++sounds_users[s];//Now this is used
                return s;
            }
        }
        //no catch here ... errors may be thrown, but they should be handled by the idiot who asked for a non-existing sound
        //Ok, now look for empty spaces to put this

        for (tex_index s = 0; s < sounds_size; ++s)
        {
            if (sounds_users[s]==0)
            {
                sounds[s].unload();
                sounds[s].set_name(name);
                sounds[s].loadWAV(sound_path);
                sounds_users[s]=1;
                return s;
            }
        }

        sounds.push_back(name);
        sounds[sounds_size].loadWAV(sound_path);
        if (!sounds[sounds_size].is_good())
        {
            end();
            throw std::runtime_error(string("Could not load audio file " + name));
        }
        sounds_users.push_back(1);//Now this is used
        ++sounds_size;
        return sounds_size-1;


        return -1;
    }
    void delete_sound(tex_index ID)//Same procedures as with textures
    {
        if (ID < sounds_size)
        {
            if (sounds_users[ID] == 0)
                cout<<"WARNING Double freed sound "<<sounds[ID].get_name()<<endl;
            else
            {
                --sounds_users[ID];
            }

            //Now, delete anything not in use IGNORE COMPILER WARNINGS IN VISUAL STUDIO, I INTENTIONALLY USE UNDERFLOW
            for (tex_index I = sounds_size - 1; I < sounds_size; --I)
            {
                if (sounds_users[I]!=0)
                {
                    break;
                }
                else
                {
                    sounds_users.pop_back();
                    sounds.pop_back();
                    --sounds_size;
                }
            }

        }

    }
    void add_sound_user(tex_index ID)
    {
        if (ID <sounds_size )
        {
            ++sounds_users[ID];
        }

    }

    // ---- Play sound functions ----
    void play_sound(tex_index ID)//"Display" this sound effect
    {
        if (ID != (tex_index )-1)
            sounds[ID].play();
    }

    void loop_sound(tex_index ID)//Keep looping this sound as long as this is called every frame, for practical reasons we can keep at most 16 sounds looping at once
    {
        if (ID != (tex_index )-1)
        {
            //Check if sound is already looping

            uchar vacant_loop = -1;//Are any loops vacant?
            bool renewed = false;
            for (uchar i = 0; i < MAX_LOOP_SOUNDS; ++i)
            {
                sound_loop& S = sound_loops[i];
                if (S.ID==ID)
                {
                    S.renew = true;
                    renewed = true;
                    break;
                }
                else if (S.ID == (tex_index )-1 && vacant_loop== (uchar)-1 )
                    vacant_loop = i;
            }

            //This is a new looping sound we are adding, and we have room for it, ok
            if (!renewed && vacant_loop != (uchar)-1)
            {

                sound_loop& S = sound_loops[vacant_loop];
                S.channel = sounds[ID].play();
                if (S.channel != -1)
                {
                    S.ID=ID;
                    add_sound_user(S.ID);
                    S.renew = true;
                }
                //If, for whatever reason, the sound could not play, don't renew it, this also guarantees that we will not later deal with channel = -1
            }

        }

    }

    //---Print ram ----
    void print_ram()//Print currently loaded sound effects to the external terminal (usually doesn't fit in the developer commandline)
    {
        cout<<"--Sounds --"<<endl;
        cout<<" ID   | Users | name"<<endl;
        for (tex_index i = 0; i < sounds_size; ++i)
        {
            cout<<' '<<i;

            if (i<10)
                cout<<' ';
            if (i<100)
                cout<<' ';
            if (i<1000)
                cout<<' ';
            cout<<" | ";

            tex_index I =sounds_users[i];
            cout<<I;

            if (I<10)
                cout<<' ';
            if (I<100)
                cout<<' ';
            if (I<1000)
                cout<<' ';
            cout<<"  | ";
            cout<<sounds[i].get_name();

            cout<<endl;
        }

    }

}
