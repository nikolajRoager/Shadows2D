
#include "raytracer.hpp"
#include <iostream>
#include <algorithm>

raytracer::raytracer(vec2 origin,  bool do_display)
{

    triangle_fan = vector<vec2>(1,origin);//Origin must always be defined
    V0=vec2(0,0);//This is my screen, so leave it at that for default
    V1=vec2(1900,1060);

    Buffer=-1;
    draw_size=0;

    if (do_display)
    {
        glGenBuffers(1, &Buffer);
        #ifdef DEBUG_VERTICES
        glGenBuffers(1, &Vertices_Buffer);
        #endif
        #ifdef DEBUG_NON_INTERSECT
        non_intersecting=0;
        glGenBuffers(1, &NI_Vertices_Buffer);
        #endif


        #ifdef DEBUG_OUTLINE
        glGenBuffers(1, &Outline_Buffer);
        #endif
    }

    theta = TWO_PI;

}

raytracer::~raytracer()
{
    //Delete the buffer of this object
    if (Buffer != (GLuint)-1)
        glDeleteBuffers(1,&Buffer);

    #ifdef DEBUG_VERTICES
    if (Vertices_Buffer != (GLuint)-1)
        glDeleteBuffers(1,&Vertices_Buffer);
    #endif

    #ifdef DEBUG_NON_INTERSECT
    if (NI_Vertices_Buffer != (GLuint)-1)
        glDeleteBuffers(1,&NI_Vertices_Buffer);

    non_intersecting=0;
    #endif

    #ifdef DEBUG_OUTLINE
    if (Outline_Buffer != (GLuint)-1)
        glDeleteBuffers(1,&Outline_Buffer);
    #endif
}


raytracer::raytracer(raytracer&& other)
{
    //If we are copying over an already existing oject, remove it
    if (Buffer != (GLuint)-1)
        glDeleteBuffers(1,&Buffer);
    triangle_fan = std::move(other.triangle_fan);


    #ifdef DEBUG_VERTICES
    Vertices_Buffer = other.Vertices_Buffer;
    other.Vertices_Buffer = 0;
    #endif
    #ifdef DEBUG_NON_INTERSECT
    NI_Vertices_Buffer= other.NI_Vertices_Buffer;
    other.NI_Vertices_Buffer= 0;

    non_intersecting=other.non_intersecting;
    #endif
    #ifdef DEBUG_OUTLINE
    Outline_Buffer = other.Outline_Buffer;
    other.Outline_Buffer = 0;
    #endif

    Buffer = other.Buffer;
    draw_size = other.draw_size;
    other.Buffer=-1;//This line is the reason we can't use the default copy constructor! otherwise this would get deleted



    theta = other.theta;
    lens_angle = other.lens_angle;

    dir = other.dir;
    extreme_left = other.extreme_left;
    extreme_right = other.extreme_right;
    extreme_dot = other.extreme_dot;

    limit_lens = other.limit_lens;

}

