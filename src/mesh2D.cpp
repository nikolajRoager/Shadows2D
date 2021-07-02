#include "mesh2D.hpp"
#include "graphicus.hpp"
#include <iostream>


mesh2D::mesh2D()
{
    vertices = vector<vec2>(0);
    size = 0;

    //Lets be real, we are going to want the buffer ready to receive data any second now.
    glGenBuffers( 1, &vertexBuffer);
}

mesh2D::mesh2D(vector<vec2>& V)
{


    vertices = std::move(V);


    size = vertices.size();
    if (vertices[size-1] != vertices[0])
    {
        vertices.push_back(vertices[0]);//Close the loop
        ++size;
    }

    vertexBuffer = -1;

    //Now, generate the glorious buffers!
    glGenBuffers( 1, &vertexBuffer);
    glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*size, &(vertices[0]), GL_DYNAMIC_DRAW );

    //Now the data exists both in a CPU and the GPU, and I told the GPU that I might occasionally want to modify the data.
}

//No need to optimize this very much, as I only need to call it once in a while
void mesh2D::add_vertex(vec2 New)
{

    if (size>0)
    {
        vertices.push_back(vertices[0]);//Keep the loop closed by extending the ending
        vertices[size-1]=New;//Add the new element
        ++size;
    }
    else
    {
        vertices.push_back(New);
        vertices.push_back(New);
        size=2;

    }

    //Redo the buffer, no need to regenerate it, and no need to do any fancy streaming, we don't need to redo this each frame
    glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*size, &(vertices[0]), GL_DYNAMIC_DRAW );

}

mesh2D::~mesh2D()
{
    //Delete the buffer of this object
    if (vertexBuffer != (GLuint)-1)
        glDeleteBuffers(1,&vertexBuffer);
}


mesh2D::mesh2D(mesh2D&& other)
{
    //If we are copying over an already existing oject, remove it
    if (vertexBuffer != (GLuint)-1)
        glDeleteBuffers(1,&vertexBuffer);
    vertices = std::move(other.vertices);
    vertexBuffer = other.vertexBuffer;
    size = other.size;
    other.vertexBuffer=-1;//This line is the reason we can't use the default! otherwise this would get deleted
}

void mesh2D::display() const
{
    if (size>1 && vertexBuffer != (GLuint)-1)//We want some kind of closed loop to display
        graphicus::draw_lines(vertexBuffer,size,vec3(1,0,0));
}
void mesh2D::save(ofstream& OUT) const
{
    OUT.write((const char*)&size,sizeof(ushort));//I know I am working with same size integers here, since I use cstdint
    OUT.write((const char*)&vertices[0],size*sizeof(vec2));
}

