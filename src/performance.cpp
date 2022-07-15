#define TWO_PI 6.283185307179586

#include<string>
#include<iostream>
#include<fstream>
#include"my_filesystem.hpp"
#include <chrono>
#include <algorithm>
#include <exception>

#include"raycaster.hpp"
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


//The main loop, I try to keep anything graphics related out of this.
int main(int argc, char* argv[])
{


    cout<<"Running benchmark"<<endl;


    vector<mesh2D> mess;

    if (argc  != 3)
    {
        cout<<"Usage "<<argv[0]<<" filename NUMBER"<<endl;
    }
    name = string(argv[1]);

    my_path poly_file=assets/(name+".bin");


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
                    if (size>0)
                    {
                        vector<vec2> vertices(size);
                        IN.read((char*)&vertices[0],size*sizeof(vec2));
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

    //For editing meshes
    ushort meshes = mess.size();

    raycaster reynold(vec2(0),lamp);
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

    return 0;
    //All is gone, the program has quit.
}

