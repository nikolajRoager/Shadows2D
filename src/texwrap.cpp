/*THIS FILE
//Definition for the texture wrapper class, which allows our graphical program to easilly interface with SDL textures, this class encodes both text and textures, as they behind the scene are the same (only the one is loaded from image files, the other is generated from text)
*/


#include "texwrap.hpp"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include<iostream>
#include<algorithm>
#include <string>
#include <fstream>


//By pre-declaring these normally hidden functions, I will be able to use them (Putting this here is functionally identical to writing a tiny graphics_extension.hpp file) the linker will dutifully link these to their definition
namespace IO::graphics
{
    //Get width and height of internal display in pixels
    //this is NOT the screen size if the display is pixelated
    //TBD, consider if this need to be public
    uint get_w();
    uint get_h();
    //Get 1/w and 1/h
    float get_inv_w();
    float get_inv_h();
    GLuint string_to_texture(string message, uint& this_w, uint& this_h, bool bgr=false);
}

#include"IO.hpp"

namespace IO::graphics
{

    //Load from file
    texwrap::texwrap(const string& msg)
    {
        message = msg;
        is_text = true;
        reload();

        //Get the inverse of the width and height in pixels, so that we can scale the image correctly
        float inv_screen_w = get_inv_w();
        float inv_screen_h = get_inv_h();

        n_frames = 1;//There is only one frame (text is not animated)
        anim_clip = vector<mat3>(n_frames);

        //The scaling of the image in order to make it have the right dimensions in normalized device coordinates.
        //The first non-zero element scales x, the next scaled y, the final scales z (but z is our false component which always must be 1 for vertices so nevermind that)
        local_transform =
            mat3(
                vec3(2 * inv_screen_w * texture_w, 0, 0),
                vec3(0, 2 * inv_screen_h * texture_h, 0),
                vec3(0, 0, 1)
            );
        //Transformation required to make the rectangle be centered, the translation of the x and y component are the x and y components of the final vector (which is actually the final column, not row), we need to move half the width of this frame, the width of this frame in pixel is the texture width , but this translation is done in normalized device coordinates which goes from -1 to 1, so there are 2/(pixels the screen is wide) NDC units per pixel, and we want to move  (texture_w )/2 pixels. Giving our resulting translation


        CenterM =
            mat3(
                vec3(1.f, 0.f, 0.f),
                vec3(0.f, 1.f, 0.f),
                vec3(-inv_screen_w * (texture_w+(texture_w%2)), -inv_screen_h *2* (texture_h), 1.f));//Texture width height should be divisible by 2, if it is not we add 1 (%2 means remainder by division with 2, it is 1 for odd numbers and 0 for even numbers)

        CornerM =
            mat3(
                vec3(1.f, 0.f, 0.f),
                vec3(0.f, 1.f, 0.f),
                vec3(0, -inv_screen_h *2* (texture_h), 1.f));//By default this would go to the top left corner, I think bottom left is more intuitive

        //When I say Center, I mean center on the x axis and be at the bottom in the y axis

        frame_w = texture_w;
        frame_h = texture_h;

    }

    texwrap::texwrap(const fs::path& _path)
    {
        is_text = false;
        path = _path;

        reload();

        n_frames = 1;//Non animated by default
        anim_clip = vector<mat3>(n_frames);

        //Get the inverse of the width and height in pixels, so that we can scale the image correctly
        float inv_screen_w = get_inv_w();
        float inv_screen_h = get_inv_h();

        local_transform =
            mat3(
                vec3(2 * inv_screen_w * texture_w, 0, 0),
                vec3(0, 2 * inv_screen_h * texture_h, 0),
                vec3(0, 0, 1));
        //Transformation required to make the rectangle be centered, the translation of the x and y component are the x and y components of the final vector (which is actually the final column, not row), we need to move half the width of this frame, the width of this frame in pixel is the texture width , but this translation is done in normalized device coordinates which goes from -1 to 1, so there are 2/(pixels the screen is wide) NDC units per pixel, and we want to move  (texture_w )/2 pixels. Giving our resulting translation
        CenterM =
            mat3(
                vec3(1.f, 0.f, 0.f),
                vec3(0.f, 1.f, 0.f),
                vec3(-inv_screen_w * (texture_w+(texture_w%2)), -inv_screen_h * 2*(texture_h), 1.f));//Texture width height should be divisible by 2, if it is not we add 1 (%2 means remainder by division with 2, it is 1 for odd numbers and 0 for even numbers)

        CornerM =
            mat3(
                vec3(1.f, 0.f, 0.f),
                vec3(0.f, 1.f, 0.f),
                vec3(0, -inv_screen_h *2* (texture_h), 1.f));//By default this would go to the top left corner, I think bottom left is more intuitive


        frame_w = texture_w;
        frame_h = texture_h;
    }


