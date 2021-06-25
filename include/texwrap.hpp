#ifndef TEXWRAP
#define TEXWRAP

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <vector>
#include<filesystem>
#include<cstdint>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
//This is a texture wrapper class, which allows our graphical program to easilly interface with SDL textures
//I have, needles to say, not missed the opportunity to name this class something which sounds like food (texwrap= a wrap from texas I guess)

using namespace std;
using namespace glm;
using uchar = uint8_t;
using ushort= uint16_t;
using uint = uint32_t;
using ulong = uint64_t;

namespace fs = std::filesystem;

class texwrap
{
private:
    fs::path path;//For reloading and for being identified
    string message;//If this is actually a piece of text

    ushort px_w;
    ushort px_h;

    mat4 local_transform;

    GLuint textureID=-1;

//    ushort n_frames;

    bool is_text;


public:
        int get_w() const {return px_w;}
        int get_h() const {return px_h;}


        bool exists() const {return textureID!=(GLuint) -1;}//Is this texture initialized or is it not? that is the question! and on this question, the fate of the entire application depends.

        string tell_name() const
        {
            if (!is_text)
            {

                return path.filename().string();
            }
            else
                return message;
        }

        texwrap(const fs::path& path,float px_per_meter);
        texwrap(const string& message,TTF_Font *my_font,float px_per_meter);

        void reload(TTF_Font *my_font=nullptr);//Redo the loading, in case some system wide settings changed


        bool is (fs::path& Path) const {return fs::equivalent(path,Path);}

        ~texwrap();

        texwrap(texwrap&&);//Bad stuff happens if there is not a custom move constructor (specifically, the destructor is called everytime the array of textures need to be enlarged and that frees the textures, causing catastrophic failure)
        texwrap& operator=(texwrap&&);

        //Draw the Texture here, centered = centered around x and with y at the lowest point of the image, otherwise top left corner, if frame is provided 'tis an animation
        GLuint get_tex() const {return textureID;}

        void destroy();//Same as destructor, but leaves the class as is corpse (maybe we need it to remain to not mess up the ordering of a list)

        const mat4& getM() const {return local_transform;}

};

#endif
