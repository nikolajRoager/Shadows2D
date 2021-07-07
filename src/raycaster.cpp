//#define DEBUG_OUTLINE
//#define DEBUG_VERTICES
//#define DEBUG_NO_TRIANGLES

#include "raycaster.hpp"
#include <iostream>
#include <algorithm>

#define TWO_PI 6.28318531

raycaster::raycaster(vec2 origin, ushort tex, ushort res, bool do_display)
{
    my_tex=tex;

    //Why precalculate the unit circle ... because trigonometry is slooooow! Seriously, this is the single largesat time-sink in this algorithm, it is just much slower to start up than the raytracer.
    //I know, startup delays do not matter as much as runtime delays, but I can make startup much much faster by requiring that the resolution is a multiple of 4, round it up!

//    res+=4-(res%4);
    //Why does that help? because now I know each quater circle is going to be the same, and guess what, a quarter circle is all you need, since  cos(theta)=sin(pi/2+theta)



    unit_circle_reference = vector<vec2>(res);

    float dtheta = float(TWO_PI)/(res);
    ushort res4 = res/4;
    for (ushort i = 0; i < res4; ++i)
    {
        float theta = i*dtheta;
        float ctheta = cos(theta);//I don't need the sines, I can just loop trough clockwise for that
        //This way I do 1/8 as many trigonometric calculations as I otherwise would have
        unit_circle_reference[i].x = ctheta;
        unit_circle_reference[res4-1-i].y = ctheta;
        unit_circle_reference[i+res4].y = ctheta;
        unit_circle_reference[2*res4-i-1].x = -ctheta;
        unit_circle_reference[i+2*res4].x = -ctheta;
        unit_circle_reference[3*res4-1-i].y = -ctheta;
        unit_circle_reference[i+3*res4].y =-ctheta;
        unit_circle_reference[res-1-i].x =ctheta;

    }
    my_tex=tex;

    draw_size=res+2;
    triangle_fan = vector<vec2>(draw_size,origin);//Origin must always be defined
    V0=vec2(-19.2,-10.8);//This is my screen, so leave it at that for default
    V1=vec2(19.2,10.8);


    Buffer=-1;
    if (do_display)
    {
        glGenBuffers(1, &Buffer);
    }
}

raycaster::~raycaster()
{
    //Delete the buffer of this object
    if (Buffer != (GLuint)-1)
        glDeleteBuffers(1,&Buffer);
}


raycaster::raycaster(raycaster&& other)
{
    //If we are copying over an already existing oject, remove it
    if (Buffer != (GLuint)-1)
        glDeleteBuffers(1,&Buffer);
    triangle_fan = std::move(other.triangle_fan);



    Buffer = other.Buffer;
    draw_size = other.draw_size;
    other.Buffer=-1;//This line is the reason we can't use the default! otherwise this would get deleted
}

