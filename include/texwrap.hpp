/*THIS FILE
//This is a texture wrapper class, which allows our graphical program to easilly interface with SDL textures, this class encodes both text and textures, as they behind the scene are the same (only the one is loaded from image files, the other is generated from text) SHOULD ONLY BE USED BY IO::GRAPHICS, NOONE ELSE SHOULD USE IT
*/

#pragma once

//Need GLuint
#include <GL/glew.h>

#include <string>
#include <vector>
#include"my_filesystem.hpp"
#include<cstdint>
#include<iostream>

#include <glm/glm.hpp>
#include"IO.hpp"

using namespace std;
using namespace glm;
using uchar = uint8_t;
using uint = uint32_t;
using ulong = uint64_t;


namespace IO::graphics
{
    class texwrap
    {
    private:
        my_path path;//For reloading and for being identified
        string message;//If this is actually a piece of text

        uint texture_w;
        uint texture_h;

        uint frame_w;
        uint frame_h;
        //Local scaling to keep pixels the same size
        mat3 local_transform;
        mat3 CenterM;//Matrix required to center this
        mat3 CornerM;//Matrix required to go to bottom left corner

        vector<mat3> anim_clip;//local transform of the UV coordinates for every frame

        GLuint textureID = -1;

        uint n_frames;

        bool is_text;


    public:
        uint get_w() const { return texture_w; }
        uint get_h() const { return texture_h; }


        uint get_frame_w() const { return frame_w; }
        uint get_frame_h() const { return frame_h; }

        bool exists() const { return textureID != (GLuint)-1; }//Is this texture initialized or is it not? that is the question! and on this question, the fate of the entire application depends.

        string tell_name(bool full = false) const
        {
            if (!is_text)
            {

                if (full)
                    return path.String();
                else
                    return path.filename().String();
            }
            else
                return message;
        }

        texwrap(const my_path& path);
        texwrap(const string& message);

        void reload();//Redo the loading, in case some system wide settings changed

        bool is(const string& msg) const
        {

            return  is_text && msg.compare(message) == 0;//I sure don't hope you asked if a texture was this text, in that case something has gone awfully wrong
        }

        bool is(my_path& Path) const {
            if (!myfs_exists(Path))//Fail safe, fs::equivalent assumes that the paths exists, and fails catastrophically if they don't (program crash with an error which can not be caught, and which does not produce an error message).
                throw std::runtime_error("Texture file not found: "+Path.String());

            //To test the "loading" page, uncomment this line, this slows down the program immensely (on Windows at least, Linux does not seem to care)
            //cout <<"Comparing "<<Path.String()" to "<<path.String()<<endl;

            return myfs_equivalent(path, Path); }

        ~texwrap();

        texwrap(texwrap&&);//Bad stuff happens if there is not a custom move constructor (specifically, the destructor is called everytime the array of textures need to be enlarged and that frees the textures, causing catastrophic failure)
        texwrap& operator=(texwrap&&);

        //Draw the Texture here, centered = centered around x and with y at the lowest point of the image, otherwise top left corner, if frame is provided 'tis an animation
        GLuint get_tex() const { return textureID; }

        void destroy();//Same as destructor, but leaves the class as is corpse (maybe we need it to remain to not mess up the ordering of a list)

        mat3 getM(bool mirror = false, bool center = false) const;
        const mat3 getUVM(uint frame) const;

        void set_animation(uint n_frames_w, uint n_frames_h);


    };
}
