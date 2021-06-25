#define TWO_PI 6.283185307179586

#include<string>
#include<iostream>
#include<fstream>
#include<filesystem>
#include <chrono>
#include <algorithm>
#include <exception>

#include"raycaster.hpp"
#include"graphicus.hpp"
#include "mesh2D.hpp"

#include<cstdint>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

//I use these types so much that these aliases are well worth it
using ushort = uint16_t;
using uint = uint32_t;
using ulong = uint64_t;//I need to be sure that this is 32 bit or more, because I use it for keeping time, and 16 bit will break in 1 minute and 5 seconds, 32 bits will last a few years

using namespace std;
using namespace glm;

namespace fs = std::filesystem;

//The main loop, I try to keep anything graphics related out of this.
int main(int argc, char* argv[])
{


    fs::path assets = "assets";

    ushort lamp = -1;

    bool do_display = argc == 2;

    if (do_display)
    {
        cout<<"Starting main display"<<endl;
        try
        {
            graphicus::init(false,assets/"textures",assets/"audio",assets/"fonts",assets/"materials");

    /*      pop = graphicus::load_sound("pop");
            food_tex = graphicus::load_tex("food.png");

            ushort explode = graphicus::load_tex("explode.png");
            graphicus::set_animation(explode,2,2);
            graphicus::set_animation(food_tex,2,2);
            graphicus::set_animation(shot_tex,2,2);
    */
            lamp = graphicus::load_tex((assets/"textures")/"lamp.png");
        }
        catch(string error)
        {
            graphicus::end();
            cout<<"couldn't set up graphics engine: "<<error<<endl;
            return 0;
        }


    cout<<"Start loop"<<endl;
    }
    else if (argc!=3)
        {cout<<"Usage "<<argv[0]<<" filename (optional iteration number)"<<endl; return 0;}

    vector<mesh2D> mess;

    //Default polygon file can not be saved to
    string name = "null";
    if (argc  == 2 || argc ==3)
    {
        name = string(argv[1]);
    }

    fs::path poly_file=assets/(name+".bin");


    if (name.compare("null"))//Because of the way this is set up, this is true whenever the file is not named null, null is interpreted as don't save anything
    {
        if (fs::exists(poly_file))//Check if the polygon file is there
        {//We could do some more tests, i.e. check for readability, but lets just load it, and let it throw an exception if it is not valid
            std::ifstream IN(poly_file, std::ios::binary);//Binary files are easier to read and write

            if (IN.is_open())//Check if the polygon file is there
            {//We could do some more tests, i.e. check for readability, but lets just load it, and let it throw an exception if it is not valid

                try
                {
                    ushort meshes=0;
                    IN.read((char*)&meshes,sizeof(ushort));
                    for (ushort i = 0; i < meshes; ++i)
                    {
                        //See! this is why I use binary! it is that easy to write!, no need to getline or stream into a temporary oject, and then add that to the vector one at the time, just yoink the entire thing
                        ushort size=0;
                        IN.read((char*)&size,sizeof(ushort));//I know I am working with same size integers here, since I use cstdint
                        if (size>0 )
                        {
                            vector<vec2> vertices(size);
                            IN.read((char*)&vertices[0],size*sizeof(vec2));
                            if (i==15 || i == 2 || i == 4)
                                mess.push_back(mesh2D(vertices));
                        }
                    }
                }
                catch(exception& e)
                {//Whatever
                    std::cout<<"Error while loading"<<e.what()<<endl;
                }
            }
            else
                std::cout<<"File could not be opened"<<endl;
        }
    }

    //For editing meshes
    ushort meshes = mess.size();

    ushort active_mesh = meshes-1;//NEVER MIND THE UNDERFLOW! I will check that meshes>0 before calling anything, and once I increase meshes, we will overflow back where we started.

    raycaster reynold(vec2(0),lamp);

    vec2 pos = vec2(0);

    if (do_display)
    {
        reynold.update(mess);
        double dt = 0;

        ulong millis=0;
        ulong pmillis = graphicus::get_millis();

        ulong time_start = pmillis;

        vec2 mouse_pos = vec2(0);

        ulong pupdate = pmillis;
        do
        {
            vec2 mouse_pos = graphicus::get_mouse_pos();

            millis = graphicus::get_millis();
            graphicus::pre_loop();//Reset display

            dt = (millis-pmillis)*0.001;


            if (graphicus::ctrl_key(true))
            {
                if (graphicus::mouse_click(true))//Right click to add new object
                {
                    ++active_mesh;
                    ++meshes;
                    mess.push_back(mesh2D());
                }
                else if (graphicus::mouse_click(false))//Left click to add new vertex to old object
                {
                    if (meshes == 0)
                    {
                        ++active_mesh;//See I told you it would be all-right
                        ++meshes;
                        mess.push_back(mesh2D());
                    }
                    mess[active_mesh].add_vertex(mouse_pos);
                    reynold.update(mess);

                }
            }
            {
                if (mouse_pos != pos)
                {
                    pos = mouse_pos;
                    cout<<"Updates "<<pos.x<<' '<<pos.y<<endl;

                    reynold.set_origin(mouse_pos);
                    reynold.update(mess);
                }

            }


            for (const mesh2D& M : mess)
                M.display();
            reynold.display();


            graphicus::flush();

            pmillis = millis;

        }while (!graphicus::should_quit());

        graphicus::end();

        //Now save the polygons

        if (name.compare("null") && false)//Don't save anything if not output was specified
        {
            std::ofstream OUT(poly_file, std::ios::binary);//Binary files are easier to read and write

            if (OUT.is_open())//Check if the polygon file is there
            {//We could do some more tests, i.e. check for readability, but lets just load it, and let it throw an exception if it is not valid

                ushort meshes = mess.size();
                OUT.write((const char*) &meshes,sizeof(ushort));
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
    }
    else
    {//Run benchmark test
        reynold.update(mess);

        uint N = stoi(argv[2]);
        auto Before = std::chrono::high_resolution_clock::now();
        for (uint i = 0; i < N; ++i)
        {
            if (int((10*i)/N)>int((10*(i-1))/N))
                cout<<((10*i)/N)*10<<'%'<<endl;
            reynold.update(mess);
        }
        auto After = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> Update_time = After-Before;
        std::cout <<" Updated "<<N<<" times: "<< Update_time.count()<<" s"<<endl;

    }
    return 0;
    //All is gone, the program has quit.
}
