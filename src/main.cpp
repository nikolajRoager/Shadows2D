//NOTE I use the glm vector math library, which USES SINGLE PRECISION FLOATS BY DEFAULT, for compatibility with OpenGL, I will use OpenGL for graphics, and besides I am not doing science, I am making something for a computer game, single precision is all I need in almost all cases ... the angle of the points which go into the sorting algorithm later is a douple, but that is the only place where that was important enough. note that I do actually need to explicitly define that ... literally everywhere, that is why I write numbers like 0.5f or 1.f

//This is multiply defined a lot of places, the compiler gets real mad but I prefer to do this explicitly whenever I need it
#define TWO_PI 6.283185307179586
//Yeah let us keep this douple precision, if I need to use single precision float(TWO_PI) will be interpreted like 6.283...f that by the compiler ... it is not literally calling a double to float function every time, that would be crazy.

#include<string>
#include<iostream>
#include<fstream>
#include<filesystem>
#include <chrono>
#include <algorithm>
#include <exception>

#include"raytracer.hpp"
#include"raycaster.hpp"
#include"IO.hpp"
#include "mesh2D.hpp"

#include<cstdint>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

//I use these types so much that these aliases are well worth it
using ushort = uint16_t;//I have never come across any system where this was an issue, but I am horrified by the fact that the c++ standards technically allows for integers of different sizes to technically be used, I want to be absolutely sure that I know where every bit is ... ok, in this project I don't do any bit manipulation, but I do that on a regular basis in the game this is meant for. I personally prefer to use the smallest type I can get away with, and never use signed if unsigned will suffice.
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

    bool do_display = (argc == 2);

    if (do_display)
    {
        cout<<"Starting main display"<<endl;
        try
        {
            IO::init(false,"I can write whatever I want here, and nobody can stop me",assets/"textures",assets/"audio",assets/"fonts",assets/"materials",assets/"keymap.txt", false, 1920,1080);

            lamp = IO::graphics::load_tex((assets/"textures")/"lamp.png");
        }
        catch(string error)
        {
            IO::end();
            cout<<"couldn't set up graphics engine: "<<error<<endl;
            return 0;
        }


    cout<<"Start loop"<<endl;
    }
    else if (argc!=3)
        {cout<<"Usage "<<argv[0]<<" filename (optional iteration number)"<<endl; return 0;}
    mesh2D::toggle_graphics(do_display);
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
                            //if (i==15 || i == 2 || i == 4)
                                mess.push_back(mesh2D(vertices));
                        }
                    }
                    IN.close();
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

    raytracer reynold(vec2(0),lamp,do_display);
    //raycaster reynold(vec2(0),lamp,1024,do_display);
