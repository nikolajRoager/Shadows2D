
#include "mesh2D.hpp"
#include "IO.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <random>
#include <numeric>
#include <functional>

#define TWO_PI 6.28318531



bool mesh2D::graphic_mode=true;

mesh2D::mesh2D()
{
    vertices = vector<vec2>(0);
    size = 0;

    if (graphic_mode)
    {
        #ifdef DEBUG_SHOW_BSPHERE
        Bsphere_vertices=vector<vec2>();
        #endif
    }
}

mesh2D::mesh2D(vector<vec2>& V)
{


    vertices = vector<vec2>();
    vertices.reserve(V.size());//Maybe copy the list, if there are no dublicate
    size=0;
    if (V.size()>0)
    {
        for (int i = 0; i < V.size(); ++i)
        {
            bool dublicate = false;
            for (int j = 0; j < vertices.size(); ++j)
                if (approx(V[i],vertices[j]) && !(j==0 && i == V.size()-1))
                {
                    dublicate =true;
                    break;
                }

            if (!dublicate)
                vertices.push_back(V[i]);
        }

    size = vertices.size();
    }
    if (size>0)
    {

        if (! approx(vertices[size-1], vertices[0]))
        {
            vertices.push_back(vertices[0]);//Close the loop
            ++size;
        }

        //Now the data exists both in a CPU and the GPU, and I told the GPU that I might occasionally want to modify the data.

        recalc_bsphere();
    }
}

//No need to optimize this very much, as I only need to call it once in a while
void mesh2D::add_vertex(vec2 New)
{

    if (size>0)
    {
        bool dublicate=false;
        for (const vec2& V : vertices)
        {
            if (approx(V,New))
            {
                dublicate = true;
                break;
            }
        }

        if (!dublicate)
        {
            vertices.push_back(vertices[0]);//Keep the loop closed by extending the ending
            vertices[size-1]=New;//Add the new element
            ++size;
            recalc_bsphere();
        }
    }
    else
    {
        vertices.push_back(New);
        vertices.push_back(New);
        size=2;

    }

}

mesh2D::~mesh2D()
{
    //Actually nothing, I used to store the buffers here
}


mesh2D::mesh2D(mesh2D&& other)
{
    vertices        = std::move(other.vertices        );

    #ifdef DEBUG_SHOW_BSPHERE
    Bsphere_vertices= std::move(other.Bsphere_vertices);
    #endif

    size = other.size;

    Bsphere_r2 = other.Bsphere_r2;
    Bsphere_center = other.Bsphere_center;



}

void mesh2D::display(vec2 cam_offset) const
{
    if (graphic_mode)
    {
        if (size>2 )
            IO::graphics::draw_lines(vertices,size,vec3(1,0,0),cam_offset );
        #ifdef DEBUG_SHOW_BSPHERE
        if (size>2 )//We want some kind of closed loop to display
            IO::graphics::draw_lines(Bsphere_vertices,32,vec3(1,0,1),cam_offset );
        #endif
    }
}
void mesh2D::save(ofstream& OUT) const
{
    OUT.write((const char*)&size,sizeof(uint));//I know I am working with same size integers here, since I use cstdint
    OUT.write((const char*)&vertices[0],size*sizeof(vec2));
}

