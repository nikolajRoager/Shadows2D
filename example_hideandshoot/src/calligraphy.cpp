/*THIS FILE
//A single unicode character lookup section for a section of unicode, this refers to a false animation, where each frame is actually a letter, we can use this to turn arbitrary unicode into pixelated textures

Unicode essentially assigns a number to every single letter, symbol or emoji in pretty much every alphabet ever created, all c++ strings essentially are, are lists of such numbers; to turn these lists into a texture, we want to render each letter next to one another, and for that, we need a class which can translate the numbers into symbols, and this is what this is.

This class supports PIXELATED text, with the same resolution as the rest of the program, this text is loaded as a png image. SDL2 does support true-type fonts (vector graphics) but vector graphics font don't look good when they are pixelated.

I obviously did not make every unicode character, but this class allows us to define whatever sections of unicode we want to use.
*/

#include "calligraphy.hpp"
#include "IO.hpp"





namespace IO::graphics
{

   calligraphy::calligraphy(uint min, uint max, uint c, uint r, tex_index _t, uchar mono_w, uchar h_space, vector<pair<uint, uchar> >& drop, vector<uchar >& gw)
    {
        texture = _t;
        unicode_min = min;
        unicode_max = max;
        cols = c;
        rows = r;
        baseline_drop = std::move(drop);
        mono_width = mono_w;
        text_h_space = h_space;

        glyph_width = std::move(gw);
    }
    //Since this is in a vector, we need a move constructor
    calligraphy::calligraphy(calligraphy&& T)
    {
        texture = T.texture;
        T.texture = -1;//Unset, to avoid deleting this
        unicode_min = T.unicode_min;
        unicode_max = T.unicode_max;
        cols = T.cols;
        rows = T.rows;
        baseline_drop = std::move(T.baseline_drop);
        glyph_width = std::move(T.glyph_width);
        mono_width = T.mono_width;
        text_h_space = T.text_h_space;


    }
    calligraphy& calligraphy::operator=(calligraphy&& T)
    {
        texture = T.texture;
        T.texture = -1;//Unset, to avoid deleting this
        unicode_min = T.unicode_min;
        unicode_max = T.unicode_max;
        cols = T.cols;
        rows = T.rows;
        baseline_drop = std::move(T.baseline_drop);
        glyph_width = std::move(T.glyph_width);
        mono_width = T.mono_width;
        text_h_space = T.text_h_space;
        return *this;
    }

    calligraphy::~calligraphy()
    {

        if (texture != (tex_index )-1)
            IO::graphics::delete_tex(texture);
    }

    uchar calligraphy::get_glyph_width(uint id) const
    {
        if (mono_width != (uchar)-1)
        {
            return mono_width + text_h_space;
        }
        else
        {
            return glyph_width[id] + text_h_space;
        }
    }
};