//NOTE THIS IS SINGLE THREADED... this is not a mistake, I think it is better this way, yes I could make every individual update faster by multithreading, but ultimately there would be some overhead when loading things in and out of different threads, I would much rather have each individual update run single threaded, and then use multiple threads if I have more than one rayycaster
void raytracer::update(const vector<mesh2D>& meshes,bool do_display)
{
    //The math here might not be super well explained, especially the parts I consider "trivial".


    struct vertexdata
    {
        vec2 pos=vec2(0);//where is this vertex
        double theta=0; //Angle relative to observer, ok in this case I do want to be absolutely as precise as I can get so douple here

        bool Locked=true;//Are we certain this is put in right order?

        uint O_ID=-1;//What is the ID of object did we hit? (screen edge is an object too!)
        uint V0_ID=-1;//What vertex in the object is this?
        uint V1_ID=-1;//If this is on an edge, V1 and V2 are the ends
        vertexdata(){}
        vertexdata(vec2 p,vec2 origin,uint O, uint V): pos(p), O_ID(O), V0_ID(V)
        {
            //This is safe! atan2(0,dx) does not divide by 0, and atan2(0,0)=0 by definition
            theta = atan2(pos.y-origin.y,pos.x-origin.x);
        }

    };


    uint Msize = meshes.size();


vector<vec2> screen ={ vec2(V0.x,V0.y),vec2(V0.x,V1.y),vec2(V1.x,V0.y),vec2(V1.x,V1.y)};

    //There is no way to know how many vertices there will be
    vector<vertexdata> vertices;

    //If you can't beat them, wait for them to go away, Points EXACTLY lining up is evil, and difficult to work with
    bool EVIL_REDO=true;

    uint extensions=0;//How many extra vertices do we need (when a vertex is on a corner, where the raycan continue afterwards)

    uint EVIL_RETRIES= 0;

    while(EVIL_REDO)
    {
        EVIL_REDO=false;

    //There is no way to know how many vertices there will be
    extensions=0;//How many extra vertices do we need (when a vertex is on a corner, where the raycan continue afterwards)


    triangle_fan = vector<vec2>(1,triangle_fan[0]);//Drop all previous points

        for (uint j = 0; j <= Msize; ++ j)
            {

                const mesh2D* M = j<Msize? &(meshes[j]) : nullptr;

                const vector<vec2>& Verts  = M!=nullptr? M->get_vertices() : screen;
                uint S = Verts.size();
                if (S < 3)//Lines or above (S includs one more point to loop back)
                    continue;
                if (M!=nullptr)//This list includes the end two times, ignore it
                    S--;

                for (uint i = 0 ; i < S ; ++i)
                {

                    vec2 V  = Verts[i];
                    //Check for intersections along the way

                    if (limit_lens)
                    {
                        //If using a limited viewing/lighting angle, throw away anything which does not fit
                        vec2 this_dir = V-triangle_fan[0];
                       float this_dot = dot(this_dir,dir);
                       //Ok, now just compare this to the dot product of the extreme angle ... and oh no!, we need to divide out the length of this_dir, and sqrt and divisions are cursed
                       //... well ok, I actually ran a speed comparison and it is really nowhere near as bad as I thought ... so yeah
                       if (this_dot < extreme_dot*sqrt(dot(this_dir,this_dir)))//I am still going to avoid a division where I can, it is not as expensive an operation as I thought but I would not is still cursed
                            continue;

                    }

                    bool intersects = false;
                    //cout<<"Check intersect of "<<i<<','<<j<<endl;
                    for (const mesh2D& M1 : meshes)
                    {
                        if(M1.has_intersect(triangle_fan[0],V,EVIL_REDO))
                        {
                          //  cout<<"Found intersect of "<<i<<','<<j<<endl;
                            intersects = true;
                            break;

                        }
                    }
                    if (!intersects)
                    {
                        //cout<<"Register non intersecting "<<i<<','<<j<<" : "<<V.x<<','<<V.y<<endl;
                        vertices.push_back(vertexdata(V,triangle_fan[0],j,i));//Associated with mesh j, vertex i

                        //Now, we want to see if we can continue the line further
                        if (M!=nullptr)//Obviously not if this is the edge box, that thing does not continue
                        {
                            vec2 L = V-triangle_fan[0];
                            if (M->continues(L,i))
                            {
                                ++extensions;
                                vertices[vertices.size()-1].Locked=false;//Ok ... Save this for after the initial sort
                            }


                        }
                    }
                }

            }



        #ifdef DEBUG_NON_INTERSECT
        if (do_display)
        {
            non_intersecting=vertices.size();
            vector<vec2> NI_vertices(non_intersecting*4);
            for (uint i = 0; i < vertices.size(); ++i)
            {
                NI_vertices[i*4]  =(vertices[i].pos+vec2(5,5));
                NI_vertices[i*4+1]=(vertices[i].pos-vec2(5,5));
                NI_vertices[i*4+2]=(vertices[i].pos+vec2(5,-5));
                NI_vertices[i*4+3]=(vertices[i].pos+vec2(-5,5));
            }

            glBindBuffer( GL_ARRAY_BUFFER, NI_Vertices_Buffer);
            glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*(non_intersecting*4), &(NI_vertices[0]), GL_DYNAMIC_DRAW );

        }
        #endif

        //The most dastardly EVIL solution thinkable, literally just re-do if you get something you can't solve
        if (EVIL_REDO)
        {
            float rnd = fract((triangle_fan[0].y+EVIL_RETRIES+triangle_fan[0].x)*32342342245.3254325342f)*1000.f*acc;
            if (std::abs(rnd)<acc)
                rnd = 10*acc;
            triangle_fan[0].x+=rnd;
            rnd = fract((triangle_fan[0].y+EVIL_RETRIES+triangle_fan[0].x)*2982345934.3254325342f)*1000.f*acc;
            if (std::abs(rnd)<acc)
                rnd = 10*acc;
            triangle_fan[0].y+=rnd;

            if (EVIL_RETRIES<100)
            {
                vertices = vector<vertexdata>();
                extensions=0;

                EVIL_RETRIES++;
            }
            else
                break;
       }
    }


    vec2 temp = triangle_fan[0];
    //Now we have all the vertices ... now we just need to sort them so that we traverse in the same direction

    uint vertex_size = vertices.size();
    if (vertex_size>0||limit_lens)//We will have at least 2 vertices if we are using a limited lens
    {

            if (limit_lens)
            {
                //c++ atan2 goes from -pi to pi so std sort will do ... well exactly what it is told to do while still messing up spectacularly, if this is a cutoff so lets do a quick and dirty fix by changing the angle to the angle relative to the point right behidn our looking direction

                float offset_angle = float(PI)-theta;
                for (vertexdata& V : vertices)
                {
                    V.theta += offset_angle;
                    if (V.theta<0)
                    {
                        V.theta =TWO_PI-fmod(-V.theta,(TWO_PI));
                    }
                    else if (V.theta > (TWO_PI))
                    {
                        V.theta = fmod(V.theta,(TWO_PI));
                    }
                }

                    //By the way, it is amazing how much easier a problem is when you try to explain what you see to someone else, I only realized this when I introduced the limited angle, and I somehow got fixated on the idea that the extreme left and right vectors should just be swapped, but then I tried explaining how the algorithm worked to my sister (who studies computer science), and I somehow imediately knew, before even finishing the sentence, that it was std sort and atan2 disagreeing ... I guess actually talking through what is happening forces one to take a big picture view ... I also have lost count of how many times I have written a stackoverflow post, only to delete it before publishing because I end up realising what is wrong, when I try to descripe it

            }
        if (vertex_size>0)
            std::sort(vertices.begin(),vertices.end(),[&temp] (const vertexdata& lhs,const vertexdata& rhs)
            {
                return rhs.theta < lhs.theta;
            }
            );

        if (limit_lens)
        {
            vertexdata Vleft = vertexdata(extreme_left+triangle_fan[0],triangle_fan[0],Msize+1,0);//Msize +1 here refers to the raycaster object lens, it turned out to be easier just to treat the raycaster and screen edge as faux meshes
            vertexdata Vright = vertexdata(extreme_right+triangle_fan[0],triangle_fan[0],Msize+1,0);

            Vleft.Locked=false;
            Vright.Locked=false;

            vertices.insert(vertices.begin(),Vleft);
            vertices.push_back(Vright);

            vertex_size+=2;

        }



        //Now, add in the extensions extra ones... trouple is, inserting in the middle of a vector is, in general a ver inefficient operation
        //OPTIMIZE: Test if it is better to insert to start with while sorting
        vertices.reserve(vertex_size+extensions*1);

        //vertex_size+=1*extensions;
        uint unlocked = extensions;


        //Some lambda functions for testing if we  should swap or not
        //I try to use C++ lambda function instead of copy pasting code ...

    //Could these things be neighbors, provided that they are already on the same object
        auto could_neighbor_same = [&meshes](const vertexdata& V0, const vertexdata& V1) -> bool
        {

            if (V1.V1_ID == (uint)-1)
            {//V1 is a vertex
                uint S = meshes[V0.O_ID].get_size()-1;

                return ( ((V1.V0_ID+1)%S == V0.V0_ID || V1.V0_ID == (V0.V0_ID+1)%S) );
            }
            else
            {
                return (
                V0.V0_ID == V1.V1_ID ||
                V0.V0_ID == V1.V0_ID ||
                V0.V1_ID == V1.V1_ID ||
                V0.V1_ID == V1.V0_ID
                );//V1 is on an an edge, and this edge includes V0
            }

        };

        //This checks using the vertex in front and back of the Old vertex and its new extension whether or not they should be swaped
        auto checkswap =[&meshes,could_neighbor_same](const vertexdata& Front,vertexdata& VOld,vertexdata& VNew,const vertexdata& Back) -> bool
        {
            bool swap=false;
            //Rule 1: Object change can only happen on an extensions, i.e. if VNew.O_ID != VOld.O_ID, then try to match neighbors
            if (VNew.O_ID != VOld.O_ID)//The check is they are on different objects is too simple to put in its own lambda function
            {

                if (Front.Locked)
                {//If this is so, we are guaranteed to resolve the issue now

                    if (VOld.O_ID == Front.O_ID)
                        swap=true;
                    VNew.Locked=true;
                    VOld.Locked=true;

                }
                else if (Back.Locked)
                {

                    if (VNew.O_ID == Back.O_ID)
                        swap=true;
                    VNew.Locked=true;
                    VOld.Locked=true;
                }
            }//Rule 2, Vertex number jumps can only happen on an extension, and VOld is guaranteed to be on a vertex while NVew is surely not, so try to match neighboring vertices
            else
            {
                if (Front.Locked)
                {//If this is so, we are guaranteed to resolve the issue now
                    swap = could_neighbor_same(VOld,Front);
                    VNew.Locked=true;
                    VOld.Locked=true;

                }
                else if (Back.Locked)
                {
                    swap = !could_neighbor_same(VOld,Back);
                    VNew.Locked=true;
                    VOld.Locked=true;
                }

            }
            return swap;
        };


        auto it = vertices.begin();
        for (uint i = 0; i<vertex_size; ++i)
        {

            vertexdata& VOld = vertices[i];
            if (!VOld.Locked)
            {
                //So ... now for the important question, where do we insert a new vertex


                //We can check for swap already, if either one of these vertices is unlocked



                float minL2 = -1;//At first, don't do any distance requirements, this allows the ray to continue much further
                vec2 W;//In retrospect, I should have used a more descriptive name, it is the output of the get intersect function though
                vec2 V = VOld.pos;
                uint hit_ID;
                bool intersects = false;

                uint my_V0=-1;
                uint my_V1=-1;

                for (uint k = 0; k < Msize; ++k)
                {
                    const mesh2D& M1 = meshes[k];

                    if(M1.get_intersect(triangle_fan[0],V,W,my_V0,my_V1,minL2))
                    {
                        hit_ID=k;
                        intersects = true;

                    }
                }

                vertexdata VNew;
                if (intersects)
                {
                    VNew = vertexdata(W,triangle_fan[0],hit_ID,my_V0);
                    VNew.Locked = false;
                    VNew.V1_ID = my_V1;

                }
                else
                {
                //No intersections found, add in the edge

                    float a = V.y - triangle_fan[0].y;
                    float b = triangle_fan[0].x - V.x;
                    float c = a*(triangle_fan[0].x) + b*(triangle_fan[0].y);

                    //Of course, we only do intersect if the intersection is ahead of us
                    //Intersections happen at V0.x, V1.x, V0.y, V1.y
                    if (approx(b,0))//Looking dead ahead
                    {
                        if(a>0)
                        {
                            V.y = V1.y;
                            my_V0 = 1;//The order doesn't matter
                            my_V1 = 3;
                        }
                        else
                        {
                            V.y = V0.y;
                            my_V0 = 0;
                            my_V1 = 2;

                        }
                    }
                    else if (approx(a,0))
                    {
                        if(b>0)
                        {
                            V.x = V0.x;
                            my_V0 = 0;
                            my_V1 = 1;
                        }
                        else
                        {
                            V.x = V1.x;
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
                            V=W1;

                        }
                        else
                        {
                            if(b<0)
                            {
                                V.x = V1.x;
                                my_V0 = 2;
                                my_V1 = 3;
                            }
                            else
                            {
                                V.x = V0.x;
                                my_V0 = 0;
                                my_V1 = 1;

                            }
                            V.y = (c-a*V.x)/b;

                        }

                    }


                    //vertices.push_back(vec4(V,0,Msize));//Associated with screen border
                //Display_vertices[j]=V;
                    VNew = vertexdata(V,triangle_fan[0],Msize,my_V0);
                    VNew.V1_ID=my_V1;
                    VNew.Locked=false;

                }


                //If this is the extreme edge of the camera just replace the old vertex
                if (VOld.O_ID == Msize+1)
                {
                    VNew.Locked=true;
                     vertices[i]=VNew;
                }
                else
                {


                //Lets see if there is enough information to see if we should swap

                bool swap =checkswap(vertices[(i+1)%vertex_size],VOld,VNew,vertices[(i+vertex_size-1)%vertex_size]);


                if (VOld.Locked)
                {
                    --unlocked;
                }
                vertices.insert(it+i+((swap)?0:1),VNew);


                ++vertex_size;//We will extend regardless of where we hit
                i+=1;

                }
            }
        }


        //Fun fact, if an unlocked pair has ANY locked neighbors, they will be resolved for sure!, so the only unlocked remaining are in the very start, and if unlocked == extensions, then everything is unlocked,  and if only we were able to lock down ANY at all, the rest will solve itself in just one pass. IT IS ESSENTIALLY A SODUKO
        if (unlocked == extensions && extensions != 0)
        {//At this rare point, we know that literally EVERYTHING is an unresolved pair
            //Let us resolve this by resolving the last pair, that will allow everything to unwrap

            //What vertices do we need to consider here
            const vertexdata& Vn2 = vertices[(2*vertex_size-3)%vertex_size];
            const vertexdata& Vn1 = vertices[(2*vertex_size-4)%vertex_size];
            vertexdata& V0 = vertices[(2*vertex_size-2)%vertex_size];
            vertexdata& V1 = vertices[vertex_size-1];
            const vertexdata& V2 = vertices[0];
            const vertexdata& V3 = vertices[1];

            //First and foremost, is this a case of a cross-object or same object
            if(V0.O_ID == V1.O_ID)
            {


                bool could_stay=false;
                bool could_swap=false;
                //Just go through our neighbors one by one, and see if they could be neighbors to V0 or V1
                if (Vn1.O_ID==V0.O_ID)
                {//If the thing behind us matches V0, we want to stay
                    could_stay = could_neighbor_same(V0,Vn1);
                }
                if (Vn2.O_ID==V0.O_ID)
                {
                    could_stay = (could_stay)? true : could_neighbor_same(V0,Vn2);
                }
                if (V2.O_ID==V0.O_ID)
                {//If the thing in front matches V0, we want to swap
                    could_swap =  could_neighbor_same(V0,V2);
                }
                if (V3.O_ID==V0.O_ID)
                {
                    could_swap = (could_swap)? true : could_neighbor_same(V0,V3);
                }

                //In any case, rule 2 guarantees that V0 matches either one of the vertices before or after so both false is not an option
                if (could_swap && !could_stay)
                {//Swap
                    vertexdata V0temp = V0;
                    V0=V1;
                    V1=V0temp;

                }
                else if (could_stay && !could_swap)
                {//Stay

                }
                else /*By rule 2, only option is (could_stay && could_swap)*/
                {//*Facepalm* ... HOW DID THIS HAPPEN
                //... How could this happen
                //... Surely, it could not!
                // Because to have this happen, we would need two vertices which drop of and then the ray hits the object itself, and with the non-intersection rule, that will require at least 6 vertices , So I am going to go with no, this is literally impossible
                }


            }
            else
            {
                //AHA!, check if the object of V0 is at only one of the neighboring pairs
                bool can_swap = (V0.O_ID == V2.O_ID || V0.O_ID == V3.O_ID );
                bool can_stay = (V0.O_ID == Vn1.O_ID || V0.O_ID == Vn2.O_ID );

                //The easy case, swap
                if (can_swap && !can_stay)
                {
                    vertexdata V0temp = V0;
                    V0=V1;
                    V1=V0temp;
                }
                else if (!can_swap && can_stay)
                {
                    //Don't swap
                }
                else//Aaargh this can only be that both returned true (refer to rule 1), so, now check V1
                {

                    can_stay = (V1.O_ID == V2.O_ID || V1.O_ID == V3.O_ID );
                    can_swap = (V1.O_ID == Vn1.O_ID || V1.O_ID == Vn2.O_ID );

                    //The easy case, swap
                    if (can_swap && !can_stay)
                    {
                        vertexdata V0temp = V0;
                        V0=V1;
                        V1=V0temp;
                    }
                    else if (!can_swap && can_stay)
                    {
                        //Don't swap
                    }//Rule 1 guarantees that we cant have false, false, but what about repeated true-trues ... well, that will imply that both V2,V3 and V-1 and V2 are made of the same objects as V0 and V1 ... which IS possible actually, although it requires some crazy self-intersection to pull off
                    else
                    {//But I sure do hope it is rare
                    }

                }

            }

            V0.Locked = true;
            V1.Locked = true;


            --unlocked;


        }
        //If the soduko has been resolved (or if none was present to begin with), just loop through everything
        {
            //Remember unlocked == 0 in most cases
            for (uint i = 0; i<unlocked ; ++i)
            {
                //NO NEED TO CHECK IF THESE ARE SWAPABLE, THEY ARE!!! If they were not everything after them would already be locked, and we would not have gotten here now.

                if (checkswap(vertices[(i*2+2)%vertex_size],vertices[i*2],vertices[i*2+1],vertices[(i*2+vertex_size-1)%vertex_size]))
                {
                    vertexdata V0 = vertices[i*2];
                    vertices[i*2]=vertices[i*2+1];
                    vertices[i*2+1]=V0;
                }
            }

            unlocked = 0;

        }

        //If we go all the way around, at in the first vertex again to close everything

        if (!limit_lens)
        {
            vertices.push_back(vertices[0]);
            ++vertex_size;
        }

    }

    draw_size = vertex_size+1;
    #ifdef DEBUG_VERTICES
    vector<vec2> Display_vertices(vertex_size*4);