void raycaster::update(const vector<mesh2D>& meshes,bool do_display)
{
    ushort Msize = meshes.size();
    for (ushort i = 0; i < draw_size-1; ++i)
    {
        vec2 ray_vertex = (unit_circle_reference[i%(draw_size-2)]+triangle_fan[0]);//Resetomg the basic romg every update, if we just re-normalized everything floating point errors would slowly build up

        if (limit_lens)
        {
           float this_dot = dot(unit_circle_reference[i%(draw_size-2)],dir);
           if (this_dot < extreme_dot)
           {

                triangle_fan[i+1]=triangle_fan[0];
                continue;
            }
        }

        float minL2 = -1;//let the ray go as far as it will
        vec2 ray_result;
        bool intersects = false;

        ushort my_V0=-1;//In this case we do not care about the vertex IDs because we will not be doing any sorting
        ushort my_V1=-1;


        for (ushort k = 0; k < Msize; ++k)
        {
            const mesh2D& M1 = meshes[k];
            if(M1.get_intersect(triangle_fan[0],ray_vertex,ray_result,my_V0,my_V1,minL2))
            {
                intersects = true;
            }
        }
        if (intersects)
            ray_vertex=ray_result;
        else
        {

        //No intersections found, add in the edge... edge collision is much easier if we know these are vertical or horisontal, that is why I treat it as a seperate case and don't just add in the edge as an object

            //This is just copy pasted from the raytracer ... which I made first, so you can freely skip over this entire else case, just do a text search for --

            float a = ray_vertex.y - triangle_fan[0].y;
            float b = triangle_fan[0].x - ray_vertex.x;
            float c = a*(triangle_fan[0].x) + b*(triangle_fan[0].y);
            //Of course, we only do intersect if the intersection is ahead of us
            //Intersections happen at V0.x, V1.x, V0.y, V1.y
            if (b==0)//Looking dead ahead
            {
                if(a>0)
                {
                    ray_vertex.y = V1.y;
                    my_V0 = 1;//The order doesn't matter
                    my_V1 = 3;
                }
                else
                {
                    ray_vertex.y = V0.y;
                    my_V0 = 0;
                    my_V1 = 2;

                }
            }
            else if (a==0)
            {
                if(b>0)
                {
                    ray_vertex.x = V0.x;
                    my_V0 = 0;
                    my_V1 = 1;
                }
                else
                {
                    ray_vertex.x = V1.x;
                    my_V0 = 2;
                    my_V1 = 3;

                }

            }
            else
            {
                vec2 W1;

                if(a>0)
                {
                    W1.y = V1.y;
                    my_V0 = 1;//The order doesn't matter
                    my_V1 = 3;
                }
                else
                {
                    W1.y = V0.y;
                    my_V0 = 0;
                    my_V1 = 2;

                }
                W1.x = (c-b*W1.y)/a;

                if (W1.x>V0.x && W1.x<V1.x )
                {
                    ray_vertex=W1;

                }
                else
                {
                    if(b<0)
                    {
                        ray_vertex.x = V1.x;
                        my_V0 = 2;
                        my_V1 = 3;
                    }
                    else
                    {
                        ray_vertex.x = V0.x;
                        my_V0 = 0;
                        my_V1 = 1;

                    }
                    ray_vertex.y = (c-a*ray_vertex.x)/b;

                }

            }


        }
        // -- If you are doing a text search in this document for the word "butts", the good news is that it's here, but the bad news is that it only appears in this unrelated quote --
        triangle_fan[i+1]=ray_vertex;

    }



}

void raycaster::display() const
{

    //Bake to buffer here, and not in the update function, this allows us to run update with all opengl functions turned off to only test the speed of the algorithm.
    glBindBuffer( GL_ARRAY_BUFFER, Buffer);
    glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*(draw_size), &(triangle_fan[0]), GL_DYNAMIC_DRAW );



    #ifndef DEBUG_NO_TRIANGLES
    if (draw_size>1 && Buffer != (GLuint)-1)//Default display
        graphicus::draw_triangles(Buffer,draw_size,vec3(0.7,0.7,0.7));
    #endif

    if (my_tex!= (ushort)-1)
        graphicus::draw_tex(my_tex,triangle_fan[0]);
}




void raycaster::set_angle(float _theta, float D)
{
    theta = _theta;
    lens_angle = D;

    if (D<0 || D>=float(TWO_PI))
    {
        lens_angle=TWO_PI;
        limit_lens=false;//No limiting angle

        //Set everything to empty then
        dir = vec2(0);
        extreme_left =vec2(0);
        extreme_right = vec2(0);
        extreme_dot = 0;
    }
    else
    {

        limit_lens=true;

        //These things make it easier for me to compare and limit the angle later, using only cheap dot products without resorting to expensive trigonometric functions
        dir = vec2(cos(theta),sin(theta));
        extreme_left =vec2(cos(theta+lens_angle*0.5f),sin(theta+lens_angle*0.5f));
        extreme_right =vec2(cos(theta-lens_angle*0.5f),sin(theta-lens_angle*0.5f));
        extreme_dot = dot(dir,extreme_left);
    }
}