bool mesh2D::has_intersect(const vec2& A,const vec2& B,bool& PANIC) const
{//Does any of my edges intersect with these? just asking for this, nevermind where

    //We need at least a line (3 points, since the first point is included twice to close the loop)
    if (size<3)
        return false;

    //Disregard anything outside range and hitting the end points exactly
    //Assume that A is the origin, and only B might be a point on the line
    float a = B.y - A.y;
    float b = A.x - B.x;



    //First check if the ray intersects the bounding sphere, this is actually a very substantial calculation, but we will be able to discard most intersection calculations right here, and from my testing it works out to waaaaay faster in the end (it is slower in a scene with very few objects, but that is the fastest case anyway so nevermind that). Why does this work? See for instance https://www.geometrictools.com/Documentation/IntersectionLine2Circle2.pdf
    //This is the single most significant time-save. When I implemented it became around three times as fast.
    vec2 ray_dir = vec2(-b,a);
    vec2 A_to_sphere = A-Bsphere_center;
    float D2 = dot(A_to_sphere,A_to_sphere);
    float det = pow(dot(A_to_sphere,ray_dir),2.f)-dot(ray_dir,ray_dir)*(D2-Bsphere_r2);



    //The bouding sphere is completely irrelevant if we are inside it
    if (D2>Bsphere_r2)
    {

    if (det<0 )//We do not intersect, and we are not inside to being with
        return false;//Do just take a moment to note that the bounding sphere detection includes neither square-roots nor divisions
    }


    float c = a*(A.x) + b*(A.y);
    bool isX = approx(A.y,B.y);
    bool isY = approx(A.x,B.x);


    //We will handle these special cases on their own, as they are super vulnerable to floating point errors, and besides, they are a lot simpler
    for (uint i = 0; i <size-1; ++i)
    {
        const vec2& C = vertices[i];
        const vec2& D = vertices[i+1];

        // Line CD represented as a1x + b1y = c1
        float a1 = D.y - C.y;


        float b1 = C.x - D.x;

        float c1 = a1*(C.x)+ b1*(C.y);


        //Selfcollision is never registered
        if (!(approx(C,B) || approx(D,B)))
        {
            //Special cases first
            if (isY && approx(a1 ,0))//Extreme rare where the ray and segment are perpendicular, and aligned to the axes, this would cause a divide by 0 error if handled in the full case, and it does happen ... very rare, but happens
            {
                //Thankfully this is the easiest thing in the word to deal with
                if (C.y<std::max(A.y,B.y) && C.y>std::min(A.y,B.y))
                    if (A.x<std::max(C.x,D.x) && A.x>std::min(C.x,D.x))
                        return true;
            }
            else if (isX && approx(b1, 0))
            {
                if (C.x<std::max(A.x,B.x) && C.x>std::min(A.x,B.x))
                    if (A.y<std::max(C.y,D.y) && A.y>std::min(C.y,D.y))
                        return true;

            }
            else if (isX && !approx(a1, 0))
            {
                //Exact vertex collision, rare but not as rare as you would think

                if (approx(C.y,B.y))
                {//The three vertices A B and C are on a line, so check 1: that the line A -> B goes in the same direction as A -> C, and if so, that A-> B goes so far that it can hit vertex C


                    float Dist1 = A.x-C.x;
                    float Dist2 = A.x-B.x;
                    if (std::abs(Dist1)<std::abs(Dist2) &&  Dist1*Dist2 >0)
                        return true;

                }
                else if (approx(D.y,B.y))
                {
                    float Dist1 = A.x-D.x;
                    float Dist2 = A.x-B.x;
                    if (std::abs(Dist1)<std::abs(Dist2) &&  Dist1*Dist2 >0)
                        return true;
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
            else if (isY && !approx(b1,0))
            {
                //Exact vertex collision, rare but not as rare as you would think
                if (approx(C.x,B.x))
                {//The three vertices A B and C are on a line, so check 1: that the line A -> B goes in the same direction as A -> C, and if so, that A-> B goes so far that it can hit vertex C


                    float Dist1 = A.y-C.y;
                    float Dist2 = A.y-B.y;
                    if (std::abs(Dist1)<std::abs(Dist2) &&  Dist1*Dist2 >0)
                        return true;

                }
                else if (approx(D.x,B.x))
                {
                    float Dist1 = A.y-D.y;
                    float Dist2 = A.y-B.y;
                    if (std::abs(Dist1)<std::abs(Dist2) &&  Dist1*Dist2 >0)
                        return true;
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
                if (!approx(det , 0))
                {

                    vec2 I = vec2((b1*c - b*c1),(a*c1 - a1*c))/det;

                    //The is-equal case happens sometimes, when things have truly been set up to be the same
                    if ((std::min(A.y,B.y)<=I.y && I.y <=std::max(A.y,B.y))  || approx(a1,0))
                        if ((std::min(A.x,B.x)<=I.x && I.x <=std::max(A.x,B.x))  || approx(b1,0))
                        {
                            //Normally, a single vertex should only be able to stop the light, if the shape continues after this




                            //Have we hit the vertices EXACTLY ... SHIT
                            if (approx(C,I))
                            {

                                //cout<<"EXACT HIT ON C "<<i<<endl;
                                PANIC = true;//SOMETHING LINED UP EXACTLY; AAAARGH
                                vec2 L = C-A;
                                if (!continues(L,i))
                                    return true;

                            }
                            else if (approx(D,I))
                            {
                                //cout<<"EXACT HIT ON D "<<i<<endl;
                                PANIC = true;//SOMETHING LINED UP EXACTLY; AAAARGH
                                vec2 L = D-A;
                                if (!continues(L,i+1))
                                    return true;
                            }
                            else
                            {
                                    //cout<<"HAVE DO NOW TESTE DO "<<std::max(C.x,D.x)<<" > (I.x) "<<I.x<< " ("<<(std::max(C.x,D.x)> I.x) <<")&& (I.x) "<<I.x<<" > "<<std::min(D.x,C.x)<<"  ("<<(std::min(C.x,D.x)< I.x) <<")"<<endl;

                                if ( ( geq(std::max(C.x,D.x),I.x) && geq(I.x,std::min(D.x,C.x))) || approx(b1,0) )
                                {
                                        //cout<<"-HAVE DO NOW TEST DO "<<std::max(C.y,D.y)<<" > (I.y) "<<I.y<< " ("<<(std::max(C.y,D.y)> I.y) <<")&& (I.y) "<<I.y<<" > "<<std::min(D.y,C.y)<<"  ("<<(std::min(C.y,D.y)< I.y) <<")"<<endl;

                                    if ((geq(I.y,std::min(C.y,D.y)) && geq(std::max(C.y,D.y),I.y)) || approx(a1,0))
                                    {
                                        return true;
                                    }
                                }
                            }
                        }
                }
            }
        }
        else//Super rare edge case, Incomming ray is parallel with the edge which contain one of the vertices
        {
            //C----D  <---A  Here ray A->C should return collision
            if (approx(a*b1, a1*b))
            {
                //cout<<"GOT EXACT LINEUP"<<endl;
                PANIC = true;//SOMETHING LINED UP EXACTLY; AAAARGH
            }
        }
    }

    //If no interesction is found, then no interesection is found

    return false;

}

//Does this ray L from O to vertices[i] continu beyond where it otherwise intersects
bool mesh2D::continues(const vec2& L,uint i) const
{
    const vec2& V1 = vertices[i];

    const vec2& V0 = vertices[(i+1)];//We assume that i is in [0,size-1]
    const vec2& V2 = i == 0 ? vertices[size-2] : vertices[i-1];

    //Now we have the edge vectors from the vertex
    vec2 V10 = V1-V0;
    vec2 V12 = V1-V2;

    //What do we require to not collide at the point? we need the edge vectors to go to the "opposite site" of our incomming ray
    /*
    This should block
    V0
     |
     V
     V1<-------
       \
        V
        V2
    */
    //So we need to check the sign of the sine of the angle between the vectors, if the are the *same* then we are good... so we need the determinants
    float det0 = V10.x*L.y-V10.y*L.x;//=|L||V10|sin(theta)
    float det1 = V12.x*L.y-V12.y*L.x;//...

    float det01 = det0*det1;

    //How do we check if they have the same sign? "easy"
    /*
    if (det01>0)
    {
        return true;
    }
    but not so fast, what if sin(theta)=0 (The edges are directly towards or away from the incomming ray)

    */

    if (approx(det0,0))
    {
        return true;
    }
    else if (approx(det1,0))
    {

        return true;
    }
    else if (det01>=0)
    {
        return true;
    }
    else
        return false;

    return det0*det1>= 0;
}


//Same as has intersect, but this time, do continue the ray
bool mesh2D::get_intersect(const vec2& A,const vec2& B, vec2& Out, uint& V0_ID, uint& V1_ID, float& AB2) const
{//Does any of my edges intersect with these? just asking for this, nevermind where

    //We need at least a line (3 points, since the first point is included twice to close the loop)
    if (size<3)
        return false;

    //Assume that A is the origin, and only B might be a point on the line

    float a = B.y - A.y;
    float b = A.x - B.x;


    //Discard using the bounding sphere as before
    vec2 A_to_sphere = A-Bsphere_center;
    vec2 ray_dir = vec2(-b,a);
    float D2 = dot(A_to_sphere,A_to_sphere);
    float det = pow(dot(A_to_sphere,ray_dir),2.f)-dot(ray_dir,ray_dir)*(D2-Bsphere_r2);

    if (det<0 && D2>Bsphere_r2)//We do not intersect, and we are not inside to being with
        return false;

    float c = a*(A.x) + b*(A.y);
    bool isX = approx(A.y,B.y);
    bool isY = approx(A.x,B.x);
    bool ret = false;
    //We will handle these special cases on their own, as they are super vulnerable to floating point errors, and besides, they are a lot simpler
    for (uint i = 0; i <size-1; ++i)
    {
        const vec2& C = vertices[i];
        const vec2& D = vertices[i+1];

        // Line CD represented as a1x + b1y = c1
        float a1 = D.y - C.y;
        float b1 = C.x - D.x;
        float c1 = a1*(C.x)+ b1*(C.y);
        //Selfcollision is never registered
        if (!(approx(C,B) || approx(D , B)))
        {
            if (isX)
            {
                if (!approx(a1,0))
                {
                    //We collide with this if
                    //a1 x + b1 A.y = c1
                    vec2 I = vec2((c1-b1*A.y)/a1,A.y);

                    if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)) || approx(b1 , 0))
                    {
                        if ((std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y)) || approx(a1,0))
                            if (dot(I-A,B-A)>0 )//If it is anywhere ahead of us
                                {
                                    float L2 = dot(I-A,I-A);
                                    if (L2<AB2 || AB2 < 0)//See if this intersection is better than the one we already have, if yes, yoinks, <0 is used when anything should be accepted
                                    {
                                        AB2 = L2;
                                        Out=I;
                                        V0_ID=i;
                                        V1_ID=(i+1)%size;
                                        ret = true;
                                    }
                                }
                    }
                }
                else
                {
            //Extreme rare where the ray and segment are perpendicular, and aligned to the axes, this would cause a divide by 0 error if handled in the full case, and it does happen ... very rare, but happens
                //Thankfully this is the easiest thing in the word to deal with
                if (C.y<std::max(A.y,B.y) && C.y>std::min(A.y,B.y))
                    if (A.x<std::max(C.x,D.x) && A.x>std::min(C.x,D.x))
                        {
                            vec2 I = vec2(A.x,C.y);
                            float L2 = dot(I-A,I-A);
                            if (L2<AB2 || AB2 < 0)
                            {
                                AB2 = L2;
                                Out=I;
                                V0_ID=i;
                                V1_ID=(i+1)%size;
                                ret = true;
                            }

                        }
  /*
                if (C.x<std::max(A.x,B.x) && C.x>std::min(A.x,B.x))
                    if (A.y<std::max(C.y,D.y) && A.y>std::min(C.y,D.y))
                        return true;
*/
                }
            }
            else if (isY && !approx(b1 , 0))
            {

                if (!approx(b1,0))
                {
                    //We collide with this if
                    //a1 A.x + b1 y = c1
                    vec2 I = vec2(A.x,(c1-a1*A.x)/b1);
                    if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)) || approx(b1,0))
                    {
                        if ((std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y))  || approx(a1,0))
                            if (dot(I-A,B-A)>0 )//If it is anywhere ahead of us
                                {
                                    float L2 = dot(I-A,I-A);
                                    if (L2<AB2 || AB2 < 0)
                                    {
                                        AB2 = L2;
                                        Out=I;
                                        V0_ID=i;
                                        V1_ID=(i+1)%size;
                                        ret = true;
                                    }
                                }
                    }
                }
                else
                {
                if (C.x<std::max(A.x,B.x) && C.x>std::min(A.x,B.x))
                    if (A.y<std::max(C.y,D.y) && A.y>std::min(C.y,D.y))
                        {
                            vec2 I = vec2(C.x,A.y);
                            float L2 = dot(I-A,I-A);
                            if (L2<AB2 || AB2 < 0)
                            {
                                AB2 = L2;
                                Out=I;
                                V0_ID=i;
                                V1_ID=(i+1)%size;
                                ret = true;
                            }

                        }
                }
            }
            else//Full case
            {


                float det = a*b1 - a1*b;
                if (!approx(det , 0))
                {
                    vec2 I = vec2((b1*c - b*c1),(a*c1 - a1*c))/det;

                    if ((std::max(C.x,D.x)>I.x && I.x>std::min(D.x,C.x)) || approx(b1,0))
                    {
                        if ((std::min(C.y,D.y)<I.y && I.y <std::max(C.y,D.y)) || approx(a1,0) )
                            if (dot(I-A,B-A)>=0 )//If it is anywhere ahead of us
                            {
                                float L2 = dot(I-A,I-A);
                                if (L2<AB2 || AB2 <0)
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

//This is only run when adding new points to the mesh ... which may very well only be done at startup, or even better, the center and radius may be saved to a file; moreover, the smaller our bounding sphere is the more calculations can be thrown away during the runtime calculations: So we can afford to use an expensive algorithm to find THE smallest bounding sphere ever.
// Lets not do that though, let us use one of the literally fastest algorithm for this problem: Welzl's algorithm ... ok faster optimizations may exist (i.e. one could use the fact that this is a closed loop which does not self intersect, something Megiddo's algorithm does not require), but this already works in O(n) which is great.
void mesh2D::recalc_bsphere()
{

    //Assert this is a valid polygon to begin with
    if (size>=3)
    {
        struct sphere_struct
        {
            vec2 center = vec2(0);
            float r2 = 0;
        };

        //The list of vertices and bounding points we will be drawing from, these are list of the indices
        vector<uint> P(size-1);
        vector<uint> R(3,-1);
        std::iota(P.begin(), P.end(), 0);

        std::random_device rd;
        std::default_random_engine rng(rd());
        shuffle(P.begin(), P.end(), rng);

        //Note that I FIRST declare the function, then define it on the next line, this is so that the compiler knows that the fucntion exists when I call it from inside itself
        std::function<sphere_struct(uint,uint)> welzl;
        welzl= [P,&R,this,&welzl](uint p_min, uint n_R) -> sphere_struct
        {
            sphere_struct result;

            if (n_R==3)
            {//Triangles are trivial ... Well ok, proving this will require you to have taken at least an introductory undergraduate course in linear algebra ... when I say trivial I simply mean that the solution is a set of well defined equations we simply can use now (see https://www.xarg.org/2018/02/create-a-circle-out-of-three-points/ )

                vec2 V0 = vertices[R[0]];
                vec2 V1 = vertices[R[1]];
                vec2 V2 = vertices[R[2]];

                float a = V0.x * (V1.y - V2.y) - V0.y * (V1.x - V2.x) + V1.x * V2.y - V2.x * V1.y;

                float b = (V0.x * V0.x + V0.y * V0.y) * (V2.y - V1.y)
                    + (V1.x * V1.x + V1.y * V1.y) * (V0.y - V2.y)
                    + (V2.x * V2.x + V2.y * V2.y) * (V1.y - V0.y);

                float c = (V0.x * V0.x + V0.y * V0.y) * (V1.x - V2.x)
                    + (V1.x * V1.x + V1.y * V1.y) * (V2.x - V0.x)
                    + (V2.x * V2.x + V2.y * V2.y) * (V0.x - V1.x);

                //Sadly we have to run a division now
                result.center.x=-b / (2 * a);
                result.center.y=-c / (2 * a);
                vec2 D = result.center-V0;//This is the same distance from everything so nevermind which point we use
                result.r2 = dot(D,D);
            }
            else if (p_min == size-1)
            {
                if (n_R==2)
                {//Two points known to be on the bounding sphere ... attempt the smallest sphere we can get away with
                    vec2 V0 = vertices[R[0]];
                    vec2 V1 = vertices[R[1]];
                    vec2 D = V0-V1;
                    result.center = V0*0.5f+V1*0.5f;
                    result.r2 = dot(D,D)*0.25f;//Divide the radius by 2, but this is squared so divide by 4, except don't do that, multiply with 0.25 instead because divisions are evil.
                }
                //if not ... Well that is just sad, we do not have enough information to get a radius, and thus EVERYTHING will be outside (except if there is one point in R, but we are guaranteed to never check for that again, so nevermind)
            }
            else
            {//Could not be resolved, call recursion on all other points EXCEPT the random point p (instead of picking it out now, we randomized the indices to begin with and let p be the first point in the list)
                result = welzl(p_min+1,n_R);

                //Now, is the point p not included here inside this? If not, it got to be on the boundary
                vec2 D = vertices[P[p_min]]-result.center;
                if (dot(D,D)>=result.r2)
                {
                    //It is NOT inside
                    //The point must be on the boundary (We get here automatically if r2=0 as the self dot product is positive or 0)
                    R[n_R]=P[p_min];
                    result = welzl(p_min+1,n_R+1);


                }// If it is inside we might be done, return it to the next level of the recursion to see if it is true
            }

            return result;
        };

        auto result = welzl(0,0);//Run the algorithm with all vertices and empty bound to start with

        Bsphere_r2 = result.r2;

        Bsphere_center= result.center;

        //Now create the debug buffers for display, they have already been generated before so just bind them and fill them
        #ifdef DEBUG_SHOW_BSPHERE
        if (graphic_mode)
        {
            Bsphere_vertices= vector<vec2> (32);

            float r = sqrt(Bsphere_r2);//This is one time only, and debug time only so squareroot is ok here, it is in the looped calculations where I don't want to do this
            float dtheta = TWO_PI/31.f;
            for (uint i =0; i<32; ++i)
            {
                float theta = i*dtheta;
                Bsphere_vertices[i]=Bsphere_center+r*vec2(cos(theta),sin(theta));
            }
        }
        #endif
    }
}

void welzl(vector<int>& P, vector<int>& R, int Pid, int Rid);