//    for (uint I : debug_numbers)
//        IO::graphics::delete_text(I);
//    debug_numbers = vector<uint >(vertex_size);
    #endif
    #ifdef DEBUG_OUTLINE
    vector<vec2> Display_outline(vertex_size+(limit_lens? 2 : 0));
    #endif

    triangle_fan = vector<vec2>(draw_size,triangle_fan[0]);
    for (uint i = 0; i < vertex_size; ++i)
    {
    //Uncomment for written breakdown of everything
    /*
        cout<<"\n Vertex "<<i<<" ("<<vertices[i].pos.x<<','<<vertices[i].pos.y<<")\n O ="<<vertices[i].O_ID<<"\n V0="<<vertices[i].V0_ID;
        if (vertices[i].V1_ID!=(uint)-1)
            cout<<"\n V1="<<vertices[i].V1_ID;
        cout<<"\n Locked ="<<vertices[i].Locked<<endl;
*/
        //Uncomment to see outline only

        #ifdef DEBUG_VERTICES
        Display_vertices[i*4+0]=vertices[i].pos-vec2(5,0.0);
        Display_vertices[i*4+1]=vertices[i].pos+vec2(5,0.0);
        Display_vertices[i*4+2]=vertices[i].pos-vec2(0.0,5);
        Display_vertices[i*4+3]=vertices[i].pos+vec2(0.0,5);

//        debug_numbers[i] = (IO::graphics::set_text(to_string(i)));

        #endif
        #ifdef DEBUG_OUTLINE
        Display_outline[i+(limit_lens? 1 : 0)]=vertices[i].pos;
        #endif
        triangle_fan[i+1]=vertices[i].pos;

    }

    //Be aware that baking the results to debug buffers is somewhat slow ... nothing compared to the algorithm itself, but enough that the debug options should be turned off when testing the speed
    #ifdef DEBUG_VERTICES
    if (do_display)
    {
        glBindBuffer( GL_ARRAY_BUFFER, Vertices_Buffer);
        glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*(vertex_size*4), &(Display_vertices[0]), GL_DYNAMIC_DRAW );
    }
    #endif



    #ifdef DEBUG_OUTLINE
    if (limit_lens)//If we use limited lense, we want to see the actual cone
    {
        Display_outline[0]=triangle_fan[0];
        Display_outline[vertex_size+1]=triangle_fan[0];
    }


    if (do_display)
    {
        glBindBuffer( GL_ARRAY_BUFFER, Outline_Buffer);
        glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*(vertex_size+(limit_lens? 2 : 0)), &(Display_outline[0]), GL_DYNAMIC_DRAW );
    }
    #endif

    if (do_display)
    {
        glBindBuffer( GL_ARRAY_BUFFER, Buffer);
        glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*(draw_size), &(triangle_fan[0]), GL_DYNAMIC_DRAW );
    }
}

