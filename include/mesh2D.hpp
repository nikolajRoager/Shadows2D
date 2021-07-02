#pragma once

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
using ushort= uint16_t;
using uint = uint32_t;
using ulong = uint64_t;

namespace fs = std::filesystem;

//A 2D shadow-casting and intersection denying polygon
class mesh2D
{
private:
    vector<vec2> vertices;//Assume we loop ack on ourself
    GLuint vertexBuffer=-1;//Element-buffer is completely uncalled for in this case

    ushort size=0;

    //I wanted to calculate a bounding sphere, to calculate when and when not to include this in the calculation, but the class is literally cursed, any including more variables will cause the program to inexplicably crash on any call to std::cout std::stoi or ... ok literally curse just means invokes undefined behavior somewhere, but there is literally now way of calculating where, so this can literally never be fixed. A non imbecile could trivially figure this out, but I am an imbecile so this is impossible.

    float empty_var = 0;
public:
    mesh2D();
    mesh2D(vector<vec2>& V, bool do_display=true);
    mesh2D(mesh2D&& other);//Need move constructor
    ~mesh2D();

    void add_vertex(vec2 New);
    const vector<vec2>& get_vertices() const{return vertices;}
    void display() const;

    ushort get_size() const {return size;}

    void save(ofstream& OUT) const;

    bool has_intersect(const vec2& A,const vec2& B) const;
    bool continues(const vec2& O,ushort i) const;
    bool get_intersect(const vec2& A,const vec2& B, vec2& Out, ushort& V0_ID, ushort& V1_ID , float& AB2) const;
};
