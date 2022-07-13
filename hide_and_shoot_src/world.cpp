#include "world.hpp"

world::world(fs::path world_folder)
{
    textures.push_back(IO::graphics::load_tex(world_folder/"level0.png",w,h));
}

world::~world()
{
    for (tex_index T : textures)
        IO::graphics::delete_tex(T);
}

void world::display(int position_x,int position_y) const
{
    //int offset_x = offset_x > w ? w : offset_x;
    IO::graphics::draw_tex(-0*position_x,-0*h/2-0*position_y,textures[0],false,false);
}
