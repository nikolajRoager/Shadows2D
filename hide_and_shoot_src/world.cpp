#include "world.hpp"

world::world(my_path world_folder)
{
    //Read the basic textures
    bottom_textures = IO::graphics::load_tex(world_folder/"level0.png",w,h,true);
    top_textures = IO::graphics::load_tex(world_folder/"level1.png",true);
    try
    {
        walk_tile_texture = IO::graphics::load_tex(world_folder/"tile.png",true);
    }
    catch(...)
    {
        //Whatever, this is just for debugging
        walk_tile_texture = -1;
    }

    ifstream walk_grid_file ( (world_folder/"walk_grid.txt").String());

    if (!walk_grid_file.is_open())
        throw std::runtime_error("File not readable: "+(world_folder/"walk_grid.txt").String());

    if (!(walk_grid_file>>grid_m))
        throw std::runtime_error("File : "+(world_folder/"walk_grid.txt").String()+" did not contain grid width");
    if (!(walk_grid_file>>grid_n))
        throw std::runtime_error("File : "+(world_folder/"walk_grid.txt").String()+" did not contain grid height");

    if (w %grid_m != 0 ||h %grid_n != 0)
        throw std::runtime_error("The image, with size : "+to_string(w)+"/"+to_string(h)+" is not divisible by the grid size : "+to_string(grid_m)+"/"+to_string(grid_n));

    tile_w = w/grid_m;
    tile_h = h/grid_n;


    walkable_grid = vector<bool>(grid_m*grid_n);

    for (uint j = 0; j < grid_n; ++j)
        for (uint i = 0; i < grid_m; ++i)
        {
            uint temp = 0;
            if (! (walk_grid_file>>temp))
                throw std::runtime_error("File : "+(world_folder/"walk_grid.txt").String()+" did not contain enough legal entries to fill the grid ("+to_string(grid_m)+"/"+to_string(grid_n)+")");
            walkable_grid[j*grid_m+i]= (temp == 1 ? false : true);
        }

    walk_grid_file.close();

    polygon_file=world_folder/("polygons.bin");

    {
        if (myfs_exists(polygon_file))//Check if the polygon file is there
        {//We could do some more tests, i.e. check for readability, but lets just load it, and let it throw an exception if it is not valid
            std::ifstream IN(polygon_file.String(), std::ios::binary);//Binary files are easier to read and write

            if (IN.is_open())//Check if the polygon file is there
            {//We could do some more tests, i.e. check for readability, but lets just load it, and let it throw an exception if it is not valid

                try
                {
                    uint meshes=0;
                    IN.read((char*)&meshes,sizeof(uint));
                    cout<<"Reading "<<meshes<<" meshes"<<endl;
                    for (uint i = 0; i < meshes; ++i)
                    {
                        //See! this is why I use binary! it is that easy to write!, no need to getline or stream into a temporary oject, and then add that to the vector one at the time, just yoink the entire thing
                        uint size=0;
                        IN.read((char*)&size,sizeof(uint));//I know I am working with same size integers here, since I use cstdint
                        if (size>0 )
                        {
                            vector<vec2> vertices(size);
                            IN.read((char*)&vertices[0],size*sizeof(vec2));
                            mess.push_back(mesh2D(vertices));
                        }
                    }
                    IN.close();
                }
                catch(exception& e)
                {//Whatever
                    std::cout<<"Error while loading"<<e.what()<<endl;
                }
                active_mesh=mess.size()-1;
            }
            else
                std::cout<<"File could not be opened"<<endl;
        }
    }
}

//Is this coordinate inside a wall, or on a walkable surface
bool world::walkable(uint x, uint y) const
{
    x=(x)/tile_w;
    y=(y)/tile_h;

    if (x<grid_m && y<grid_n)
        return walkable_grid[y*grid_m+x];
    else
        return false;
}

world::~world()
{
    IO::graphics::delete_tex(bottom_textures);
    IO::graphics::delete_tex(top_textures);
    IO::graphics::delete_tex(walk_tile_texture);
}

void world::display_background(int camera_position_x,int camera_position_y ) const
{


    IO::graphics::set_shadow(1,0.5,vec4(0,0,0,1));
    IO::graphics::draw_tex(-camera_position_x,-camera_position_y,bottom_textures ,false,false);
    IO::graphics::set_shadow(false);
}

void world::display_top(int camera_position_x,int camera_position_y, bool debug) const
{
    //Blend mode 2, means that if this is in "light" (i.e. in view of hte player) you can see through it
    IO::graphics::set_shadow(2,1.f,vec4(-1,-1,-1,0));
    IO::graphics::draw_tex(-camera_position_x,-camera_position_y,top_textures ,false,false);
    IO::graphics::set_shadow(false);

    if (debug)
    {


        if (walk_tile_texture != (tex_index) -1)
            for (uint j = 0; j < grid_n; ++j)
                for (uint i = 0; i < grid_m; ++i)
                    if (!walkable_grid[j*grid_m+i])
                        IO::graphics::draw_tex(i*tile_w-camera_position_x,j*tile_h-camera_position_y,walk_tile_texture,false,false);
       for (const mesh2D& M : mess)
            M.display(vec2(camera_position_x,camera_position_y));
    }

}

void world::save_polygons() const
{
    cout<<"Saving to"<<polygon_file.String()<<endl;
    std::ofstream OUT(polygon_file.String(), std::ios::binary);//Binary files are easier to read and write

    if (OUT.is_open())//Check if the polygon file is there
    {//We could do some more tests, i.e. check for readability, but lets just load it, and let it throw an exception if it is not valid
        uint meshes = mess.size();
        cout<<"Saving "<<meshes<<endl;
        OUT.write((const char*) &meshes,sizeof(uint));
        try
        {
            for (const mesh2D& M : mess)
            {
                M.save(OUT);
            }
        }
        catch(exception& e)
        {//Whatever
            std::cout<<"Error while saving "<<e.what()<<endl;
        }
    }
    else
        std::cout<<"File could not be opened"<<endl;



}

void world::add_vertex(uint x, uint y)
{
    if (active_mesh == (uint) -1)
    {
        ++active_mesh;
        mess.push_back(mesh2D());
    }
    mess[active_mesh].add_vertex(vec2(x,y));
}

void world::new_polygon()
{
    ++active_mesh;
    mess.push_back(mesh2D());
}

void world::update_rc(raytracer& Reynold) const
{
    Reynold.update(mess);
}
