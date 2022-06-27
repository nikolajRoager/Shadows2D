#define TWO_PI 6.28318531

#include <GL/glew.h>

/*THIS FILE

The declaration for the openGL shader loading functions, this is a part of the io::graphics namespace, but the functions are so long that they have been put elsewhere.
This function loads and compiles a vertex and fragment shader written in glsl (openGL Shader Language)

This should ONLY be included by IO::graphics
*/


#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include"IO.hpp"
#include"load_shader.hpp"



namespace IO::graphics
{

    //Defined later: get the log of the loaded program
    string get_log(GLuint program_or_shader);

    GLuint load_program(fs::path material_path, string name, string& log)
    {
        //This is extremely basic code for loading and compiling GLSL shaders, not many comments are needed

        //First things first, can we even load the code?

        string vertexShader_source;
        string fragmentShader_source;

        fs::path VSP = material_path / name / "vertex.glsl";
        fs::path FSP = material_path / name / "fragment.glsl";

        //Load vertex shader
        ifstream vertexShader_stream(VSP);

        if (!vertexShader_stream.is_open())
        {
            throw std::runtime_error("File not found: " + VSP.string());
        }
        else
        {
            //This is the fastest way I know of reading files using fstream... Yes I could have used c style memory copy, but I prefer to use c++ rather than c style syntax, wherever I can get away with it
            stringstream temp;
            temp << vertexShader_stream.rdbuf();
            vertexShader_source = temp.str();
            vertexShader_stream.close();
        }

        //Same thing for fragment shader
        ifstream fragmentShader_stream(FSP);

        if (!fragmentShader_stream.is_open())
        {
            throw std::runtime_error("File not found: " + FSP.string());
        }
        else
        {
            stringstream temp;
            temp << fragmentShader_stream.rdbuf();
            fragmentShader_source = temp.str();
            fragmentShader_stream.close();
        }



        //Create vertex shader target, attach and copile
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar* vertexShader_source_cstr = vertexShader_source.c_str();
        glShaderSource(vertexShader, 1, &vertexShader_source_cstr, nullptr);
        glCompileShader(vertexShader);

        //Did it work
        GLint vertexShader_good = GL_FALSE;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexShader_good);

        if (vertexShader_good != GL_TRUE)
        {
            string Error = get_log(vertexShader);
            glDeleteShader(vertexShader);
            throw std::runtime_error("Couldn't compile vertex shader:\n" + Error);
        }
        //Same for fragment shader:

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar* fragmentShader_source_cstr = fragmentShader_source.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShader_source_cstr, nullptr);
        glCompileShader(fragmentShader);

        //Did it work
        GLint fragmentShader_good = GL_FALSE;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentShader_good);

        if (fragmentShader_good != GL_TRUE)
        {
            string Error = get_log(fragmentShader);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            throw std::runtime_error("Couldn't compile fragment shader, error:\n" + Error);
        }


        //Do the actual program, not much to say here
        GLuint out = glCreateProgram();
        glAttachShader(out, vertexShader);
        glAttachShader(out, fragmentShader);
        glLinkProgram(out);

        //Check for errors
        GLint program_good = GL_TRUE;
        glGetProgramiv(out, GL_LINK_STATUS, &program_good);

        if (program_good != GL_TRUE)
        {
            string Error = get_log(out);
            glDetachShader(out, vertexShader);
            glDetachShader(out, fragmentShader);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            throw std::runtime_error("Error linking program:\n" + Error);
        }

        string Vlog = get_log(vertexShader);
        string Flog = get_log(vertexShader);
        string Plog = get_log(vertexShader);
        log = "";
        if (Vlog.size() != 0)
        {
            log = log + "---Vertex shader log---\n" + Vlog + "\n";
        }
        if (Flog.size() != 0)
        {
            log = log + "--Fragment shader log--\n" + Flog + "\n";
        }
        if (Plog.size() != 0)
        {
            log = log + "---Linker shader log---\n" + Plog + "\n";
        }

        glDetachShader(out, vertexShader);
        glDetachShader(out, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        //There you go, this is your program, if we got here in the code, it is definitely working
        return out;

    }

    //Get the error log of program or shaders which did not work
    string get_log(GLuint program_or_shader)
    {
        string out = "";

        //I like to just have one get-log function, so check what it is



        //This is really basic code, which you could copy from any one of thousands of tutorials, surely this should just come as a build in function in openGL. So only minimal comments will be made

        //Parameters needed, set later
        int log_length = 0;
        int max_length = log_length;


        if (glIsProgram(program_or_shader))
        {
            //get length (actually max length)
            glGetProgramiv(program_or_shader, GL_INFO_LOG_LENGTH, &max_length);

            //Make room for the log, then copy it where we want it
            char* log_c_str = new char[max_length];
            glGetProgramInfoLog(program_or_shader, max_length, &log_length, log_c_str);
            if (max_length > 0)
            {
                out += log_c_str;//Convert to c++ string
            }

            //Throw away the c string version
            delete[] log_c_str;
        }
        //The exact same thing, almost
        else if (glIsShader(program_or_shader))
        {
            //get length (actually max length)
            glGetShaderiv(program_or_shader, GL_INFO_LOG_LENGTH, &max_length);

            //Make room for the log, then copy it where we want it
            char* log_c_str = new char[max_length];
            glGetShaderInfoLog(program_or_shader, max_length, &log_length, log_c_str);
            if (max_length > 0)
            {
                out += log_c_str;//Convert to c++ string
            }

            //Throw away the c string version
            delete[] log_c_str;
        }
        else
            //I am sure noone is stupid enough to send us something which is neither a shader nor a program
        {
            out = "Not a valid program or shader";
        }

        return out;
    }
}