    void texwrap::reload()//Redo the loading, in case some system wide settings changed, we are reloading frome file, because this is also intended to be used in developer mode, where the files may change, a lot
    {

        if (textureID != (GLuint)-1)
            destroy();

        //Loading png is harder than jpg, because there are more options in the same format, i.e. do we use 16 or 8 bit pixels, do we have alpha? Oh well, lets do it anyway

        //Load to a simple SDL texture; SDL knows what to do with any file format; it can itself figure out what format we are using
        SDL_Surface* texture_surface = nullptr;

        if (is_text)
        {
            if (message.length() == 0)
                message = " ";
            //What kind of idiot would ask for a message with 0 length ... me apparantly sometimes that happens in the developer command prompt, just make it a blank space then
            //Render this to a texture using OpenGL
            textureID = string_to_texture(message, texture_w, texture_h, true);
        }
        else
        {
            texture_surface = IMG_Load(path.string().c_str());


            if (texture_surface == NULL)
            {
                cout <<"THERE WAS AN ERROR loading a texture, if \""<< path.string().c_str() << "\" is different from \""<<path.string()<<"\", then something has gone wrong in the port, and it is all Microsoft's fault; if not look at the below error message"<<endl;
                throw std::runtime_error("Couldn't load texture " + path.string() + " to surface: SDL returned error: " + string(IMG_GetError()));
            }

            texture_w = texture_surface->w;
            texture_h = texture_surface->h;

            //Now let us try to figure out what on earth this image is; if this is jpg we know that the image format is 8 bit GL_RGB, but if this is PNG it is anybodies guess. 16 bit png won't work, sorry.

            GLenum image_format;//What data is used by the image file
            GLenum GL_format;//And what should our target data be

            //Does this do Alpha?
            if (texture_surface->format->BytesPerPixel == 4)
            {
                image_format = GL_RGBA;
                GL_format = GL_RGBA;//I assume that if the image has an alpha channel, it is meant to be there
            }
            else
            {
                image_format = GL_RGB;
                GL_format = GL_RGB;
            }//I am going to be using png and jpg, formats which do RGB not BGR so I won't even consider that


            //This is our destination
            glGenTextures(1, &(textureID));
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D
            (
                GL_TEXTURE_2D,//What texture type is this
                0,//Mipmap-Level, this project uses low-resolution textures intentionally
                GL_format,//internal format for graphics card
                texture_surface->w,//Width/height in pixels
                texture_surface->h,
                0,
                image_format,       //Format of input
                GL_UNSIGNED_BYTE,//Type of data
                texture_surface->pixels //A pointer to the actual data,
            );

            //This is the place to glGenerateMipmap(GL_TEXTURE_2D);
            //But for the purpose of this project I do not want that

            //Now set some more parameters,
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            //Get best alignment for pixel storage, it is the largest element in {1,2,4,8} which divides the pitch of the texture
            uchar align = 8;
            while (texture_surface->pitch % align) align >>= 1;
            //FASTER binary notation for align/=2, (shift every bit one right).
            //For a texture with power of two width/height greater than 8, I sure hope that align=8.

            glPixelStorei(GL_UNPACK_ALIGNMENT, align);
            //Doing this may result in a very small performance boost, I am not sure it matters though ... no wait, just checked, it literally does not work without

            SDL_FreeSurface(texture_surface);

        }



    }

    //In some circumstances, the same word repeated a number of time is a valid sentence in some languages... in C++, this happens in the case of move constructor
    texwrap::texwrap(texwrap&& T)
    {
        n_frames = T.n_frames;

        path = T.path;
        message = T.message;
        is_text = T.is_text;
        texture_w = T.texture_w;
        texture_h = T.texture_h;
        frame_w = T.frame_w;
        frame_h = T.frame_h;
        //Quietly yank the pointer
        textureID = T.textureID;
        local_transform = T.local_transform;
        CenterM = T.CenterM;
        CornerM= T.CornerM;
        T.textureID = -1;
        anim_clip = std::move(T.anim_clip);


    }
    texwrap& texwrap::operator=(texwrap&& T)
    {

        if (textureID != (GLuint)-1)
            destroy();

        n_frames = T.n_frames;
        path = T.path;
        message = T.message;
        is_text = T.is_text;
        texture_w = T.texture_w;
        texture_h = T.texture_h;
        frame_w = T.frame_w;
        frame_h = T.frame_h;
        //Quietly yank the pointer
        textureID = T.textureID;
        T.textureID = -1;

        local_transform = T.local_transform;
        CenterM = T.CenterM;
        CornerM= T.CornerM;
        anim_clip = std::move(T.anim_clip);
        return *this;
    }


