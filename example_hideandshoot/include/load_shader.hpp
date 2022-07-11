#pragma once
/*THIS FILE

The declaration for the openGL shader loading functions, this is a part of the io::graphics namespace, but the functions are so long that they have been put elsewhere.

This function loads and compiles a vertex and fragment shader written in glsl (openGL Shader Language)

This should ONLY be included by IO::graphics
*/


//Need to call something which knows GLuint
#include <GL/glew.h>

#include<string>
#include<cstdint>

#include<filesystem>


//I use these types so much that these aliases are well worth it
using uint = uint32_t;
using uchar = uint8_t;
using ulong = uint64_t;

using namespace std;

namespace fs = std::filesystem;

namespace IO::graphics
{
    GLuint load_program(fs::path material_path, string name, string& log);//Load and compile openGL glsl program from shaders: assets/material_path/name/vertex.glsl assets/material_path/fragment.glsl
}

