
#include "texwrap.hpp"
#include "graphicus.hpp"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include<iostream>
#include<algorithm>
#include <string>
#include <fstream>


//Load from file
texwrap::texwrap(const string& msg,TTF_Font *my_font,float m_per_px)
{
    message = msg;
    is_text = true;
    reload(my_font);
    local_transform= scale(vec3(m_per_px*px_w,m_per_px*px_h,1));
}

texwrap::texwrap(const fs::path& _path,float m_per_px)
{
    is_text = false;
    path = _path;
    reload();

    local_transform= scale(vec3(m_per_px*px_w,m_per_px*px_h,1));
}


void texwrap::reload(TTF_Font* my_font)//Redo the loading, in case some system wide settings changed, we are reloading frome file, because this is also intended to be used in developer mode, where the files may change, a lot
{


    //Loading png is harder than jpg, because there are more options in the same format, i.e. do we use 16 or 8 bit pixels, do we have alpha? Oh well, lets do it anyway

        //Load to a simple SDL texture; SDL knows what to do with any file format; it can itself figure out what format we are using
    SDL_Surface* texture_surface = nullptr;

    if (is_text)
    {

        SDL_Color Color = {255,0,0,255};

        texture_surface=TTF_RenderUTF8_Blended(my_font,message.c_str(),Color);
    }
    else
        texture_surface=IMG_Load(path.c_str());

    if(texture_surface == NULL)
    {
        throw ("Couldn't load texture "+path.string()+" to surface"+string(IMG_GetError()));
    }

    px_w=texture_surface->w;
    px_h=texture_surface->h;

    //Now let us try to figure out what on earth this image is; if this is jpg we know that the image format is 8 bit GL_RGB, but if this is PNG it is anybodies guess. 16 bit png won't work, sorry.

    GLenum image_format;//What data is used by the image file
    GLenum GL_format;//And what should our target data be

    //Does this do Alpha?
    if (texture_surface->format->BytesPerPixel == 4)
    {
        image_format = GL_RGBA;
        GL_format = GL_RGBA;//I assume that if the image has an alpha channel, it is meant to be there
    } else
    {
        image_format = GL_RGB;
        GL_format = GL_RGB;
    }//I am going to be using png and jpg, formats which do RGB not BGR so I won't even consider that


    //This is our destination
    glGenTextures(1, &(textureID));
    glBindTexture(GL_TEXTURE_2D,textureID);
    glTexImage2D
    (
        GL_TEXTURE_2D,//What texture type is this
        0,//Mipmap-Level (We auto-generate them later)
        GL_format,//internal format for graphics card
        texture_surface->w,//Width/height in pixels
        texture_surface->h,
        0,
        image_format,       //Format of input
        GL_UNSIGNED_BYTE,//Type of data
        texture_surface->pixels //A pointer to the actual data,
    );


    glGenerateMipmap(GL_TEXTURE_2D);//ABSOLUTELY DO THIS, AS WE MAY ZOOM FAR OUT
    //Now set some more parameters, we want the texture to repeat (for interpolation purpose)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    //Get best alignment for pixel storage, it is the largest element in {1,2,4,8} which divides the pitch of the texture
    uchar align = 8;
    while (texture_surface->pitch%align) align>>=1;
    //FASTER binary notation for align/=2, (shift every bit one right).
    //For a texture with power of two width/height greater than 8, I sure hope that align=8.

    glPixelStorei(GL_UNPACK_ALIGNMENT,align);
    //Doing this may result in a very small performance boost

    //Sure, we "free" the surface (We all know the surface is dead though)
    SDL_FreeSurface( texture_surface );


}

//In some circumstances, the same word repeated a number of time is a valid sentence in some languages... in C++, this happens in the case of move constructor
texwrap::texwrap(texwrap&& T)
{
    path = T.path;
    message = T.message;
    is_text = T.is_text;
    px_w=T.px_w;
    px_h=T.px_h;
    //Quietly yank the pointer
    textureID = T.textureID;
    local_transform= T.local_transform;
    T.textureID=-1;
}
texwrap& texwrap::operator=(texwrap&& T)
{

    if (textureID!=(GLuint)-1 )
        destroy();

    path = T.path;
    message = T.message;
    is_text = T.is_text;
    px_w=T.px_w;
    px_h=T.px_h;
    //Quietly yank the pointer
    textureID = T.textureID;
    T.textureID=-1;

    local_transform= T.local_transform;
    return *this;
}


//Same as destructor, except leaves the object intact (maybe we are in a vector, and we don't want to throw of the ordering)
void texwrap::destroy()
{
    if (textureID != (GLuint)-1)
    {
        glDeleteTextures(1,&textureID);
        textureID=-1;
    }
}

texwrap::~texwrap()
{
    destroy();
}

