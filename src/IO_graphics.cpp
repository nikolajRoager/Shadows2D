/*THIS FILE

The definition for the IO::graphics namespace, which handles the 2D pixelated texture rendering. This namespace also handles text-box rendering, this might sound like something completely different which need to be given its own namespace, however text boxes are also textures, and openGL functionalities are used to generate them, so all attempts at separating this into its own namespace have not succeeded

General note, OpenGL is NOT object oriented, the OpenGL functions I use here are actually regular c functions.
*/

//Needed to unlock some more glm features

//openGL Extension Wrangler library
#include <GL/glew.h>
//Main SDL library
#include <SDL2/SDL.h>
//loading of png and jpg
#include <SDL2/SDL_image.h>
//openGL functionalities
#include <SDL2/SDL_opengl.h>

//Vector math
#include <glm/glm.hpp>
//Matrix math: gtx require experimental flag


using namespace glm;


#include "IO.hpp"
#include "load_shader.hpp"
//A texture wrapper, which includes not only the graphical information but also the name and path required to reload this texture (which may also be a text-box)
#include "texwrap.hpp"
#include "calligraphy.hpp"

//std string
#include <string>
//string-stream, for turning a string into a "stream" object, useful for loading plain text files
#include <sstream>
//text output, should never be used in looping functions
#include <iostream>
//operating independent file-system functions
#include<filesystem>
//file streams, we use filesystem for paths, and fstream to actually stream the data from files in.
#include <fstream>

//Dynamic sized list
#include <vector>

//Not so dynamic sized lists
#include <array>


namespace IO::graphics
{
    //Watch me continue to struggle defining the internal graphics variable in any sort of sensible order

    // ---- OpenGL setup and renderbuffers ----
    //Just one window for now ... actually ever, turns out SDL does not handle more than one window well.
    SDL_Window* window = nullptr;
    //Another thing which we only need to define one of ... it does important things behind the scene
    SDL_GLContext context;

    GLuint VertexArrayObject = -1;//I will create this, but I won't use it for anything actually, I just need to create it

    //In openGL we render to "framebuffers" which include both the rendered image and more buffers (depth buffers), framebuffer 0 always refers to the main screen

    //The main display is rendered to a texture for post processing (pixelation) and only rendered to the actual screen at the end, hence why I call this "display"
    GLuint display_Framebuffer = -1;
    GLuint display_texture = -1;

    //This framebuffer and texture is used for light and shadow calculations, using a raytracing approach which I am very proud of
    GLuint light_Framebuffer = -1;
    GLuint light_texture = -1;


    GLuint Filter_Framebuffer = -1;
    GLuint Temp_filter_texture = -1;
    //We could add in more framebuffers and textures if we want to do more effects,


    // ---- Constants related to the pixelation ----
    //This game is pixel-art based
    //All internal calculations and rendering are done with a viewport this size, we render this to a w by h texture which we then finally render to the screen, any future reference to pixels means pixels in the w by h display, not actual pixels on your screen... unless you scale the window way down, which you are welcome to do

    //Default values, but can be changed in the init function
    uint w = 320;
    uint h = 180;

    //Needed for scaling textures right
    float inv_w = 1.f / 320.f;
    float inv_h = 1.f / 180.f;

    float a_fixed = 0.5625;//=h/w, if win_h/win_w does not fit we will need to do some trickery


    //window width/height in external window pixels
    uint win_h;
    uint win_w;

    //Current way of handling if win_h/win_w is not h/w, keep aspect ratio of the internal display constant, now it won't take up the entire screen, so add a little offset
    //window offset of the top-left corner of the windows in external window pixels, needed to re-scale the mouse position
    uint offset_x;
    uint offset_y;

    float inv_s_fac;//How much to scale the screen to fit the window, actually only used for getting the mouse coordinates


    // ---- Other internal flags and data which we need to keep around ----
    //That is, things the graphics needs to remember between functions.
    bool rsz_scr = false;//Flag to tell us that the size of the window just changed, we may need to do a lot of updates later down the line, but these updates are relatively slow, so only do them if we need

    //Paths we load things from
    fs::path texture_path;
    fs::path fonts_path;
    fs::path shader_path;

    //----- Texture  memory management ----
    //The list of actual textures;
    vector<texwrap> textures;
    vector<uint> texs_users;//Which of the textures are not set to delete, track number of users to avoid double deletion

    tex_index textures_size = 0;//How many are there of these

    //Same overall idea for text boxes and sounds, note that text boxes are the same class and textures
    vector<texwrap> text_boxes;//Text boxes may be created and destroyed much more readily than textures, so leave them on their own
    vector<uint> texts_users;

    tex_index text_boxes_size = 0;

    mat3 Screen_matrix = mat3(1);//The transformation for the surface used to display the rescaled screen texture, will be manually calculated each time the screen size changes
    const mat3 unit_matrix_ref = mat3(1);//An identity matrix, this will be the default matrix used for non-animated textures:
    /*
    1 0 0
    0 1 0
    0 0 1
    */

    //---- The textured rectangle shader ----
    //A simple textured rectangle, used for basically everything
    GLuint surf_ProgramID = -1;

    //The singular attribute, the UV, aka. texture coordinate of the rectangle, which goes from 0,0 to 1,1
    GLuint surf_VertexUVAttribID = -1;

    GLuint surf_UVBuffer = -1;
    GLuint surf_ElementBuffer = -1;
    size_t surf_elements = 0;//How many triangle-vertices are there here?

    //These IDs are where the uniforms are stored on the graphics card, so that we can upload data to them
    GLuint surf_colorTex_ID = -1;//The identity of the texture in the surf program
    GLuint surf_matrix_ID=-1;//The tranformation matrix from UV coordinates to device coordinates in the surf program
    GLuint surf_tex_matrix_ID=-1;//Transformation of the texture, for instance for animation


    GLuint surf_lightTex_ID = -1;
    GLuint surf_use_light_ID = -1;
    GLuint surf_shadow_blend_ID = -1;
    GLuint surf_shadow_color_ID = -1;



    //A line which is the basis of the mesh-rendering engine
    GLuint Line_ProgramID = -1;

    //The vertex position of the lines drawn, in world-space
    GLuint Line_VertexPosAttribID = -1;

    GLuint Line_matrix_ID=-1;//The tranformation matrix from line-worldspace coordinates to device coordinates in the Line program
    GLuint Line_color_ID=-1; //The color of the line to be drawn


    //The program for drawing the area illuminated
    GLuint Light_ProgramID = -1;

    //The vertex position of the triangles drawn, in world-space
    GLuint Light_VertexPosAttribID = -1;

    GLuint Light_matrix_ID=-1;//The tranformation matrix, same as for the line program
    GLuint Light_color_ID=-1; //The color of the light at the source

    GLuint Light_origin_ID=-1; //Where does this light come from
    GLuint Light_range_ID=-1;  //If we are using some kind of abrupt fall-off, how far does it go



    //Information needed for the font
    vector<calligraphy> dictionary;//A list of textures of letters, defined in calligraphy.hpp/.cpp
    //Spacing between lines, and text height must be constant
    uchar text_v_space = 2;
    uchar text_h = 9;

    //Pre-declare some things we need in init
    void set_output_size(uint _w, uint _h);
    void loadCalligraphy(fs::path scripture);

    // ---- one time functions ----
    void init(bool fullscreen, string window_header, fs::path tex, fs::path scripture, fs::path shaders, uint _w, uint _h)
    {

        texture_path = tex;
        fonts_path = scripture;
        shader_path = shaders;

        //Get the error ready, just in case
        //TBD use std exception instead
        string error = "";

        //Initialize openGL version, this version should be good enough, it is not the newest, so some crazy experimental features are not there.
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);