bool mesh2D::has_intersect(const vec2& A,const vec2& B) const
{//Does any of my edges intersect with these? just asking for this, nevermind where
    //Disregard anything outside range and hitting the end points exactly
    //Assume that A is the origin, and only B might be a point on the line

    float a = B.y - A.y;
    float b = A.x - B.x;
    float c = a*(A.x) + b*(A.y);
    bool isX = A.y == B.y;
    bool isY = A.x == B.x;
    //We will handle these special cases on their own, as they are super vulnerable to floating point errors, and besides, they are a lot simpler
    for (ushort i = 0; i <size-1; ++i)
    {
        const vec2& C = vertices[i];
        const vec2& D = vertices[i+1];

        // Line CD represented as a1x + b1y = c1
        float a1 = D.y - C.y;


        float b1 = C.x - D.x;

        float c1 = a1*(C.x)+ b1*(C.y);
        //Selfcollision is never registered
        if (!(C==B || D == B))
        {
            //Special cases first
            if (isY && a1 == 0)//Extreme rare where the ray and segment are perpendicular, and aligned to the axes, this would cause a divide by 0 error if handled in the full case, and it does happen ... very rare, but happens
            {
                //Thankfully this is the easiest thing in the word to deal with
                if (C.y<std::max(A.y,B.y) && C.y>std::min(A.y,B.y))
                    if (A.x<std::max(C.x,D.x) && A.x>std::min(C.x,D.x))
                        return true;
            }
            else if (isX && b1 == 0)
            {
                if (C.x<std::max(A.x,B.x) && C.x>std::min(A.x,B.x))
                    if (A.y<std::max(C.y,D.y) && A.y>std::min(C.y,D.y))
                        return true;

            }
            else if (isX && a1 != 0)
            {
                //Exact vertex collision, rare but not as rare as you would think
                if (C.y==B.y)
                {
                    float Dist1 = std::abs(A.x-C.x);
                    float Dist2 = std::abs(A.x-B.x);
                    return Dist1<Dist2;

                }
                else if (D.y==B.y)
                {
                    float Dist1 = std::abs(A.x-D.x);
                    float Dist2 = std::abs(A.x-B.x);
                    return Dist1<Dist2;
                }
                //We collide with this if
                //a1 x + b1 A.y = c1
                vec2 I = vec2((c1-b1*A.y)/a1,A.y);
                if (std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x))
                {
                    if (std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y) )
                        if (std::min(A.x,B.x)<I.x && I.x <std::max(A.x,B.x) )
                            {
                                return true;
                            }
                }
            }
            else if (isY && b1 !=0)
            {
                //Exact vertex collision, rare but not as rare as you would think
                if (C.x==B.x)
                {
                    float Dist1 = std::abs(A.y-C.y);
                    float Dist2 = std::abs(A.y-B.y);
                    return Dist1<Dist2;

                }
                else if (D.x==B.x)
                {
                    float Dist1 = std::abs(A.x-D.x);
                    float Dist2 = std::abs(A.x-B.x);
                    return Dist1<Dist2;
                }
                //We collide with this if
                //a1 A.x + b1 y = c1
                vec2 I = vec2(A.x,(c1-a1*A.x)/b1);
                if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)))
                {
                    if (std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y) )
                        if (std::min(A.y,B.y)<I.y && I.y <std::max(A.y,B.y) )
                            {
                                return true;
                            }
                }
            }
            else//Full case
            {


                float det = a*b1 - a1*b;
                if (det != 0)
                {

                    vec2 I = vec2((b1*c - b*c1),(a*c1 - a1*c))/det;
                    //The is-equal case happens whenever this vertex is on the x or y axis, which is very common if the boxes are placed by the computer thorugh some procedure
                    if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)) || b1 ==0 )
                    {
                        if ((std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y)) || a1 ==0)
                            if ((std::min(A.y,B.y)<I.y && I.y <std::max(A.y,B.y))  || a1 ==0)
                                if ((std::min(A.x,B.x)<I.x && I.x <std::max(A.x,B.x))  || b1 ==0)
                                {
                                    return true;
                                }
                    }
                }
            }
        }
        else//Super rare edge case, Incomming ray is parallel with the edge which contain one of the vertices, in that case do register self-collision
        {
            //C----D  <---A  Here ray A->C should return collision
            if (a*b1 == a1*b)
            {
                float Dist1 = dot(A-C,A-C);
                float Dist2 = dot(A-D,A-D);
                if (B==C)
                    return Dist1>Dist2;
                else
                    return Dist1<Dist2;
            }
        }
    }

    //If no interesction is found, then no interesection is found

    return false;

}

//Does this ray L from O to vertices[i] continu beyond where it otherwise intersects
bool mesh2D::continues(const vec2& L,ushort i) const
{
    const vec2& V1 = vertices[i];

    const vec2& V0 = vertices[(i+1)];//We assume that i is in [0,size-1]
    const vec2& V2 = i == 0 ? vertices[size-2] : vertices[i-1];

    //Now we have the edge vectors from the vertex
    vec2 V10 = V1-V0;
    vec2 V12 = V1-V2;

    //What do we require to not collide at the point? we need the edge vectors to go to the "opposite site" of our incomming ray
    /*
    V0
     V1<-------
        V2
    */
    //So we need to check the sign of the sine of the angle between the vectors, if the are the *same* then we are good... so we need the determinants
    float det0 = V10.x*L.y-V10.y*L.x;//=|L||V10|sin(theta)
    float det1 = V12.x*L.y-V12.y*L.x;//...


    //How do we check if they have the same sign? easy
    return det0*det1>= 0;
}