//    raytracer richard(vec2(1,3),lamp,do_display);

    float this_theta = 2.8;
    float this_Dtheta=TWO_PI;
    reynold.set_angle(this_theta, this_Dtheta);


    vec2 pos = vec2(0);

    if (do_display)
    {

        reynold.screen_bounds();
        reynold.update(mess);
//        richard.screen_bounds();
//        richard.update(mess);
        double dt = 0;

        ulong millis=0;
        ulong pmillis = IO::input_devices::get_millis();

        ulong time_start = pmillis;


        vec2 mouse_pos = vec2(
-2.400000,
4.643683
);
        {

            std::ifstream file("position_file.bin", std::ios::binary);//Binary files are easier to read and write

            /*
            //DEBUG, READ AND WRITE SPECIFIC COORDINATES
            if (file.is_open())//Check if the polygon file is there
            {//We could do some more tests, i.e. check for readability, but lets just load it, and let it throw an exception if it is not valid
                file.read((char*)&mouse_pos,sizeof(vec2));
            }
            file.close();
            */
        }
        ulong pupdate = pmillis;

        do
        {
            reynold.screen_bounds();
//            richard.screen_bounds();

            int mouse_x_px, mouse_y_px;
            IO::input_devices::get_mouse(mouse_x_px,mouse_y_px);
            mouse_pos.x = 2*(mouse_x_px-IO::graphics::get_w())/100.f;
            mouse_pos.y = 2*(mouse_y_px-IO::graphics::get_h())/100.f;

            int scroll = IO::input_devices::get_scroll();

            millis = IO::input_devices::get_millis();

            dt = (millis-pmillis)*0.001;

            /*DEBUG READ AND WRITE SPECIFIC COORDINATES
            if (IO::input_devices::mouse_click(false))
            {

                std::ofstream file("position_file.bin", std::ios::binary);

                file.write((const char*)&mouse_pos,sizeof(vec2));
                if (mouse_pos.x==erratum)
                    cout<<"Is exactly the same"<<endl;
                else
                    cout<<mouse_pos.x<<" Is not "<<erratum<<endl;


                IO::input_devices::debug_print_mouse_pos();
                cout<<std::fixed<<mouse_pos.x<<endl;
                cout<<std::fixed<<mouse_pos.y<<endl;
                file.close();
            }
            */

            if (IO::input_devices::ctrl_key())
            {
                if (IO::input_devices::mouse_click(true))//Right click to add new object
                {
                    ++active_mesh;
                    ++meshes;
                    mess.push_back(mesh2D());
                }
                else if (IO::input_devices::mouse_click(false))//Left click to add new vertex to old object
                {
                    if (meshes == 0)
                    {
                        ++active_mesh;//See I told you it would be all-right
                        ++meshes;
                        mess.push_back(mesh2D());
                    }
                    mess[active_mesh].add_vertex(mouse_pos);
                    reynold.update(mess);
//                    richard.update(mess);

                }
            }
            {
                bool do_update = false;
                if (mouse_pos != pos)
                {
                    pos = mouse_pos;

                    do_update = true;
                    reynold.set_origin(mouse_pos);
                }

                if (scroll!= 0)
                {
                    if (IO::input_devices::shift_key())
                    {
                        //Just let theta overflow, we are taking trigonometric functions anyway
                        this_theta+=scroll*0.1f;//Scroll contains how many steps the scrolle wheel has moved since last checked, so no need to include dt in here, if the program slowed down scroll would simply be larger
                    }
                    else
                    {
                        this_Dtheta+=scroll*0.1f;
                        this_Dtheta =  (this_Dtheta > float(TWO_PI))?  float(TWO_PI) : this_Dtheta;
                        this_Dtheta =  (this_Dtheta < 0.1f)? 0.1f: this_Dtheta;
                    }
                    reynold.set_angle(this_theta, this_Dtheta);
                    do_update = true;
                }

                if (do_update)
                {
                    reynold.update(mess);
                }
            }




            for (const mesh2D& M : mess)
                M.display();
            reynold.display();
//            richard.display();

            IO::graphics::activate_Ray();
            reynold.display();
//            richard.display();
            IO::graphics::render_Ray();



            IO::post_loop();

            pmillis = millis;

        }while (!IO::input_devices::should_quit());

        IO::end();

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
        reynold.set_bounds(vec2(-19.2,-10.8),vec2(19.2,10.8));
        reynold.update(mess,false);//Update with display turned off, this will overwrite any depug settings and not make any calls to openGL, in truth this makes very little difference ... but a little is still something
        cout<<"Tries to read "<<argv[2]<<endl;
        uint N = stoi(argv[2]);
        cout<<"Did read "<<N<<endl;
        //cout does take time to flush, so I find that for very small N it was a timesink ... so only print 10 times, and only if we are going to be here for a while
        bool do_print = N>10000;
        auto Before = std::chrono::high_resolution_clock::now();
        for (uint i = 0; i < N; ++i)
        {

            if (do_print)
            {
                if (int((10*i)/N)>int((10*(i-1))/N))
                    cout<<((10*i)/N)*10<<'%'<<endl;
            }
            reynold.update(mess,false);

        }
        auto After = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> Update_time = After-Before;
        std::cout <<" Updated "<<N<<" times: "<< 1000*Update_time.count()<<" ms (single threaded)| for reference, 60 fps is 16.66 ms per frame"<<endl;// And some of that time is spend waiting for the graphics card to finish doing its part, we could conviently do this while we wait for large frame to finish

    }

    cout<<"Stop"<<endl;
    return 0;
    //All is gone, the program has quit.
}