    //Same as destructor, except leaves the object intact (maybe we are in a vector, and we don't want to throw of the ordering)
    void texwrap::destroy()
    {
        if (textureID != (GLuint)-1)
        {
            glDeleteTextures(1, &textureID);
            textureID = -1;
        }
    }

    texwrap::~texwrap()
    {
        destroy();
    }

    //Set up an animated sprite
    void texwrap::set_animation(uint n_frames_w, uint n_frames_h)
    {

        n_frames = n_frames_w * n_frames_h;


        float inv_screen_w = get_inv_w();
        float inv_screen_h = get_inv_h();

        //The scaling of the image in order to make it have the right dimensions in normalized device coordinates.
        //The first non-zero element scales x, the next scaled y, the final scales z (but z is our false component which always must be 1 for vertices so nevermind that)
        local_transform =
            mat3(
                vec3(2 * inv_screen_w * (texture_w / n_frames_w), 0, 0),
                vec3(0, 2 * inv_screen_h * (texture_h / n_frames_h), 0),
                vec3(0, 0, 1));


        //Transformation required to make the rectangle be centered, the translation of the x and y component are the x and y components of the final vector (which is actually the final column, not row), we need to move half the width of this frame, the width of this frame in pixel is the texture width divided by the number of frames side by side, but this translation is done in normalized device coordinates which goes from -1 to 1, so there are 2/(pixels the screen is wide) NDC units per pixel, and we want to move  (texture_w / n_frames_w)/2 pixels. Giving our resulting translation
        CenterM =
            mat3(
                vec3(1.f, 0.f, 0.f),
                vec3(0.f, 1.f, 0.f),
                vec3(-inv_screen_w * (texture_w / n_frames_w), -  inv_screen_h *2* (texture_h / n_frames_h), 1.f));

        CornerM =
            mat3(
                vec3(1.f, 0.f, 0.f),
                vec3(0.f, 1.f, 0.f),
                vec3(0, -inv_screen_h *2* (texture_h/n_frames_h), 1.f));//By default this would go to the top left corner, I think bottom left is more intuitive
        //Now, we need to generate the matrices to transform the texture coordinates, so that we can pick out each frame exactly
        anim_clip = vector<mat3>(n_frames);


        //We scale the image slightly if for instance we want 3 frames in a 16 pixel wide image, the image will be scaled to 15 pixel. The final column of pixels will simply be ignored then

        float fix_scale_x = (texture_w - texture_w % n_frames_w) / float(texture_w * n_frames_w);
        float fix_scale_y = (texture_h - texture_h % n_frames_h) / float(texture_h * n_frames_h);
        //In the majority of cases the width and height of the texture is a multiple of the pixel width/height per frame

        frame_w = (texture_w) / (n_frames_w);
        frame_h = (texture_h) / (n_frames_h);

        for (uint i = 0; i < n_frames_h; ++i)
            for (uint j = 0; j < n_frames_w; ++j)
            {
                //This is really a scaling matrix times a translation matrix, but I know how matrix multiplication work so here is the result:
                anim_clip[j + i * n_frames_w] =
                    mat3(
                        vec3(fix_scale_x, 0, 0),
                        vec3(0, fix_scale_y, 0),
                        vec3(j * fix_scale_x, i * fix_scale_y, 1)
                    );

            }
        anim_clip.shrink_to_fit();

    }
    mat3 texwrap::getM(bool mirror, bool center) const
    {
        //The total matrix is found by multiplying the seperate things we might want to do. In general we actually need to multiply the matrices from right to left.
        mat3 Re = local_transform;
        if (center)//Use the matrix for centering this
            Re = CenterM * Re;
        else
            Re = CornerM* Re;

        if (mirror)//Scale by -1 along the x axis
            Re = mat3(vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)) * Re;
        return Re;
    }

    const mat3 texwrap::getUVM(uint frame) const
    {
        if (n_frames != 0 && n_frames != 1)
            return anim_clip[frame % n_frames];
        else
            return mat3(1);
    }
}