void raytracer::display() const
{

    //Different versions of Debug mode display
    #ifdef DEBUG_OUTLINE
    //Outline rather than triangle fan, easier to spot a wrong swap
    if (draw_size>1 && Outline_Buffer != (GLuint)-1)
        IO::graphics::draw_lines(Outline_Buffer,draw_size-1+(limit_lens? 2 : 0),vec3(0,0,1));
    #endif
    #ifdef DEBUG_VERTICES
    //Draw vertices as crosses
    if (draw_size>1 && Vertices_Buffer!= (GLuint)-1)
        IO::graphics::draw_segments(Vertices_Buffer,(draw_size-1)*4,vec3(0,1,0));
    #endif



    #ifdef DEBUG_NON_INTERSECT
    //Draw vertices as crosses
    if (non_intersecting>0 && NI_Vertices_Buffer!= (GLuint)-1)
        IO::graphics::draw_segments(NI_Vertices_Buffer,non_intersecting*4,vec3(1,1,1));
    #endif


    #ifndef DEBUG_NO_TRIANGLES
    if (draw_size>1 && Buffer != (GLuint)-1)//Default display
        IO::graphics::draw_triangles(Buffer,draw_size,vec3(1));
    #endif

}


void raytracer::set_angle(float _theta, float D)
{
    theta = _theta;
    lens_angle = D;

    if (theta<0)
    {
        theta =TWO_PI-fmod(-theta,float(TWO_PI));
    }
    else if (theta > float(TWO_PI))
    {
        theta = fmod(theta,float(TWO_PI));
    }


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
