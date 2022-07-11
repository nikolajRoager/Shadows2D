#pragma once
/*THIS FILE
//A single unicode character lookup section for a section of unicode, this refers to a false animation, where each frame is actually a letter, we can use this to turn arbitrary unicode into pixelated textures

Unicode essentially assigns a number to every single letter, symbol or emoji in pretty much every alphabet ever created, all c++ strings essentially are, are lists of such numbers; to turn these lists into a texture, we want to render each letter next to one another, and for that, we need a class which can translate the numbers into symbols, and this is what this is.

This class supports PIXELATED text, with the same resolution as the rest of the program, this text is loaded as a png image. SDL2 does support true-type fonts (vector graphics) but vector graphics font don't look good when they are pixelated.

I obviously did not make every unicode character, but this class allows us to define whatever sections of unicode we want to use.
*/

#include<cstdint>
#include<vector>

//I use these types so much that these aliases are well worth it
using ulong = uint64_t;
using uint = uint32_t;
using tex_index = uint16_t;
using uchar = uint8_t;

using namespace std;

namespace IO::graphics
{
    struct calligraphy
    {
        //Image containing unicode character table
        tex_index texture = -1;//Loaded into the ordinary texture list because why not
        uint unicode_min = 0x0020;
        uint unicode_max = 0x024F;
        uint cols = 51;
        uint rows = 14;



        vector<pair<uint, uchar> > baseline_drop;//Some characters are dropped a few pixels below the baseline, this list tells us which characters drop how much

        //If monospaced, this is the width used
        uchar mono_width = 5;//Set to -1 to get non monospaced
        uint text_h_space = 1;

        //If monospacing is turned off, this might not be the same,
        vector<uchar > glyph_width;


        calligraphy(uint min, uint max, uint c, uint r, tex_index _t, uchar mono_w, uchar h_space, vector<pair<uint, uchar> >& drop, vector<uchar >& gw);
        calligraphy(calligraphy&& T);
        calligraphy& operator=(calligraphy&& T);
        ~calligraphy();

        //Get how wide this thing is, note that the font is not always monospaced
        uchar get_glyph_width(uint id) const;
    };
}
