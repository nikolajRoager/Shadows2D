#pragma once
#define DEBUG_SHOW_BSPHERE

#include <GL/glew.h>

#include <string>
#include <vector>
#include<filesystem>
#include<fstream>
#include<cstdint>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>


using namespace std;
using namespace glm;
using uchar = uint8_t;
using uint = uint32_t;
using ulong = uint64_t;

namespace fs = std::filesystem;

//A 2D shadow-casting and intersection denying polygon
class mesh2D
{
private:
    vector<vec2> vertices;//Assume we loop back on ourself
    GLuint vertexBuffer=-1;//Element-buffer is completely uncalled for in this case

    uint size=0;

    #ifdef DEBUG_SHOW_BSPHERE
    GLuint Bsphere_debug_Buffer=-1;
    #endif
    static bool graphic_mode;

    //Most important optimization, find the smallest possible sphere around this object, and only do intersection collision if the ray is pointed at it
    float Bsphere_r2 = 0;//radius squared turned out to be more useful, because sqrt is evil and must be avoided at all cost
    vec2 Bsphere_center = vec2(0);
    void recalc_bsphere(); //reset the bounding sphere

    float acc=1e-6;

    //Floating points numbers are EVIL, regular == sometimes fails because minor floating point errors have caused them to drift 0.0000000001 or something off. I decided NOT to use a macro function, because I WANT to get compiler warnigns if I try to do approx(int,int)
    inline bool approx(float a, float b) const
    {
        return std::abs(a-b)<acc;
    }
    inline bool approx(const vec2& a, const vec2& b) const
    {
        return std::abs(a.x-b.x)<acc && std::abs(a.y-b.y)<acc;
    }

public:
    mesh2D();
    mesh2D(vector<vec2>& V);
    mesh2D(mesh2D&& other);//Need move constructor
    ~mesh2D();

    static void toggle_graphics(bool val) {graphic_mode=val;}

    void add_vertex(vec2 New);
    const vector<vec2>& get_vertices() const{return vertices;}
    void display() const;

    uint get_size() const {return size;}

    void save(ofstream& OUT) const;

    bool has_intersect(const vec2& A,const vec2& B) const;
    bool continues(const vec2& O,uint i) const;
    bool get_intersect(const vec2& A,const vec2& B, vec2& Out, uint& V0_ID, uint& V1_ID , float& dist2) const;

};