//Same as has intersect, but this time, do continue the ray
bool mesh2D::get_intersect(const vec2& A,const vec2& B, vec2& Out, ushort& V0_ID, ushort& V1_ID, float& AB2) const
{//Does any of my edges intersect with these? just asking for this, nevermind where
    //Assume that A is the origin, and only B might be a point on the line

    float a = B.y - A.y;
    float b = A.x - B.x;
    float c = a*(A.x) + b*(A.y);
    bool isX = A.y == B.y;
    bool isY = A.x == B.x;
    bool ret = false;
    //We will handle these special cases on their own, as they are super vulnerable to floating point errors, and besides, they are a lot simpler
    for (ushort i = 0; i <size-1; ++i)
    {
        const vec2& C = vertices[i];
        const vec2& D = vertices[i+1];

        // Line CD represented as a1x + b1y = c1
        float a1 = D.y - C.y;
        float b1 = C.x - D.x;
        float c1 = a1*(C.x)+ b1*(C.y);
        //Selfcollision is never registered
        if (!(C==B || D == B))
        {
            if (isX)
            {
                if (a1!=0)
                {
                    //We collide with this if
                    //a1 x + b1 A.y = c1
                    vec2 I = vec2((c1-b1*A.y)/a1,A.y);

                    if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)) || b1 == 0)
                    {
                        if ((std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y)) || a1==0)
                            if (dot(I-A,B-A)>0 )//If it is anywhere ahead of us
                                {
                                    float L2 = dot(I-A,I-A);
                                    if (L2<AB2 || AB2 ==-1)//See if this intersection is better than the one we already have, if yes, yoinks
                                    {
                                        AB2 = L2;
                                        Out=I;
                                        V0_ID=i;
                                        V1_ID=(i+1)%size;
                                    }
                                    ret = true;
                                }
                    }
                }
            }
            else if (isY && b1 != 0)
            {

                if (b1!=0)
                {
                    //We collide with this if
                    //a1 A.x + b1 y = c1
                    vec2 I = vec2(A.x,(c1-a1*A.x)/b1);
                    if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)) || b1 ==0)
                    {
                        if ((std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y))  || a1 ==0)
                            if (dot(I-A,B-A)>0 )//If it is anywhere ahead of us
                                {
                                    float L2 = dot(I-A,I-A);
                                    if (L2<AB2 || AB2 ==-1)
                                    {
                                        AB2 = L2;
                                        Out=I;
                                        V0_ID=i;
                                        V1_ID=(i+1)%size;
                                    }
                                    ret = true;
                                }
                    }
                }
            }
            else//Full case
            {


                float det = a*b1 - a1*b;
                if (det != 0)
                {
                    vec2 I = vec2((b1*c - b*c1),(a*c1 - a1*c))/det;

                    if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)) || b1 ==0)
                    {
                        if ((std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y)) || a1 ==0 )
                            if (dot(I-A,B-A)>0 )//If it is anywhere ahead of us
                            {
                                float L2 = dot(I-A,I-A);
                                if (L2<AB2 || AB2 ==-1)
                                {
                                    AB2 = L2;
                                    Out=I;
                                    V0_ID=i;
                                    V1_ID=(i+1)%size;
                                    ret = true;//Don't break! maybe there is another closer intersection
                                }
                                //If we already have a better intersect, don't even bother returning true
                            }
                    }
                }
            }
        }
    }

    //If no interesction is found, then no interesection is found

    return ret;

}