        //Note this is a c style binary flag, the int here is just a list of 0's and 1's, each 1 corresponds to one thing we want to turn on, for instance this could be 0011 or something like that, the | operator is a binary or, (i.e. 0010 | 0001 = 0011)
        int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags))//the & is a binary and, if this thing is all 0, then something was not initialized correctly
        {
            error.append(" Could not initialize SDL_image with JPEG and PNG loading:\n\t\t");
            error.append(IMG_GetError());
            throw std::runtime_error(error);
        }


        win_w = 1920;
        win_h = 1000;

        //The transformation for the surface used to display the rescaled screen texture, will be manually calculated later
        //Create window and its surface
        SDL_DisplayMode DM;

        SDL_GetCurrentDisplayMode(0, &DM);
        win_w = DM.w;
        win_h = DM.h;

        cout << "Width " << win_w << " Height " << win_h << endl;

        uint FLAGS = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | ((fullscreen) ? SDL_WINDOW_FULLSCREEN : (SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED));

        window = SDL_CreateWindow(window_header.c_str(), 0, 0, win_w, win_h, FLAGS);


        if (window == nullptr)
        {
            end();
            throw std::runtime_error(SDL_GetError());
        }

        SDL_ShowCursor(SDL_DISABLE);

        context = SDL_GL_CreateContext(window);

        if (context == nullptr)
        {
            error.append("OpenGL context failed to be creted\n\t\t");
            error.append(SDL_GetError());
            throw std::runtime_error(error);
        }

        glewExperimental = GL_TRUE;
        GLenum glewError = glewInit();

        if (glewError != GLEW_OK)
        {
            error.append("Glew not initialized OK\n\t\t");
            error.append(string((char*)glewGetErrorString(glewError)));
        }

        //I have never actually needed to use this thing explicitly, and I am sure at least 90% of applications don't either, and until OpenGL 3.2 this would be defined behind the scenes but now we need to define it to get our programs to work... I would have prefered it it would just be done by OpenGL behind the scenes still, OpenGL just seems to require a little to many lines of setup already.
        glGenVertexArrays(1, &VertexArrayObject);
        glBindVertexArray(VertexArrayObject);


        //Use Vsync, this seems a little questionable on some computers
        if (SDL_GL_SetSwapInterval(1) < 0)
        {
            error.append("Could not set VSync\n\t\t");
            error.append(SDL_GetError());
            throw std::runtime_error(error);
        }

        //We have an open window, but nothing in it yet, this looks bad (it may include a random chunk of whatever was on the screen before the window was opened and it will look as if the program has crashed), it might take a few seconds to load so lets change the window to black

        //A most wonderfull background color
        glClearColor(0.0f, 0.0f, 0.0f, 1.f);
        //behind the scenes OpenGL has two buffers for the things to be shown to the window, we set one of them to black here
        glClear(GL_COLOR_BUFFER_BIT);
        //Then we swap the buffers, putting the black buffer into the window and we will keep it there until we have rendered the first frame to the other buffer
        SDL_GL_SwapWindow(window);

        //Alpha blending, aka transparant textures
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)

        //In case this was on by default!
        glDisable(GL_DEPTH_TEST);
        //We are not working in 3D, so this is not needed (likely not on by default) this would only draw triangles facing the camera, 2D makes this irrelevant
        glDisable(GL_CULL_FACE);



        //Loading the rectangular surface shader

        string log;
        surf_ProgramID = load_program(shader_path, "surface", log);

        surf_VertexUVAttribID = glGetAttribLocation(surf_ProgramID, "vertex_uv");

        surf_matrix_ID = glGetUniformLocation(surf_ProgramID, "UV_to_DC");
        surf_tex_matrix_ID = glGetUniformLocation(surf_ProgramID, "UV_transform");
        surf_colorTex_ID = glGetUniformLocation(surf_ProgramID, "colorSampler");

        surf_lightTex_ID = glGetUniformLocation(surf_ProgramID, "lightSampler");
        surf_use_light_ID = glGetUniformLocation(surf_ProgramID, "dynamic_light");
        surf_shadow_blend_ID = glGetUniformLocation(surf_ProgramID, "shadow_blend");
        surf_shadow_color_ID = glGetUniformLocation(surf_ProgramID, "shadow_color");



        cout << "Loaded surface program" <<endl;
        cout << log << endl;

        Line_ProgramID = load_program(shader_path, "line", log);

        Line_VertexPosAttribID = glGetAttribLocation(Line_ProgramID , "vertex_worldspace");

        Line_matrix_ID = glGetUniformLocation(Line_ProgramID , "worldspace_to_DC");
        Line_color_ID= glGetUniformLocation(Line_ProgramID , "color");





        glLineWidth(1.f);

        cout << "Loaded Line program " <<endl;
        cout << log << endl;



        Light_ProgramID = load_program(shader_path, "light", log);

        Light_VertexPosAttribID = glGetAttribLocation(Light_ProgramID , "vertex_worldspace");

        Light_matrix_ID = glGetUniformLocation(Light_ProgramID , "worldspace_to_DC");
        Light_color_ID= glGetUniformLocation(Light_ProgramID , "color");

        Light_origin_ID = glGetUniformLocation(Light_ProgramID , "origin");
        Light_range_ID= glGetUniformLocation(Light_ProgramID , "range");




        cout << "Loaded Light program " <<endl;
        cout << log << endl;


        static const GLfloat surface_uv_data[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
        };
        //Ok, maybe using an index-buffer is a little excessive with such a tiny object
        static const GLuint surface_index_data[] = { 0, 1 , 2, 1 ,3, 2 };
        surf_elements = 6;
        //Create VBOs

        //Load up the basic surface vertices into the GPU
        glGenBuffers(1, &surf_UVBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, surf_UVBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(surface_uv_data), surface_uv_data, GL_STATIC_DRAW);
        //Static draw since we will not need to update this at any point

        //One could debate if such a small object needs an elementbuffer at all
        glGenBuffers(1, &surf_ElementBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surf_ElementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surface_index_data), &(surface_index_data[0]), GL_STATIC_DRAW);



        //Just check that starting width and height is what we set it to
        int tw, th;//this width and height I first used just w and h and got confused when the later setup of the framebuffer gave me waaay to high resolution
        SDL_GetWindowSize(window, &tw, &th);
        win_h = th;
        win_w = tw;

        //set the internal output size
        set_output_size(_w, _h);

        //Now see how much we shall stretch our screen
        inv_s_fac = 1.f / std::min(win_h * 1.f / _h, win_w * 1.f / _w);

        //Create the matrix transformation for the rectangle displayed to the screen
        float this_a = win_h / float(win_w);


        float ox = -1;//offset of the screen rectangle in device coordinates
        float oy = -1;
        float sx = 2;//scaling of this rectangle
        float sy = 2;

        //Scaling factor (2*used_i/win_i), 2 because device coorindates are from -1 to 1 not 0 to 1, and the initial -1 are to move origin to the top left corner
        if (this_a < a_fixed)//Screen is "too wide"
        {
            uint used_w = win_h / a_fixed;//what width should we really use
            sx = used_w * 2.f / win_w;
            ox += 1.f - 0.5f * sx;
            offset_x = (win_w - used_w) / 2;
            offset_y = 0;



        }
        else//If the aspect ratio is perfect no padding will be added, but we do not need to treat that as a special case, just handle it alognside the screen is "too tall" case
        {
            uint used_h = win_w * a_fixed;//what width should we really use
            sy = used_h * 2.f / win_h;
            oy += 1.f - 0.5f * sy;
            offset_y = -(1.f - 0.5f * sy) * win_h * oy / 2.f;
            offset_x = 0;

        }

        //Also get the scaling and offset needed to fix the mouse position, we need those in pixels, remember device coordinates goes from -1 to 1, so width/height is 2/2, in pixels it is win_w/win_h.

        Screen_matrix =
            mat3(
                vec3(sx, 0, 0),
                vec3(0, -sy, 0),
                vec3(ox, -oy, 1)
            );

        //Now check what font we should load
        ifstream default_font_file((fonts_path / "default.txt"));
        string default_font = "null";
        bool found_font = false;
        if (default_font_file.is_open())
            if (getline(default_font_file, default_font))
                found_font = true;

        if (!found_font)
            throw std::runtime_error(string((fonts_path / "default.txt").string() + " not found"));

        dictionary = vector<calligraphy>();
        //Load font
        loadCalligraphy(fonts_path / default_font);


    }

    void end()
    {
        //Destroy everything
        SDL_DestroyWindow(window);
        window = nullptr;

        //Explicitly unload all buffers, framebuffers and textures provided they are not already set to default not set value (-1)
        if (VertexArrayObject != (GLuint)-1)
        {
            glDeleteVertexArrays(1, &VertexArrayObject);
            VertexArrayObject = -1;
        }

        if (surf_UVBuffer != (GLuint)-1)
        {
            glDeleteBuffers(1, &surf_UVBuffer);
            surf_UVBuffer = -1;
        }
        if (surf_ElementBuffer != (GLuint)-1)
        {
            glDeleteBuffers(1, &surf_ElementBuffer);
            surf_ElementBuffer = -1;
        }

        if (surf_ProgramID != (GLuint)-1)
        {
            glDeleteProgram(surf_ProgramID);
            surf_ProgramID = -1;
        }
        if (Line_ProgramID != (GLuint)-1)
        {
            glDeleteProgram(Line_ProgramID);
            Line_ProgramID = -1;
        }
        if (Light_ProgramID != (GLuint)-1)
        {
            glDeleteProgram(Light_ProgramID);
            Light_ProgramID = -1;
        }

        if (display_Framebuffer != (GLuint)-1)
        {
            glDeleteFramebuffers(1, &display_Framebuffer);
            display_Framebuffer = -1;
        }
        if (display_texture != (GLuint)-1)
        {
            glDeleteTextures(1, &display_texture);
            display_texture = -1;
        }
        if (light_Framebuffer != (GLuint)-1)
        {
            glDeleteFramebuffers(1, &light_Framebuffer);
            light_Framebuffer = -1;
        }
        if (light_texture != (GLuint)-1)
        {
            glDeleteTextures(1, &light_texture);
            light_texture = -1;
        }

        if (Filter_Framebuffer != (GLuint)-1)
        {
            glDeleteFramebuffers(1, &Filter_Framebuffer);
            Filter_Framebuffer = -1;
        }
        if (Temp_filter_texture != (GLuint)-1)
        {
            glDeleteTextures(1, &Temp_filter_texture);
            Temp_filter_texture = -1;
        }

        //Delet eopenGL context
        if (context != nullptr)
        {
            SDL_GL_DeleteContext(context);
            context = nullptr;
        }


        //Setting these to empty automatically calls destructor
        dictionary = vector<calligraphy>();

        //Check for Orphaned resources (resources which are still loaded when the program is closing)

        bool orphaned_res = false;

        cout<<"Deleting any remaining graphical resources"<<endl;
        for (tex_index i = 0; i < text_boxes_size; ++i)
        {
            uint I =texts_users[i];
            if (I>0)
            {
                cout<<I<<" users | text box : \""<<text_boxes[i].tell_name()<<"\""<<endl;
                orphaned_res=true;
            }
        }

        for (tex_index i = 0; i < textures_size; ++i)
        {
            uint I =texs_users[i];
            if (I>0)
            {
                cout<<I<<" users | texture : \""<<textures[i].tell_name(true)<<"\""<<endl;
                orphaned_res=true;
            }
        }

        if (!orphaned_res)
            cout<<"All graphical resources already unloaded"<<endl;
        else
            cout<<"All graphical resources has been deleted"<<endl;


        textures = vector<texwrap>();//Automatically calls destructor functions for all
        texs_users = vector<uint>();
        text_boxes = vector<texwrap>();
        texts_users = vector<uint>();

        textures_size = 0;
        text_boxes_size = 0;

        //Quit image module
        IMG_Quit();

    }


    // ---- Texture memory management ----
    // Functions for loading, deleting and displaying different types of textures and sounds

    tex_index load_tex(fs::path path, uint& w, uint& h, bool absolute_path)//Load a texture and gets its location in the texture list. Returns -1 = 65535 if it did not work, w and h will be set to the width and height (as they are references they can be read later, essentially a way of getting more than one output)
    {
    //If absolute path is false, then the path inside  texture_path
        if (!absolute_path)
            path = texture_path/path;
        for (tex_index t = 0; t < textures_size; ++t)
        {
            texwrap& T = textures[t];
            if (T.is(path))
            {
                textures[t].reload();//This un-sets all animation settings.
                ++texs_users[t];//Now this is used
                w = T.get_w();
                h = T.get_h();
                return t;
            }
        }
        //no catch here ... errors may be thrown, but they should be handled by the idiot who asked for a non-existing texture
        //Ok, now look for empty spaces to put this
        for (tex_index t = 0; t < textures_size; ++t)
        {
            if (texs_users[t]==0)//we found an empty space left by a deleted texture, take it
            {
                texs_users[t]=1;//Now this is used
                textures[t]=texwrap(path);
                w = textures[t].get_w();
                h = textures[t].get_h();
                return t;
            }
        }

        //if no other options were available push a new element back
        textures.push_back(texwrap(path));
        texs_users.push_back(1);//Now this is used

        const texwrap& T = textures[textures_size];
        w = T.get_w();
        h = T.get_h();

        ++textures_size;


        return textures_size -1;//Point to the last element in the list
    }


    tex_index load_tex(fs::path path, bool absolute_path)//Same function, but do not return width and height
    {
        uint tmp;//We are not interested in this, so ignore it
        return load_tex(path,tmp,tmp,absolute_path);
    }
    void add_tex_user(tex_index ID)//Add another user to this texture, to avoid deleting textures in use
    {
        if (ID <textures_size)
        {
            ++texs_users[ID];
        }

    }
    void delete_tex(tex_index ID)//Wipe this texture from memory (if no-one else uses it)
    {
        if (ID < textures_size)
        {//If this thing has already been marked as deleted, we have a problem, not a crash, but drop a warning
            if (texs_users[ID] == 0)
                cout<<"WARNING Double freed texture "<<textures[ID].tell_name(true)<<endl;
            else
                --texs_users[ID];

            //Now, if we have multiple unused textures at the very end of the list, we can delete them IGNORE COMPILER WARNINGS IN VISUAL STUDIO, I INTENTIONALLY USE UNDERFLOW
            for (tex_index i = textures_size- 1; i < textures_size; --i)//I is an unsigned integer, so when I=0 the next element is 0-1=65535 > textures_size
            {
                if (texs_users[i]!=0)
                {
                    break;
                }
                else
                {
                    texs_users.pop_back();
                    textures.pop_back();
                    --textures_size;
                }
            }

        }

    }
    void set_animation(tex_index ID, uint n_frames_w, uint n_frames_h)//Set this texture as actually an animation, the sprite sheet is n_frames_w wide and n_frames_h tall
    {
        if (ID<textures_size)
            textures[ID].set_animation(n_frames_w, n_frames_h);
    }


    //--Functions for loading text --
    //This is a part of the graphics, and could not be separated into a separate calligraphy namespace, because textboxes simply are textures which happen to contain text, we need access to the same internal functions to display them, and they are generated through OpenGL rendering, thus we can not separate it
    tex_index set_text(const string& text, bool force_unique)//Force unique forces this texture to be considered unique, i.e. another identical text won't be given a reference to this thing, this should only be set for text we plan to edit
    {
        if (!force_unique)//Provided that we are not obliged not to dublicate this
        {
            for (tex_index t = 0; t < text_boxes_size; ++t)
            {
                texwrap& T = text_boxes[t];
                if (T.is(text))
                {
                    text_boxes[t].reload();//This is certainly loaded now
                    ++texts_users[t];
                    return t;
                }
            }
        }

        //Ok, now look for empty spaces to put this

        for (tex_index t = 0; t < text_boxes_size ; ++t)
        {
            if (texts_users[t]==0)
            {
                texts_users[t]=1;//Now this is used
                text_boxes[t]=texwrap(text);
                return t;
            }
        }
        text_boxes.push_back(texwrap(text));
        texts_users.push_back(1);//Now this is used
        ++text_boxes_size;
        return text_boxes_size - 1;
    }
    //Note that the text ID are not the same as texture ID's (even though they are the same class behind the scene), i.e. we can have texture 1 and text 1 be different. This is because text is typically changed much more often than textures
    void add_text_user(tex_index ID)//Same procedure as with textures
    {
        if (ID <text_boxes_size )
        {
            ++texts_users[ID];
        }

    }
    void delete_text(tex_index ID)
    {
        if (ID < text_boxes_size)
        {

            if (texts_users[ID] == 0)
                cout<<"WARNING Double freed text "<<text_boxes[ID].tell_name()<<endl;
            else
                --texts_users[ID];

            //Now, delete anything not in use
            for (tex_index I = text_boxes_size- 1; I < text_boxes_size; --I)
            {
                if (texts_users[I] != 0)
                {
                    break;
                }
                else
                {
                    texts_users.pop_back();
                    text_boxes.pop_back();
                    --text_boxes_size;
                }
            }
        }
    }
    void overwrite_text(tex_index ID, const string& text)//Change this text with this ID
    {
        if (ID < text_boxes_size)
        {

            if (!text_boxes[ID].is(text))
            {
                //Assume this thing already has ownership of this texture
                text_boxes[ID] = texwrap(text);
            }
        }

    }

    //get a texture with the following key, should only be called on startup and only by the input_devices::init function, so we won't look for duplicates
    using Char = int;
    tex_index get_key_tex(Char key)
    {

        string name = SDL_GetKeyName(key);

        //Is this a single character
        bool simple = (name.length()==1);

        //Ok, now look for empty spaces to put this
        for (tex_index t = 0; t < textures_size; ++t)
        {
            if (texs_users[t]==0)
            {
                texs_users[t]=1;//Now this is used

                try
                {
                    if (simple)
                        textures[t]=texwrap(name);
                    else
                        textures[t]=texwrap(texture_path/"keys"/(name+".png"));
                }
                catch(...)
                {//Oh well, 404 then
                    textures[t]=texwrap(texture_path/"keys"/"404.png");
                }
                return t;
            }
        }

        //Otherwise, push back
        try
        {
            if (simple)
                textures.push_back(texwrap(name));
            else
                textures.push_back(texwrap(texture_path/"keys"/(name+".png")));
        }
        catch(...)
        {//Oh well, 404 then
            textures.push_back(texwrap(texture_path/"keys"/"404.png"));
        }
        texs_users.push_back(1);//Now this is used

        ++textures_size;


        return textures_size-1;
    }


    // ----  Display functions ----
    //There are a lot of options, most have default values:
    /*
    x and y are positions in pixels, negative position is allowed since somethign can be halfway outside the screen
    tex is the ID in the list of textures (gotten from load_tex)
    mirror (true flips image left to right)
    centered (controls whether (true) the x,y coordinates given is the center-bottom of the sprite, otherwise it is the lower left corner coordinate (CENTER BOTTOM IS MORE USEFUL THAN TRUE CENTER FOR SPRITES WHICH MUST STAND ON THE GROUND)
    inv_y (true: the lower edge of the screen is 0, false the top of the screen is 0)
    Actually all draw functions secretly calls the same hidden draw function
    */

    //The internally used rendering function is defined at the end of this file, it is not visible from the outside

    void internal_animate_sprite(int x, int y, const texwrap& Tex, uint frame, bool mirror , bool centered , bool inv_y );
    void draw_tex(int x, int y, tex_index tex, bool mirror , bool centered , bool inv_y )
    {
        //PLOT TWIST, this is the same draw function as for animations. If no animation is set frame 0 is the entire image
        //Note that tex is unsigned, so if tex=-1 then tex>textures_size
        if (tex < textures_size)
            internal_animate_sprite(x, y, textures[tex], 0, mirror, centered, inv_y);

    }
    void animate_sprite(int x, int y, tex_index tex, uint frame, bool mirror , bool centered , bool inv_y )
    {


        if (tex < textures_size)
            internal_animate_sprite(x, y, textures[tex], frame, mirror, centered, inv_y);
    }
    void draw_text(int x, int y, tex_index text, bool mirror, bool centered, bool inv_y)
    {
        if (text < text_boxes.size())
            internal_animate_sprite(x, y, text_boxes[text], 0, mirror, centered, inv_y);
    }

    // ---- Functions which must be called once per loop ----
    void clear()//Updates window  and resets the image, must be called each loop
    {

        //Clear all framebuffers
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, light_Framebuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, display_Framebuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        //Reset the viewport to internal size, (It can not be set on a per-framebuffer basis, so each time I want to render to the screen I need to set it to win_w, win_h, and then I need to set it back)
        glViewport(0, 0, w, h);


        //Test if width/height of window has changed
        int nw = 0;
        int nh = 0;
        SDL_GetWindowSize(window, &nw, &nh);

        if (nh != win_h || nw != win_w) //IGNORE THE WARNING, yes they are different signedness, but no computer screen in the world has a number of pixels even approaching the limit
        {
            win_w = nw;
            win_h = nh;
            rsz_scr = true;


            inv_s_fac = 1.f / std::min(win_h * 1.f / h, win_w * 1.f / w);

            float this_a = win_h / float(win_w);


            float ox = -1;//offset of the screen rectangle in device coordinates
            float oy = -1;
            float sx = 2;
            float sy = 2;
            //Scaling factor (2*used_i/win_i), 2 because device coorindates are from -1 to 1 not 0 to 1, and the initial -1 are to move origin to the top left corner
            if (this_a < a_fixed)//Screen is "too wide"
            {
                uint used_w = win_h / a_fixed;//what width should we really use
                sx = used_w * 2.f / win_w;
                ox += 1 - 0.5f * sx;

                offset_x = (win_w - used_w) / 2.f;//-(1-0.5*sx)*win_w*ox/2.f;
                offset_y = 0;

            }
            else//If the aspect ratio is perfect no padding will be added, but we do not need to treat that as a special case, just handle it alognside the screen is "too tall" case
            {
                uint used_h = win_w * a_fixed;//what width should we really use
                sy = used_h * 2.f / win_h;
                oy += 1.f - 0.5f * sy;

                offset_y = (win_h - used_h) / 2.f;
                offset_x = 0;
            }


            Screen_matrix =
                mat3(
                    vec3(sx, 0, 0),
                    vec3(0, -sy, 0),
                    vec3(ox, -oy, 1)
                );
            //The transformation for the surface used to display the rescaled screen texture, will be manually calculated later

        }
        else
        {
            rsz_scr = false;
        }



    }

    bool showlightmap = false;
    void debug_showlightmap()
    {
        showlightmap = !showlightmap ;
    }

    void flush()
    {

        set_shadow(0);

        //Simply draw a full square to the screen, which will include our rendered texture, stretching this to fit the entire screen with closest interpolation results in the pixelation effect
        //Now render the output to the screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)
        //glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);//Needed for light blending (true additive)
        //Enable what we need
        glUseProgram(surf_ProgramID);

        //Screen viewport, the only one which can change, but we need to update it every frame because OpenGL only stores one set of viewport variables globally, and not individual ones on a per framebuffer basis
        glViewport(0, 0, win_w, win_h);


        //OpenGL has a number of different texture "slots" here I activate slot number 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, showlightmap ? light_texture : display_texture);//Here I bind the texture I want to display, it is bound to the currently active texture slot (0 here)
        //1i tells opengl that this is 1 integer, 2f would be a 2-d vector with floating point data. It is very important that the type matches the data in the glsl shader.
        glUniform1i(surf_colorTex_ID, 0);//Here I set the uniform texture sampler to integer 0, OpenGL understand that this means it shall look in slot 0


        //OpenGL uniform MatrixN sends an N by N matrix, f says it has floating values and v tells us that we want to send a pointer to the values in the CPU memory, rather than setting the values manually
        glUniformMatrix3fv(
        surf_matrix_ID,
        1,                       //How many (OpenGL does support array uniforms), in this case there is only 1
        GL_FALSE,                //Should openGL transpose this for us, (transposing a matrix swaps element j,i into i,j), I don't see why this is useful enough to be a parameter here, I can just transpose the matrix myself before sending it to OpenGL
        &(Screen_matrix[0][0])   //Due to the way the glm library works, the matrix elements is stored in a list with 0,0 being the first. The order is correct for OpenGL to copy the data correctly if we tell it to start at the adress (&) of element 0,0, the paranthesis around the element is not needed, but I think it makes it more clear what order things happen in
        );
        //This is not an animation, send a unit matrix to "transform" the UV coordinates nowhere
        glUniformMatrix3fv(surf_tex_matrix_ID, 1, GL_FALSE, &(unit_matrix_ref[0][0]));//Same procedure as before


        //We only have one vertex attribute, these are the values which change from vertex to vertex
        glEnableVertexAttribArray(surf_VertexUVAttribID);
        glBindBuffer(GL_ARRAY_BUFFER, surf_UVBuffer);
        glVertexAttribPointer
        (
            surf_VertexUVAttribID,//Attribute location, you can either locate this in the program (with glGetAttributeLocation), or you can force the shader to use a particular location from inside the shader, I do the former
            2,                   //Number of the below unit per element (this is a 2D vector, so 2)
            GL_FLOAT,            //Unit, this is single precition GL float
            GL_FALSE,             //Normalized? Nope
            0,                    //Stride and offset, I don't use these for anything
            (void*)0
        );


        //Use our element buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surf_ElementBuffer);
        //The actual drawing part of this
        glDrawElements(GL_TRIANGLES, surf_elements, GL_UNSIGNED_INT, NULL);

        //Disable everything activated
        glDisableVertexAttribArray(surf_VertexUVAttribID);



        glUseProgram(0);

        //Now we want to see this
        SDL_GL_SwapWindow(window);//behind the scenes OpenGL has two buffers for the things to be shown to the window, now we put the buffer we just edited into the window so the image changes, and in the next loop we edit the other buffer.

    }

    GLuint string_to_texture(string message, uint& this_w, uint& this_h, bool bgr)//Should only be used internally by the texwrap class
    {
        glClearColor(0.0f, 0.0f, 0.0f, bgr ? 1.f : 0.f);

        //The number one question we have to know: How much room do we need to make ready in this viewport, for that, we need to see what glyphs we need, as I like to keep open the possibility of variable size glyphs-maps. So let us just write a list of the letters we need

        //TBD NO TEXT WRAP, ADD THAT LATER
        this_w = 0;
        this_h = 11;

        uint ts = message.size();//Get the number of c++ 8 bit chars needed to store this, keeping in mind that this is NOT the same as the number of characters to be displayed

        struct Glyph
        {
            tex_index tex_map;
            tex_index tex_id;
            uchar  baseline_drop;
            uchar  width;

            Glyph(tex_index T, uint i, uchar d, uchar w)
            {
                tex_map = T;
                tex_id = i;
                baseline_drop = d;
                width = w;
            }
        };
        vector<Glyph> glyphs;
        for (tex_index i = 0; i < ts; ++i)
        {


            char c = message[i];
            uint C = 0;//The ancient Romans used an alphabet which could fit in one byte, but that is one of the things which we have lost and forgotten, newspeak uses pictograms (unicode 1F300 to 1F64F and however many extensions they have come up with), oh, and I would like to open up the possibility of moding in any alphabet so 32 bits are used here
            if (!(c & 0b10000000))//U 0000 to U 007F
            {
                C = c;
            }
            else if ((c & 0b11100000) == 0b11000000)//U 0080 to UTF07FF
            {
                //Blimey, we have to stitch together something

                if (i < ts - 1)
                {



                    C = ((c & 0b00011111) << 6) | ((message[i + 1] & 0b00111111));
                    ++i;

                }
            }
            else if ((c & 0b11110000) == 0b11100000)//U 0800 to U FFFF
            {
                //Oh no... well I need this
                if (i < ts - 2)
                {
                    C = (((c & 0b00011111) << (12)) | ((message[i + 1] & 0b00111111) << 6) | ((message[i + 2] & 0b00111111)));
                    i += 2;
                }
            }
            else if ((c & 0b11111000) == 0b11110000)//U 10000 to U 10FFFF
            {
                //yikes
                if (i < ts - 3)
                {
                    C = ((c & 0b00001111) << (18)) | ((message[i + 1] & 0b00111111) << 12) | ((message[i + 2] & 0b00111111) << 6) | ((message[i + 3] & 0b00111111));
                    i += 3;
                }
            }
            else
            {
                //AAAAaaaaargh (quitely ignore possibly corrupted text)
            }


            //Find any calligraphical table which supports this script
            bool found = false;
            for (const calligraphy& cal : dictionary)
            {
                if (cal.unicode_min <= C && cal.unicode_max >= C)
                {
                    uchar D = 0;
                    for (const pair<uint, uchar>& Dr : cal.baseline_drop)
                        if (Dr.first + cal.unicode_min == C)
                            D = Dr.second;

                    glyphs.push_back(Glyph(cal.texture, C - cal.unicode_min, D, cal.get_glyph_width(C - cal.unicode_min)));
                    //animate_sprite_screen(x,y-D,cal.texture,uint(C-cal.unicode_min));
                    this_w += cal.get_glyph_width(C - cal.unicode_min);

                    found = true;
                    break;//Yes, in some cases there may be overlap between the supported blocks, but only display the first match (would be first loaded)
                }
            }

            //Oh no, most likely a result of modding in new language support
            if (!found)
            {
                stringstream stream;
                stream << std::hex << C;
                throw std::runtime_error("Font does not support Unicode character U+0x"+stream.str()+"In text "+message);
            }
        }
        this_h = text_h + text_v_space;
        //Add one pixel in front in case we use contrasting background
        ++this_w;

        //Prepare the internal viewport
        glBindFramebuffer(GL_FRAMEBUFFER, Filter_Framebuffer);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)
        //glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);//Needed for light blending (true additive)

        //Reset this texture, whatever it points to now has likely already been send off somewhere else, and that elsewhere will remember to delete it when its time has come.
        Temp_filter_texture = -1;
        glGenTextures(1, &Temp_filter_texture);
        glBindTexture(GL_TEXTURE_2D, Temp_filter_texture);

        //Initialize empty, and at the size of the internal screen
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this_w, this_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        //No interpolation, we want pixelation
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


        GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
        //Now the Text framebuffer renders to the texture we finally Text
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Temp_filter_texture, 0);
        glDrawBuffers(1, DrawBuffers);//Color attachment 0 as before


        glViewport(0, 0, this_w, this_h);

        //Get the inverse of a pixel in this one framebuffer
        float inv_px_x = 1.f / this_w;
        float inv_px_h = 1.f / this_h;

        glClear(GL_COLOR_BUFFER_BIT);

        //Now draw everything

        int x = 1;
        int y = 0;

        glUseProgram(surf_ProgramID);

        for (Glyph& G : glyphs)
        {

            //Enable what we need


            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[G.tex_map].get_tex());
            glUniform1i(surf_colorTex_ID, 0);

            uint fw = textures[G.tex_map].get_frame_w();
            uint fh = textures[G.tex_map].get_frame_h();

            mat3 thisMVP =
                mat3(vec3(2.f * inv_px_x * fw, 0, 0), vec3(0, 2.f * inv_px_h * fh, 0), vec3(2.f * inv_px_x * x - 1.f, 2.f * inv_px_h * (y + G.baseline_drop) - 1.f, 1));//*Tex.getM(mirror,centered);
            mat3 this_UVM = textures[G.tex_map].getUVM(G.tex_id);


            glUniformMatrix3fv(surf_matrix_ID, 1, GL_FALSE, &thisMVP[0][0]);
            //Absolutely not an animation, send a unit matrix to "transform" the UV coordinates nowhere
            glUniformMatrix3fv(surf_tex_matrix_ID, 1, GL_FALSE, &this_UVM[0][0]);

            glEnableVertexAttribArray(surf_VertexUVAttribID);

            glBindBuffer(GL_ARRAY_BUFFER, surf_UVBuffer);
            glVertexAttribPointer
            (
                surf_VertexUVAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
                2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
                GL_FLOAT,            //Unit, this is single precition GL float
                GL_FALSE,             //Normalized? Nope
                0,                    //Stride and offset, I don't use these for anything
                (void*)0
            );


            //Use our most excellent element buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surf_ElementBuffer);
            //The actual drawing part of this
            glDrawElements(GL_TRIANGLES, surf_elements, GL_UNSIGNED_INT, NULL);

            //Disable everything activated
            glDisableVertexAttribArray(surf_VertexUVAttribID);

            //We have added up both the width of the font and the spacing
            x += G.width;
        }
        glUseProgram(0);

        //Return to the old display and the old viewport size
        glBindFramebuffer(GL_FRAMEBUFFER, display_Framebuffer);
        glViewport(0, 0, w, h);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)
        return Temp_filter_texture;
    }

    // ---- Functions for getting information about the graphical setup ----

    //Get width and height of internal display in pixels
    //this is NOT the screen size if the display is pixelated
    //These functions for returning this is not defined in the IO.hpp, but one namespace which do know these is IO::input_devices and the texture wrapper class, which need them for finding the mouse position in internal pixel coordinates,
    uint get_w()
    {
        return w;
    }
    uint get_h()
    {

        return h;
    }
    //Get 1/w and 1/h
    float get_inv_w()
    {

        return inv_w;
    }
    float get_inv_h()
    {

        return inv_h;
    }


    uchar get_text_v_space()
    {
        return text_v_space;
    }
    uchar get_text_h()
    {
        return text_h;
    }

    uint get_offset_x() {return offset_x;}
    uint get_offset_y() {return offset_y;}
    uint get_win_w()    {return win_w;}
    uint get_win_h()    {return win_h;}
    float get_inv_s_fac(){return inv_s_fac;}



    //---Print ram ----
    void print_ram()//Print currently loaded textures to the external terminal (usually doesn't fit in the developer commandline)
    {
        cout<<"---Regular textures ("<<textures_size<<") ---"<<endl;
        cout<<" ID   | Users | path"<<endl;
        for (tex_index i = 0; i < textures_size; ++i)
        {
            cout<<' '<<i;

            if (i<10)
                cout<<' ';
            if (i<100)
                cout<<' ';
            if (i<1000)
                cout<<' ';
            cout<<" | ";

            tex_index I =texs_users[i];
            cout<<I;

            if (I<10)
                cout<<' ';
            if (I<100)
                cout<<' ';
            if (I<1000)
                cout<<' ';
            cout<<"  | ";
            cout<<textures[i].tell_name(true);

            cout<<endl;
        }
        cout<<"---Text boxes ("<<text_boxes_size<<") ---"<<endl;
        cout<<" ID   | Users | path"<<endl;
        for (tex_index i = 0; i < text_boxes_size; ++i)
        {
            cout<<' '<<i;

            if (i<10)
                cout<<' ';
            if (i<100)
                cout<<' ';
            if (i<1000)
                cout<<' ';
            cout<<" | ";

            tex_index I =texts_users[i];
            cout<<I;

            if (I<10)
                cout<<' ';
            if (I<100)
                cout<<' ';
            if (I<1000)
                cout<<' ';
            cout<<"  | ";
            cout<<text_boxes[i].tell_name(true);

            cout<<endl;
        }
    }

    //It is possible to run at different settings, but changing outside init is not recommended
    //Set up the render-targets which are not the screens, this looks very convoluted because OpenGL gives a lot of options which we won't use in this case: I just want to render to a texture, but I create a Framebuffer, which is the thing I actually renders too, which the texture is attached to; actually Framebuffers can contain many textures or buffers to encode more information, such as depth ... or whatever really, OpenGL is an extremely general tool.
    void set_output_size(uint _w, uint _h)
    {
        w = _w;
        h = _h;

        inv_w = 1.f / w;
        inv_h = 1.f / h;


        a_fixed =float(h)/float(w);//, if win_h/win_w does not fit we will need to do some trickery

        //First, make sure that the Framebuffers and textures are empty.
        if (display_Framebuffer != (GLuint)-1)
        {
            glDeleteFramebuffers(1, &display_Framebuffer);
            display_Framebuffer = -1;
        }
        if (display_texture != (GLuint)-1)
        {
            glDeleteTextures(1, &display_texture);
            display_texture = -1;
        }
        if (light_Framebuffer != (GLuint)-1)
        {
            glDeleteFramebuffers(1, &light_Framebuffer);
            light_Framebuffer = -1;
        }
        if (light_texture != (GLuint)-1)
        {
            glDeleteTextures(1, &light_texture);
            light_texture = -1;
        }
        if (Filter_Framebuffer != (GLuint)-1)
        {
            glDeleteFramebuffers(1, &Filter_Framebuffer);
            Filter_Framebuffer = -1;
        }
        if (Temp_filter_texture != (GLuint)-1)
        {
            glDeleteTextures(1, &Temp_filter_texture);
            Temp_filter_texture = -1;
        }

        //Set up the display we will render to
        glGenFramebuffers(1, &display_Framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, display_Framebuffer);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)

        glGenTextures(1, &display_texture);
        glBindTexture(GL_TEXTURE_2D, display_texture);
        //Initialize empty, and at the size of the internal screen
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

        //No interpolation, we want pixelation
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


        //Now the display framebuffer renders to the texture we finally display
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, display_texture, 0);
        GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, DrawBuffers);//Color attachment 0 as before


        //Set up the light map we will use for lighting calculation
        glGenFramebuffers(1, &light_Framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, light_Framebuffer);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)
        glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);//Needed for light blending (true additive)

        glGenTextures(1, &light_texture);
        glBindTexture(GL_TEXTURE_2D, light_texture);
        //Initialize empty, and at the size of the internal screen
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

        //No interpolation, we want pixelation
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //Now the light framebuffer renders to the texture we will use to calculate dynamic lighting
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, light_texture, 0);
        glDrawBuffers(1, DrawBuffers);//Color attachment 0 as before


        glGenFramebuffers(1, &Filter_Framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, Filter_Framebuffer);

        glGenTextures(1, &Temp_filter_texture);
        glBindTexture(GL_TEXTURE_2D, Temp_filter_texture);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)

        //Initialize empty, ok, we will likely have to re-set this every time we render some new text
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

        //No interpolation, we want pixelation
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


        //Now the Text framebuffer renders to the texture, which will become the text string-textures
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Temp_filter_texture, 0);
        glDrawBuffers(1, DrawBuffers);//Color attachment 0 as before


        //Now we can render to whatever we want by using:
        //glBindFramebuffer(GL_FRAMEBUFFER,0);//Final output i.e. what the user to see this, but not used internally because we want low resolution for pixelation effects
        //glBindFramebuffer(GL_FRAMEBUFFER,display_Framebuffer);//Default rendertarget used here, renders result to a texture which we will display to screen (with framebuffer 0)
        //glBindFramebuffer(GL_FRAMEBUFFER,Filter_Framebuffer);//Render to a texture, which we will use to render arbitrary text

        //by the way I DID NOT FORGET glViewport, but it does not go in the setup, because OpenGL only stores one viewport size globally, it can not be set on a per-framebuffer basis (As one would expect), 1and since we are using several different sized viewports per display we will need to manyally call it twice per loop anyway (in case you wonder: no, you do not need to set viewport before calling glClear, many code examples on the internet do put their viewport call very early because they only use one size framebuffer)
    }

    //---- OpenGL rendering functions used internally----



    //A utility function to tell if a texture is even partially inside the screen
    bool inside(int x, int y, const texwrap& tex)
    {//Is this texture inside the screen

        //Need to save this as int, as the IDIOT compiler interprets x+wT as unsigned, if either one is unsigned
        int wT = tex.get_w();
        int hT = tex.get_h();



        //Give a bit of extra room
        return (x + wT >= 0 || x <= w + wT) && (y + hT >= 0  ||  y <= h + hT);//IGNORE THE WARNING, yes there are mixed signedness in some cases, but it does not matter as the pixel width/height will come nowhere close to max int
    }

    //The internal rendering function, needs to be defined in this file as they need to use the internal variables visible only here
    int use_light = 0; //0 off, 1 blend if in shadow, 2 blend if in light
    float shadow_blend_factor=0;
    vec4 shadow_blend_color = vec4(0);

    void set_shadow(int mode , float factor, vec4 blend_color)
    {
        use_light =mode;
        shadow_blend_factor = factor;
        shadow_blend_color = blend_color;
    }

    void internal_animate_sprite(int x, int y, const texwrap& Tex, uint frame, bool mirror , bool centered , bool inv_y )
    {
        y = (inv_y) ? h - y - 1 : y;//The right way around coordinate system works better for everything except for display


        if (inside(x, y, Tex) && Tex.get_tex() != (GLuint)-1)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, display_Framebuffer);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)

            //Enable what we need
            glUseProgram(surf_ProgramID);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, Tex.get_tex());
            glUniform1i(surf_colorTex_ID, 0);


            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, light_texture );
            glUniform1i(surf_lightTex_ID , 1);


            glUniform1i(surf_use_light_ID , use_light);
            glUniform1f(surf_shadow_blend_ID , shadow_blend_factor);
            glUniform4f(surf_shadow_color_ID , shadow_blend_color.x,shadow_blend_color.y,shadow_blend_color.z,shadow_blend_color.w);



            //The model-view-projection matrix is the combined matrix which takes the corners of this rectangle from the (0,0),(0,1), (1,0) and (1,1) to wherever we want them to be on the screen, so that the texture has the right dimensions, the first matrix defined is just a transformation matrix, the matrix we get from the texture includes how this texture wants to be transformed (depending on whether it wants to be centered or mirrored) this includes the scaling of the texture into Normalized Device coordinates
            mat3 thisMVP =
                mat3(vec3(1, 0, 0), vec3(0, 1, 0), vec3(x * 2 * inv_w - 1.f, y * 2 * inv_h - 1.f, 1)) * Tex.getM(mirror, centered);



            //How should we transform the texture coordinates of the cubes corner, to only show the frame we are interested in, if this is not animated, this is just an identity matrix
            mat3 this_UVM = Tex.getUVM(frame);


            glUniformMatrix3fv(surf_matrix_ID, 1, GL_FALSE, &thisMVP[0][0]);
            glUniformMatrix3fv(surf_tex_matrix_ID, 1, GL_FALSE, &this_UVM[0][0]);

            glEnableVertexAttribArray(surf_VertexUVAttribID);

            glBindBuffer(GL_ARRAY_BUFFER, surf_UVBuffer);
            glVertexAttribPointer
            (
                surf_VertexUVAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
                2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
                GL_FLOAT,            //Unit, this is single precition GL float
                GL_FALSE,             //Normalized? Nope
                0,                    //Stride and offset, I don't use these for anything
                (void*)0
            );


            //Use our most excellent element buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surf_ElementBuffer);
            //The actual drawing part of this
            glDrawElements(GL_TRIANGLES, surf_elements, GL_UNSIGNED_INT, NULL);

            //Disable everything activated
            glDisableVertexAttribArray(surf_VertexUVAttribID);
            glUseProgram(0);

        }

    }

    //The loading of the font, really only ever called in the init, but so long that it is placed separately
    void loadCalligraphy(fs::path scripture)
    {
        string temp;
        text_h = -1;//Mark as unset
        ifstream texture_data((scripture / "calligraphy.txt"));
        if (texture_data.is_open())
        {
            getline(texture_data, temp);
            text_v_space = stoi(temp);
            //If this fails to load, we might not want the program to quit, so only commit these changes once we know this works
            while (true)
            {
                uint calligraphy_min = -1;
                uint calligraphy_max = -1;
                uint calligraphy_cols = -1;
                uint calligraphy_rows = -1;
                uint calligraphy_h_space = -1;
                bool monospace = false;
                vector<pair<uint, uchar> > drop = vector<pair<uint, uchar> >();

                string _name;
                //This file should be structured exactly like this when created

                try
                {
                    //Please read the README file in the fonts folder for a description of what's what
                    if (!getline(texture_data, _name))
                        break;
                    getline(texture_data, temp);
                    calligraphy_min = stoi(temp, 0, 16);//Hexadecimal
                    getline(texture_data, temp);
                    calligraphy_max = stoi(temp, 0, 16);
                    getline(texture_data, temp);
                    calligraphy_cols = stoi(temp);
                    getline(texture_data, temp);
                    calligraphy_rows = stoi(temp);

                    //Drop below the baseline of various characters
                    getline(texture_data, temp);
                    if (temp.length() != 0)
                    {
                        stringstream ss(temp);

                        uint pos;
                        uint _drop;

                        while (ss >> pos)
                        {
                            ss >> _drop;
                            drop.push_back(pair<uint, uchar>(pos, (uchar)_drop));
                        }
                        cout << endl;

                    }
                    getline(texture_data, temp);
                    calligraphy_h_space = stoi(temp);
                    getline(texture_data, temp);
                    monospace = (0 != temp.compare("autospace"));
                }
                catch (...)
                {
                    break;
                }
                //Perform sanity check
                if (
                    calligraphy_min == (uint)-1    ||
                    calligraphy_max == (uint)-1    ||
                    calligraphy_cols == (uint)-1 ||
                    calligraphy_rows == (uint)-1
                    )
                    throw std::runtime_error("Could not load data in calligraphy.txt in " + scripture.string());

                //Now try to load the texture
                tex_index calligraphy_texture = load_tex(scripture / _name,true);//Using absolute path, as the font is not inside the texture folder

                if (calligraphy_max == (uint)-1)
                    throw std::runtime_error("Could not load " + _name + " in " + scripture.string());

                textures[calligraphy_texture].set_animation(calligraphy_cols, calligraphy_rows);

                uint frame_w = textures[calligraphy_texture].get_frame_w();
                uint frame_h = textures[calligraphy_texture].get_frame_h();

                if (text_h == (uchar)-1)//If height has not been set, now it is
                    text_h = frame_h;
                else if (text_h != frame_h)//But if height has been set, it must be consistent
                    throw std::runtime_error("Inconsistent glyph height in " + scripture.string());

                vector<uchar > glyph_width_list = vector<uchar >();
                //Now time to see how wide the glyphs are, assuming they are written as far left as possible in their rectangle
                if (!monospace)
                {//I found that it is simply easier to load the same texture twice, rather than adding a custom backdoor into the texture loading class to copy the data out
                    SDL_Surface* texture_surface = IMG_Load((scripture / _name).string().c_str());


                    if (texture_surface == NULL)
                    {
                        throw std::runtime_error("Couldn't load font texture " + _name + " in font " + scripture.string() + " to surface: " + string(IMG_GetError()));
                    }



                    uint bpp = texture_surface->format->BytesPerPixel;


                    //This is where the pixel data lives now
                    const uchar* data = (uchar*)texture_surface->pixels; //A pointer to the actual data,
//                    uint px_w = texture_surface->w;
//                    uint px_h = texture_surface->h;
                    uint pitch = texture_surface->pitch;
                    //We need to check the colored bytes only, where are they
                    uchar Rloc = 0;//ok, they are not always in this order
                    uchar Bloc = 1;
                    uchar Gloc = 2;

                    //From my testing it seems that this order is the same, EVEN ON LITTLE ENDIAN SYSTEMS. It would appear the loading fixes that already

                    uint x0 = 0;
                    uint y0 = 0;

                    //Loop through every glyph, one column at the time, starting from the left, until you find the first none blank column, that is how wide the glyph is
                    for (uint num = 0; num < calligraphy_max - calligraphy_min; ++num)
                    {

                        x0 = frame_w * (num % calligraphy_cols);
                        y0 = frame_h * (num / calligraphy_cols);

                        uchar glyph_width = frame_w;//Default to full width, this means that full space becomes full width
                        for (uint x = 0; x < frame_w; ++x)
                        {
                            for (uint y = 0; y < frame_h; ++y)
                            {
                                uchar r = data[(y + y0) * pitch + (frame_w - x - 1 + x0) * bpp + Rloc];
                                uchar g = data[(y + y0) * pitch + (frame_w - x - 1 + x0) * bpp + Gloc];
                                uchar b = data[(y + y0) * pitch + (frame_w - x - 1 + x0) * bpp + Bloc];
                                //Alpha channel is quitely ignored
                                uchar val = std::max(r, std::max(g, b));
                                if (val != 0 && glyph_width == frame_w)
                                {
                                    glyph_width = frame_w - x;
                                    x = frame_w;//Break all loops
                                    y = frame_h;
                                    break;
                                }
                            }
                        }
                        glyph_width_list.push_back(glyph_width);
                    }

                    SDL_FreeSurface(texture_surface);
                }

                //Commit changes to memory
                dictionary.push_back
                (
                    calligraphy
                    (
                        calligraphy_min,
                        calligraphy_max,
                        calligraphy_cols,
                        calligraphy_rows,
                        calligraphy_texture,
                        monospace ? frame_w : (uchar)-1,
                        calligraphy_h_space,
                        drop,
                        glyph_width_list
                    )
                );
            }
        }
        else
            throw std::runtime_error("Could not open  calligraphy.txt in " + scripture.string());

        if (text_h == (uchar)-1)
            throw std::runtime_error("Text height could not be deduced in " + scripture.string() + ", may be due to uncaught image loading errors or in deducing rows and cols");

    }

    SDL_Window * get_window()
    {
        return window ;
    }

    //-- Mesh rendering functions --

    //Internal drawing function, which does the drawing in different ways
    void draw_unicolor(const vector<vec2>& vertices, uint size, vec3 color,GLuint displaymode,vec2 offset= vec2(0));

    void draw_lines(const vector<vec2>& vertices, uint size, vec3 color,vec2 offset)
    {
        draw_unicolor(vertices,size,color,GL_LINE_STRIP,offset);
    }

    void draw_triangles(const vector<vec2>& vertices, uint size, vec3 color,vec2 offset)
    {
        draw_unicolor(vertices,size,color,GL_TRIANGLE_FAN,offset);
    }
    void draw_segments(const vector<vec2>& vertices, uint size, vec3 color,vec2 offset)
    {
        draw_unicolor(vertices,size,color,GL_LINES,offset);
    }


    //A more advanced version of draw unicolor, specifically made for drawing the triangle fan for the lighting, has a certain range which the color will fall off until it reaches
    void draw_triangles(const vector<vec2>& vertices, uint size, vec3 color,vec2 origin, float range,vec2 offset)
    {
        GLuint Buffer=-1;

        size =size > vertices.size() ? vertices.size() : size;
        glGenBuffers(1, &Buffer);
        glBindBuffer( GL_ARRAY_BUFFER, Buffer);
        glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*(size), &(vertices[0]), GL_DYNAMIC_DRAW );


        //Enable what we need
        glUseProgram(Light_ProgramID);


        mat3 thisMVP =
                mat3(vec3(2*inv_w, 0, 0), vec3(0, -2*inv_h, 0), vec3(-2*offset.x*inv_w - 1.f,  2*offset.y*inv_h+ 1.f, 1));

        glUniformMatrix3fv(Light_matrix_ID, 1, GL_FALSE, &thisMVP[0][0]);
        glUniform3f(Light_color_ID,color.x,color.y,color.z);
        glUniform2f(Light_origin_ID,origin.x,origin.y);
        glUniform1f(Light_range_ID,range);

        glEnableVertexAttribArray(Light_VertexPosAttribID);


        glBindBuffer( GL_ARRAY_BUFFER, Buffer);
        glVertexAttribPointer
        (
            Light_VertexPosAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
            2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
            GL_FLOAT,            //Unit, this is single precition GL float
            GL_FALSE,             //Normalized? Nope
            0,                    //Stride and offset, I don't use these for anything
            (void*)0
        );

        glDrawArrays(GL_TRIANGLE_FAN,0,size);

        //Disable everything activated
        glDisableVertexAttribArray(Light_VertexPosAttribID);
        glUseProgram(0);

        //Delete the buffer of this object
        if (Buffer != (GLuint)-1)
            glDeleteBuffers(1,&Buffer);


    }

    void draw_unicolor(const vector<vec2>& vertices, uint size, vec3 color,GLuint displaymode, vec2 offset)
    {
        GLuint Buffer=-1;

        size =size > vertices.size() ? vertices.size() : size;
        glGenBuffers(1, &Buffer);
        glBindBuffer( GL_ARRAY_BUFFER, Buffer);
        glBufferData( GL_ARRAY_BUFFER,  sizeof(vec2)*(size), &(vertices[0]), GL_DYNAMIC_DRAW );

        //Enable what we need
        glUseProgram(Line_ProgramID);


        mat3 thisMVP =
                mat3(vec3(2*inv_w, 0, 0), vec3(0, -2*inv_h, 0), vec3(-2*offset.x*inv_w - 1.f,  2*offset.y*inv_h+ 1.f, 1));

        glUniformMatrix3fv(Line_matrix_ID, 1, GL_FALSE, &thisMVP[0][0]);
        glUniform3f(Line_color_ID,color.x,color.y,color.z);

        glEnableVertexAttribArray(Line_VertexPosAttribID);


        glBindBuffer( GL_ARRAY_BUFFER, Buffer);
        glVertexAttribPointer
        (
            Line_VertexPosAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
            2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
            GL_FLOAT,            //Unit, this is single precition GL float
            GL_FALSE,             //Normalized? Nope
            0,                    //Stride and offset, I don't use these for anything
            (void*)0
        );

        glDrawArrays(displaymode,0,size);

        //Disable everything activated
        glDisableVertexAttribArray(Line_VertexPosAttribID);
        glUseProgram(0);

        //Delete the buffer of this object
        if (Buffer != (GLuint)-1)
            glDeleteBuffers(1,&Buffer);
    }



    void activate_Lightmap()
    {

            glBindFramebuffer(GL_FRAMEBUFFER, light_Framebuffer);
            glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);//Needed for light blending (true additive)
    }

    void activate_Display()
    {

        glBindFramebuffer(GL_FRAMEBUFFER, display_Framebuffer);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Good for texture blending (front texture obscures back)
    }
}

