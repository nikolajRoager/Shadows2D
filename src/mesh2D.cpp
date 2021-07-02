#include "mesh2D.hpp"
#include "graphicus.hpp"
#include <iostream>


mesh2D::mesh2D()
{
    vertices = vector<vec2>(0);
    size = 0;

    //Lets be real, we are going to want the buffer ready to receive data any second now.
    glGenBuffers( 1, &vertexBuffer);
 //   Bsphere_center=vec2(0);
 //   Bsphere_d=0;
}

mesh2D::mesh2D(vector<vec2>& V, bool do_display)
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
  //  Bsphere_center=other.Bsphere_center;
  //  Bsphere_d=other.Bsphere_d;
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
                vec2 I = vec2((c1-b1*A.y),A.y);//Actually the x coodriante should be divided by a1, but divisions are evil and must be avoided at all cost, me multiply with a1 when testing instead
                float Ax = A.x*a1;
                float Bx = B.x*a1;
                float Cx = C.x*a1;
                float Dx = D.x*a1;
                if (std::max(Cx,Dx)>I.x && I.x>std::min(Dx,Cx))
                {
                    if (std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y) )
                        if (std::min(Ax,Bx)<I.x && I.x <std::max(Ax,Bx) )
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
                vec2 I = vec2(A.x,(c1-a1*A.x));//Same thing, should divide the y coordiante by b1 but divisions are evil
                float Ay = A.y*b1;
                float By = B.y*b1;
                float Cy = C.y*b1;
                float Dy = D.y*b1;
                if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)))
                {
                    if (std::min(Cy,Dy)<I.y && I.y <std::max(Cy,Dy) )
                        if (std::min(Ay,By)<I.y && I.y <std::max(Ay,By) )
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

                    vec2 I = vec2((b1*c - b*c1),(a*c1 - a1*c));//Really this should be divided by the determinant, but divisions are evil
                    //The is-equal case happens whenever this vertex is on the x or y axis, which is very common if the boxes are placed by the computer thorugh some procedure
                    float Ax = A.x*det;//It is around 10% faster to scale up everything else by the determinant than to divide by it
                    float Bx = B.x*det;
                    float Cx = C.x*det;
                    float Dx = D.x*det;
                    float Ay = A.y*det;
                    float By = B.y*det;
                    float Cy = C.y*det;
                    float Dy = D.y*det;

                    if ((std::max(Cx,Dx)>I.x && I.x>std::min(Dx,Cx)) || b1 ==0 )
                    {
                        if ((std::min(Cy,Dy)<I.y && I.y <std::max(Cy,Dy)) || a1 ==0)
                            if ((std::min(Ay,By)<I.y && I.y <std::max(Ay,By))  || a1 ==0)
                                if ((std::min(Ax,Bx)<I.x && I.x <std::max(Ax,Bx))  || b1 ==0)
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

    //Actually lets look for some more extremely rare edge cases, what if the ray goes EXACTLY through one vertex which is not the target (i.e. within floating point precision), that should count as a collision, but that might ... rarely ... get here

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

    //cout<<"Checks continues "<<i<<endl;

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
                    vec2 I = vec2((c1-b1*A.y)/a1,A.y);//This one division is not really worth optimizing away

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
            else if (isY)
            {

                if (b1!=0)
                {
                    //We collide with this if
                    //a1 A.x + b1 y = c1
                    vec2 I = vec2(A.x,(c1-a1*A.x)/b1);//This one division is not really worth optimizing away
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
                    //Really we should divide by the determinant... but divisions are evil so lets instead scale everything else by the determinant, and only divide if we really need to
                    vec2 I = vec2((b1*c - b*c1),(a*c1 - a1*c));
                    vec2 newA = A*det;
                    vec2 newB = B*det;
                    vec2 newC = C*det;
                    vec2 newD = D*det;

                    if ((std::max(newC.x,newD.x)>I.x && I.x>std::min(newD.x,newC.x)) || b1 ==0)
                    {
                        if ((std::min(newC.y,newD.y)<I.y && I.y <std::max(newC.y,newD.y)) || a1 ==0 )
                            if (dot(I-newA,newB-newA)>0 )//If it is anywhere ahead of us
                            {
                                float L2 = dot(I-newA,I-newA);
                                if (L2<AB2*(det*det) || AB2 ==-1)
                                {
                                    AB2 = L2/(det*det);
                                    Out=I/det;
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

/*
//Set up the bounding sphere ... actually the class is literally cursed, I can not add in more variables or the entire program will inexplicably crash on any call to std::cout, even if this class is never used
void mesh2D::calc_bsphere()
{

    //There is no easy way to do this, we have to loop through every combination of every vertex
    float max_d2=-1;
    vec2 A,B;

    for (ushort i = 0; i<size; ++i)
        for (ushort j = i+1; j<size; ++j)
        {
            vec2 c = vertices[i]-vertices[j];
            float d2 = dot(c,c);
            if (d2<max_d2 || max_d2==-1)
            {
                A=vertices[i];
                B=vertices[j];
                max_d2 = d2;
            }
        }
    Bsphere_center = A*0.5f+B*0.5f;
    Bsphere_d = sqrt(max_d2);

}*/
