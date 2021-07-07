
#define GLM_ENABLE_EXPERIMENTAL
#define TWO_PI 6.28318531

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

//No need to invent what is already there, this most excellent library does vector and matrix mathematics really well
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
//The underlying math won't be explained, I will just use it.


using namespace glm;

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <chrono>
#include <vector>

#include <unistd.h>

#include<vector>
#include"graphicus.hpp"
#include"texwrap.hpp"

//1 diveded by 60 FPS
#define MSPF_60FPS 16.66667

#define HALF_M_TO_PX 4
#define M_TO_PX 8

//To capture taps and arrowkeys we need more than 8 bit, and it feels wrong to refer to it as an int
using Char =int;

//Alright, sound is a part of SDL, so it really makes more sense to keep in here, and it just so happens that the sound class only has so short functions that they can all be declared inline
class SFX
{
private:
    string name;//For identifying sounds, so that we don't load the same multiple times
    Mix_Chunk* sound;

public:
    SFX(string Name)
    {
        sound = nullptr;
        name=Name;
    }
    ~SFX()
    {
        unload();
    }

    SFX(SFX&& that)
    {
        name=that.name;
        sound=that.sound;
        that.sound=nullptr;
    }

    void loadWAV(const fs::path& sounds)
    {
        unload();
        fs::path SND = sounds/string(name+".wav");

        sound=Mix_LoadWAV(SND.c_str());
    }

    void unload()
    {
        if (sound!=nullptr)
            Mix_FreeChunk(sound);
        sound=nullptr;
    }

    void play()
    {
        Mix_PlayChannel( -1, sound, 0);
    }

    bool is_good() {return sound!=nullptr;}

    bool is(const string& S) {return (S.compare(name)==0);}
};



namespace graphicus
{
    //Just one window for now
    SDL_Window* window = nullptr;
    SDL_GLContext context;


    vector<SFX> sounds;

    //Some internal variables we need to remember



    //window width/height
    ushort win_h;
    ushort win_w;

    ushort fps;

    bool quit=false;
    bool rsz_scr=false;//Flag to resize screeen, we may need to do a lot of updates later down the line
    bool mv_cam=false;//Has the camera been moved, saved for same reason as above





    //The list of actual textures;
    vector<texwrap> textures;
    vector<texwrap> text_boxes;//Text boxes may be created and destroyed much more readilly than textures, so leave them on their own
    vector<bool> texts_used;//Which of the text boxes are not set to delete



    GLuint VertexArrayObject = -1;//I will create this, but I won't use it for anything actually, I just need to create it



    GLuint Line_ProgramID = -1;
    GLuint Line_VertexPosAttribID= -1;
    GLuint Line_VP_ID=-1;
    GLuint Line_COL_ID=-1;

    GLuint Ray_ProgramID = -1;
    GLuint Ray_VertexPosAttribID= -1;
    GLuint Ray_VP_ID=-1;
    GLuint Ray_COL_ID=-1;
    GLuint Ray_origin_ID=-1;
    GLuint Ray_mode_ID=-1;



//A simple textured quad
    GLuint surf_ProgramID = -1;

    GLuint surf_VertexPosAttribID= -1;
    GLuint surf_VertexUVAttribID= -1;

    GLuint surf_PositionBuffer = -1;
    GLuint surf_UVBuffer = -1;
    GLuint surf_ElementBuffer= -1;
    size_t surf_elements=0;//How many triangle-vertices are there here?


    GLuint surf_colorTex_ID = -1;//The identity of the surface in the surf program

    GLuint surf_VP_ID;//The view-projection matrix ID in the surf program

    mat4 ProjectionMatrix = mat4(1);
    mat4 ViewMatrix= mat4(1);
    //The projection and view matrices are global, of course, the model matrix is identity for all the planets and regions thereon, so in this tool MVP=VP (except for the 3D cursor)
    mat4 VP=mat4(1);
    mat4 invVP=mat4(1);//For getting mouse location

    float m_per_px=0.01;//Each pixel is 1 cm


    //Predefine these functions, which should not be visible from outside, and thus isn't included in the header file on purpose
    GLuint load_program(string name, string& log);//Load and compile openGL glsl program from shaders: assets/shaders/name/vertex.glsl assets/name/fragment.glsl
    string get_log(GLuint program_or_shader);//Get the error log of this thing which failed.

    //Load this png image to an openGL texture
    GLuint load_gl_texture(fs::path path)
    {//Loading png is harder than jpg, because there are more options in the same format, i.e. do we use 16 or 8 bit pixels, do we have alpha? Oh well, lets do it anyway

        //Load to a simple SDL texture; SDL knows what to do with any file format; it can itself figure out what format we are using
        SDL_Surface* texture_surface = IMG_Load(path.c_str());
        if(texture_surface == NULL)
        {
            throw ("Couldn't load texture "+path.string()+" to surface"+string(IMG_GetError()));
        }

        //Now let us try to figure out what on earth this image is; if this is jpg we know that the image format is 8 bit GL_RGB, but if this is PNG it is anybodies guess. 16 bit png won't work, sorry.

        GLenum image_format;//What data is used by the image file
        GLenum GL_format;//And what should our target data be

        //Does this do Alpha?
        if (texture_surface->format->BytesPerPixel == 4)
        {
            image_format = GL_RGBA;
            GL_format = GL_RGBA;//I assume that if the image has an alpha channel, it is meant to be there
        } else
        {
            image_format = GL_RGB;
            GL_format = GL_RGB;
        }//I am going to be using png and jpg, formats which do RGB not BGR so I won't even consider that


        GLuint temp=0;
        //This is our destination
        glGenTextures(1, &(temp));
        glBindTexture(GL_TEXTURE_2D,temp);
        glTexImage2D
        (
            GL_TEXTURE_2D,//What texture type is this
            0,//Mipmap-Level (We auto-generate them later)
            GL_format,//internal format for graphics card
            texture_surface->w,//Width/height in pixels
            texture_surface->h,
            0,
            image_format,       //Format of input
            GL_UNSIGNED_BYTE,//Type of data
            texture_surface->pixels //A pointer to the actual data,
        );


        glGenerateMipmap(GL_TEXTURE_2D);//ABSOLUTELY DO THIS, AS WE MAY ZOOM FAR OUT
        //Now set some more parameters, we want the texture to repeat (for interpolation purpose)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        //Get best alignment for pixel storage, it is the largest element in {1,2,4,8} which divides the pitch of the texture
        uchar align = 8;
        while (texture_surface->pitch%align) align>>=1;
        //FASTER binary notation for align/=2, (shift every bit one right).
        //For a texture with power of two width/height greater than 8, I sure hope that align=8.

        glPixelStorei(GL_UNPACK_ALIGNMENT,align);
        //Doing this may result in a very small performance boost

        //Sure, we "free" the surface (We all know the surface is dead though)
        SDL_FreeSurface( texture_surface );
        return temp;

    }







    //Default, but can be changed
    Char UP_KEY   =SDLK_UP;
    Char LEFT_KEY =SDLK_LEFT;
    Char RIGHT_KEY=SDLK_RIGHT;
    Char DOWN_KEY =SDLK_DOWN;


    TTF_Font* my_font =nullptr;
    ushort my_font_height =0;

    ushort font_size=64;
    chrono::time_point this_loop = chrono::high_resolution_clock::now();

    long millis;//Current time

    //Whether or not the selected direction keys are pressed
    bool up_press   =false;
    bool down_press =false;
    bool left_press =false;
    bool right_press=false;
    bool ctrl_press =false;
    bool shift_press =false;

    bool up_key(bool clicked)  {return up_press   == clicked;}
    bool down_key(bool clicked){return down_press == clicked;}
    bool left_key(bool clicked){return left_press == clicked;}
    bool right_key(bool clicked){return right_press== clicked;}
    bool ctrl_key(bool clicked){return ctrl_press == clicked;}
    bool shift_key(bool clicked){return shift_press == clicked;}

    //Mouse down? this turn or last? what button?, where?
    bool l_mouse_down=false;
    bool l_p_mouse_down=false;
    bool r_mouse_down=false;
    bool r_p_mouse_down=false;
    int mouse_x, mouse_y;
    int scroll = 0;

    int get_scroll()
    {
        return scroll;
    }

    bool mouse_click(bool right)
    {
        if (right)
            return r_mouse_down && !r_p_mouse_down;
        else
            return l_mouse_down && !l_p_mouse_down;
    }
    bool mouse_press(bool right)
    {
        if (right)
            return r_mouse_down;
        else
        {
            return l_mouse_down;
        }

    }

    vec2 get_mouse_pos()
    {
        //SDL2 can't get the mouse position if the mouse has not been moved at least once, there is no workaround here
        vec4 pos4D = invVP*vec4(float(mouse_x)*2.f/win_w-1.f,1.f-float(mouse_y)*2.f/win_h,0,1);
        return vec2(pos4D.x,pos4D.y);
    }
    void debug_print_mouse_pos()
    {
        cout<<std::fixed<<mouse_x<<' '<<mouse_y<<endl;
        vec4 pos4D = invVP*vec4(float(mouse_x)*2.f/win_w-1.f,1.f-float(mouse_y)*2.f/win_h,0,1);
        cout<<std::fixed<<pos4D.x<<' '<<pos4D.y<<endl;

    }
    void get_wh(vec2& V0, vec2& V1)
    {
        vec4 _V0 = invVP*vec4(-0.9f,-0.9f,0,1);
        vec4 _V1 = invVP*vec4( 0.9f, 0.9f,0,1);

        V0 = vec2(std::min(_V0.x,_V1.x),std::min(_V0.y,_V1.y));
        V1 = vec2(std::max(_V0.x,_V1.x),std::max(_V0.y,_V1.y));
    }


    fs::path texture_path;
    fs::path sound_path;
    fs::path fonts_path;
    fs::path material_path;

    void init(bool fullscreen, fs::path tex, fs::path audio, fs::path scripture, fs::path materia)
    {
        texture_path = tex;
        sound_path   = audio;
        fonts_path   = scripture;
        material_path= materia;
        //If this is a restart command, actually just stop and start again
        end();

        //Flag to tell us that we are not stopped
        quit=false;
        rsz_scr=false;

        fps=60;//Initial value for printed fps, a little optimistic


        //CHAPTER 1: HOW A LOT OF APIS WERE INITIALIZED
        //Get the error ready, just in case
        string error = "";

        if(SDL_Init(SDL_INIT_EVERYTHING)<0)
        {
            error.append("Could not initialize SDL:\n\t\t");
            error.append(SDL_GetError());
            throw error;
        }

        //Initialize openGL version
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);



        int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
        if( !( IMG_Init( imgFlags ) & imgFlags ) )
        {
            error.append(" Could not initialize SDL_image with JPEG and PNG loading:\n\t\t");
            error.append(IMG_GetError());
            throw error;
        }

        if(TTF_Init() < 0)
        {
            end();
            throw TTF_GetError();
        }

         //Initialize sound
        if(Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
        {
            end();
            throw Mix_GetError();
        }
        //This is optimized for my workflow on my screen, I open the window at the right side of my screen, and have the terminal at the left side, feel free to edit as you please. The height is exactly set to match the height of my screen not taken up by my status bar at the top

        win_w=1920;
        win_h=1000;

        //Create window and its surface
        SDL_DisplayMode DM;
        SDL_GetCurrentDisplayMode(0, &DM);
        win_w = DM.w;
        win_h = DM.h;

        uint FLAGS = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | ((fullscreen)? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);

        window = SDL_CreateWindow( "2D light and shadows", win_w, 0, win_w, win_h , FLAGS);
        if (window == nullptr)
        {
            end();
            throw SDL_GetError();
        }

        SDL_ShowCursor(SDL_DISABLE);

        context = SDL_GL_CreateContext(window);

        if(context == nullptr)
        {
            error.append("OpenGL context failed to be creted\n\t\t");
            error.append(SDL_GetError());
            throw error;
        }

        glewExperimental = GL_TRUE;
        GLenum glewError = glewInit();

        if( glewError != GLEW_OK )
        {
            error.append("Glew not initialized OK\n\t\t");
            error.append(string( (char*)glewGetErrorString(glewError)));
        }

        //I have never actually needed to use this thing explicitly, and I am sure at least 90% of applications don't either, and until OpenGL 3.2 this would be defined behind the scenes but now we need to define it to get our programs to work... I would have prefered it it would just be done by OpenGL behind the scenes still, OpenGL just seems to require a little to many lines of setup already.

        glGenVertexArrays(1, &VertexArrayObject);
        glBindVertexArray(VertexArrayObject);

        //CHAPTER 2, HOS SOME THINGS WERE ENABLED

        //Use Vsync
        if( SDL_GL_SetSwapInterval( 1 ) < 0 )
        {
            error.append("Could not set VSync\n\t\t");
            error.append(SDL_GetError());
            throw error;
        }

        //A most wonderfull background color
        glClearColor( 0.0f, 0.0f, 0.0f, 1.f );

        //Alpha blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //glEnable(GL_DEPTH_TEST);
        //glDepthFunc(GL_LESS);

        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        //LOADING AHEAD, this black screen is much nicer to look at while we do that
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);


        //CHAPTER 3, How a lot of assets were loaded

        string log;


        Line_ProgramID = load_program("line",log);

        Line_VertexPosAttribID = glGetAttribLocation(Line_ProgramID, "vertex_location_modelspace");

        Line_VP_ID = glGetUniformLocation(Line_ProgramID, "MVP");
        Line_COL_ID = glGetUniformLocation(Line_ProgramID, "color");
        cout<<"Loaded Line program"<<endl;
        cout<<log<<endl;


        Ray_ProgramID = load_program("ray",log);

        Ray_VertexPosAttribID = glGetAttribLocation(Ray_ProgramID, "vertex_location_worldspace");

        Ray_VP_ID = glGetUniformLocation(Ray_ProgramID, "VP");
        Ray_COL_ID = glGetUniformLocation(Ray_ProgramID, "color");
        Ray_origin_ID = glGetUniformLocation(Ray_ProgramID, "origin_worldspace");
        Ray_mode_ID = glGetUniformLocation(Ray_ProgramID, "mode");

        cout<<"Loaded Ray program  "<<endl;
        cout<<log<<endl;

        surf_ProgramID = load_program("surface",log);

        surf_VertexPosAttribID = glGetAttribLocation(surf_ProgramID, "vertex_location_modelspace");
        surf_VertexUVAttribID = glGetAttribLocation(surf_ProgramID, "vertex_uv");

        surf_VP_ID = glGetUniformLocation(surf_ProgramID, "MVP");
        surf_colorTex_ID = glGetUniformLocation(surf_ProgramID, "colorSampler");
        cout<<"Loaded surface program"<<endl;
        cout<<log<<endl;

        //Prepare the quad
        static const GLfloat surface_pos_data[] = {
           -1.f,-1.f,
            1.f,-1.f,
           -1.f, 1.f,
            1.f, 1.f
        };

        static const GLfloat surface_uv_data[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
        };

        static const GLuint surface_index_data[] = {0, 1 , 2, 1 ,3, 2};
        surf_elements=6;
        //Create VBOs

        glGenBuffers( 1, &surf_PositionBuffer);
        glBindBuffer( GL_ARRAY_BUFFER, surf_PositionBuffer );
        glBufferData( GL_ARRAY_BUFFER,  sizeof(surface_pos_data), surface_pos_data, GL_STATIC_DRAW );

        glGenBuffers( 1, &surf_UVBuffer);
        glBindBuffer( GL_ARRAY_BUFFER, surf_UVBuffer );
        glBufferData( GL_ARRAY_BUFFER,  sizeof(surface_uv_data), surface_uv_data, GL_STATIC_DRAW );


        glGenBuffers(1, &surf_ElementBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,surf_ElementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(surface_index_data), &(surface_index_data[0]), GL_STATIC_DRAW);



        string fpath = fonts_path/"FreeSerif.ttf";


        my_font = TTF_OpenFont(fpath.c_str(), font_size);
        if( my_font == nullptr)
        {
            throw string (TTF_GetError() );
        }
        my_font_height = TTF_FontHeight(my_font);
        //Keep these ready, just in case ... yes, I do hardcode these

        this_loop = chrono::high_resolution_clock::now();



        //Get starting width and height

        int w, h;
        SDL_GetWindowSize(window,&w,&h);
        win_h = h;
        win_w = w;

        //These things are very simple with 2D graphics
        ViewMatrix = mat4(1);
        ProjectionMatrix=ortho(-win_w*m_per_px,win_w*m_per_px,-win_h*m_per_px,win_h*m_per_px,-2.f,2.f );//Z should not do anything

        glViewport(0,0, win_w, win_h);

        VP=ProjectionMatrix * ViewMatrix;//Projection*View*Model Where model is identity, remember, matrix transform should be multiplied in reverse order to what you would expect
        invVP = inverse(VP);//EXPENSIVE operation, only call when needed


    }


    ushort load_sound(string name)
    {
        for (ushort ID = 0; ID < sounds.size(); ++ID)
            if (sounds[ID].is(name))
                return ID;
        ushort ID = sounds.size();
        sounds.push_back(name);
        sounds[ID].loadWAV(sound_path);
        if (!sounds[ID].is_good())
        {
            end();
            throw string("Could not load audio file "+name);
        }

        return ID;
    }

    void play_sound(ushort ID)//Play this sound effect
    {
        if (ID != (ushort) -1 )
            sounds[ID].play();
    }

    bool should_quit()
    {
        return quit;
    }


    void pre_loop()
    {
        millis =SDL_GetTicks();
        this_loop = chrono::high_resolution_clock::now();
        SDL_Event e;

        //Do note that I don't care about the mouse, I like keyboards better, they are more precise

        r_p_mouse_down = r_mouse_down;
        l_p_mouse_down = l_mouse_down;

        scroll=0;



        ctrl_press=( SDL_GetModState() & KMOD_CTRL);
        shift_press=( SDL_GetModState() & KMOD_SHIFT);

        //Look at every single event
        while(SDL_PollEvent(&e)!=0)
        {
            //Match against those we actually care about

            //Death to the application!
            switch(e.type)
            {
                case SDL_QUIT:
                    quit=true;
                break;

                //Keydown is only the act of pressing down this thing, releasing it is another act
                case  SDL_KEYDOWN:
                    {
                        if( e.key.keysym.sym == SDLK_ESCAPE)
                        {
                                quit=true;
                        }
                        else
                        {
                        //Switch don't work with non-const statements for some reason, so use if-else instead... it is likely going to be interpreted the same way by the compiler anyway
                        if( e.key.keysym.sym == UP_KEY)
                        {
                            up_press=true;
                        }
                        else if( e.key.keysym.sym == DOWN_KEY)
                        {
                                down_press=true;
                        }
                        else if( e.key.keysym.sym == LEFT_KEY)
                        {
                                left_press=true;
                        }
                        else if( e.key.keysym.sym == RIGHT_KEY)
                        {
                                right_press=true;
                        }
                        }
                    }


                break;

                case SDL_MOUSEBUTTONDOWN:
                    if(e.button.button == SDL_BUTTON_LEFT)
                        l_mouse_down = true;
                    else if (e.button.button==SDL_BUTTON_RIGHT)
                        r_mouse_down = true;
                    //Also make sure mouse position is current
                    SDL_GetMouseState( &mouse_x, &mouse_y );

                break;

                case SDL_MOUSEBUTTONUP:
                    if(e.button.button==SDL_BUTTON_LEFT)
                        l_mouse_down = false;
                    else if (e.button.button==SDL_BUTTON_RIGHT)
                        r_mouse_down = false;
                    else if (e.button.button==SDL_BUTTON_MIDDLE)
                    {
                        //Nothing for now
                    }

                    //Also make sure mouse position is current
                    SDL_GetMouseState( &mouse_x, &mouse_y );
                break;

                case SDL_MOUSEMOTION:
                    //Get mouse position
                    SDL_GetMouseState( &mouse_x, &mouse_y );

                break;


                case  SDL_KEYUP:
                    //No controls if in terminal
                    {
                        if( e.key.keysym.sym == UP_KEY)
                        {
                            up_press=false;
                        }
                        else if( e.key.keysym.sym == DOWN_KEY)
                        {
                                down_press=false;
                        }
                        else if( e.key.keysym.sym == LEFT_KEY)
                        {
                                left_press=false;
                        }
                        else if( e.key.keysym.sym == RIGHT_KEY)
                        {
                                right_press=false;
                        }
                        else if( e.key.keysym.sym == SDLK_ESCAPE)
                        {
                            quit=true;
                        }

                    }
                break;

                case SDL_MOUSEWHEEL://How scrolled is it
                    if(e.wheel.y != 0) // scroll up or down
                    {
                         scroll=e.wheel.y;
                    }
                break;

            }
        }


        //Test if width/height of window has changed
        int nw=0;
        int nh=0;
        SDL_GetWindowSize(window,&nw,&nh);

        if (nh!=win_h || nw != win_w)
        {
            win_w=nw;
            win_h=nh;
            rsz_scr=true;

            //Redo projection matrix if we resized screen
            //ProjectionMatrix=ortho(-win_w*m_per_px,-win_w*m_per_px,win_h*m_per_px,win_h*m_per_px,-2.f,2.f );//Z should not do anything

            glViewport(0,0, win_w, win_h);
        }
        else
        {
            rsz_scr=false;
        }

        if (rsz_scr || mv_cam)
        {
            VP=ProjectionMatrix*ViewMatrix;
            mv_cam=false;//Unset the flag (When making this, I may have forgotten this line, resulting in a crazy waste of computing power by inversing the same matrix each frame)
            invVP = inverse(VP);//EXPENSIVE operation, only call when needed
        }

        glClear(GL_COLOR_BUFFER_BIT);

    }


    void flush()
    {
        //Flush the things we want to see
        SDL_GL_SwapWindow(window);

        //Get how long more we should wait
        chrono::duration<double> time_span = chrono::high_resolution_clock::now()  - this_loop;


        fps =1/time_span.count()+0.5;//The last addition rounds to nearest rather than lowest

    }

    //There is no penalty whatsoever in using integer number of milliseconds, noone can tell anyway
    ulong get_millis()
    {
        if (window!=nullptr)
            return millis;
        else
            return 0;

    }

    void end()
    {

        SDL_ShowCursor(SDL_ENABLE);

        //Destroy everything

        //Only call the disallocation if we actually need to do so
        //This is likely very excessive, these things are likely disallocated anyway when the program quits.
        //The disallocate might often even check if the disallocate command is valid in the first place...
        //I just like to do this manually, it feels more clean
        if (window != nullptr)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        if (VertexArrayObject!=(GLuint)-1)
        {
            glDeleteVertexArrays(1, &VertexArrayObject);
            VertexArrayObject=-1;
        }

        if (surf_UVBuffer!=(GLuint)-1)
        {
            glDeleteBuffers(1,  &surf_UVBuffer);
            surf_UVBuffer=-1;
        }
        if (surf_PositionBuffer!=(GLuint)-1)
        {
            glDeleteBuffers(1,  &surf_PositionBuffer);
            surf_PositionBuffer=-1;
        }
        if (surf_ElementBuffer!=(GLuint)-1)
        {
            glDeleteBuffers(1, &surf_ElementBuffer);
            surf_ElementBuffer=-1;
        }

        if (surf_ProgramID!=(GLuint) -1)
        {
            glDeleteProgram(surf_ProgramID);
            surf_ProgramID=-1;
        }

        if (Ray_ProgramID !=(GLuint) -1)
        {
            glDeleteProgram(Ray_ProgramID);
            Ray_ProgramID=-1;
        }



        if (Line_ProgramID!=(GLuint) -1)
        {
            glDeleteProgram(Line_ProgramID);
            Line_ProgramID=-1;
        }

        if (context!=nullptr)
        {
            SDL_GL_DeleteContext(context);
            context=nullptr;
        }



        //Automatically calls destructor
        sounds = vector<SFX>();

        textures = vector<texwrap>();
        text_boxes = vector<texwrap>();
        texts_used = vector<bool>();

        //Quit all of SDL
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
    }

    GLuint load_program(string name, string& log)
    {
        //This is extremely basic code for loading and compiling GLSL shaders, not many comments are needed

        //First things first, can we even load the code?

        string vertexShader_source;
        string fragmentShader_source;

        fs::path VSP = material_path/name/"vertex.glsl";
        fs::path FSP = material_path/name/"fragment.glsl";

        //Load vertex shader
        ifstream vertexShader_stream(VSP);

        if(!vertexShader_stream.is_open())
        {
            throw ("File not found: "+VSP.string());
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

        if(!fragmentShader_stream.is_open())
        {
            throw ("File not found: "+FSP.string() );
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
        const GLchar * vertexShader_source_cstr = vertexShader_source.c_str();
        glShaderSource(vertexShader, 1, &vertexShader_source_cstr, nullptr);
        glCompileShader(vertexShader);

        //Did it work
        GLint vertexShader_good = GL_FALSE;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexShader_good);

        if( vertexShader_good != GL_TRUE )
        {
            string Error = get_log(vertexShader);
            glDeleteShader(vertexShader);
            throw ("Couldn't compile vertex shader:\n"+Error);
        }
        //Same for fragment shader:

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar * fragmentShader_source_cstr = fragmentShader_source.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShader_source_cstr, nullptr);
        glCompileShader(fragmentShader);

        //Did it work
        GLint fragmentShader_good = GL_FALSE;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentShader_good);

        if( fragmentShader_good != GL_TRUE )
        {
            string Error = get_log(fragmentShader);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            throw ("Couldn't compile fragment shader, error:\n"+Error);
        }


        //Do the actual program, not much to say here
        GLuint out = glCreateProgram();
        glAttachShader(out,vertexShader);
        glAttachShader(out,fragmentShader);
        glLinkProgram(out);

        //Check for errors
        GLint program_good = GL_TRUE;
        glGetProgramiv(out, GL_LINK_STATUS, &program_good);

        if( program_good != GL_TRUE )
        {
            string Error = get_log(out);
            glDetachShader(out, vertexShader);
            glDetachShader(out, fragmentShader);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            throw ("Error linking program:\n"+Error);
        }

        string Vlog = get_log(vertexShader);
        string Flog = get_log(vertexShader);
        string Plog = get_log(vertexShader);
        log ="";
        if (Vlog.size()!=0)
        {
            log = log+"---Vertex shader log---\n"+Vlog+"\n";
        }
        if (Flog.size()!=0)
        {
            log = log+"--Fragment shader log--\n"+Flog+"\n";
        }
        if (Plog.size()!=0)
        {
            log = log+"---Linker shader log---\n"+Plog+"\n";
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


        if(glIsProgram(program_or_shader))
        {
            //get length (actually max length)
            glGetProgramiv(program_or_shader,GL_INFO_LOG_LENGTH,&max_length);

            //Make room for the log, then copy it where we want it
            char* log_c_str = new char[max_length];
            glGetProgramInfoLog(program_or_shader, max_length, &log_length, log_c_str);
            if(max_length>0)
            {
                out+=log_c_str;//Convert to c++ string
            }

            //Throw away the c string version
            delete[] log_c_str;
        }
        //The exact same thing, almost
        else if(glIsShader(program_or_shader))
        {
            //get length (actually max length)
            glGetShaderiv(program_or_shader,GL_INFO_LOG_LENGTH,&max_length);

            //Make room for the log, then copy it where we want it
            char* log_c_str = new char[max_length];
            glGetShaderInfoLog(program_or_shader, max_length, &log_length, log_c_str);
            if(max_length>0)
            {
                out+=log_c_str;//Convert to c++ string
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

    //Draw with a falloff, useful for lighting effects
    void draw_ray(GLuint buffer, ushort size, vec3 color,GLuint displaymode,vec2 origin, int falloff_mode)
    {
        //Enable what we need
        glUseProgram(Ray_ProgramID );



        glUniformMatrix4fv(Ray_VP_ID, 1, GL_FALSE, &VP[0][0]);
        glUniform3f(Ray_COL_ID,color.x,color.y,color.z);
        glUniform2f(Ray_origin_ID,origin.x,origin.y);
        glUniform1i(Ray_mode_ID,falloff_mode);

        glEnableVertexAttribArray(Ray_VertexPosAttribID);


        glBindBuffer( GL_ARRAY_BUFFER, buffer);
        glVertexAttribPointer
        (
            Ray_VertexPosAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
            2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
            GL_FLOAT,            //Unit, this is single precition GL float
            GL_FALSE,             //Normalized? Nope
            0,                    //Stride and offset, I don't use these for anything
            (void*)0
        );

        glDrawArrays(displaymode,0,size);

        //Disable everything activated
        glDisableVertexAttribArray(Ray_VertexPosAttribID);
        glUseProgram(0);

    }

    void draw_unicolor(GLuint buffer, ushort size, vec3 color,GLuint displaymode)
    {
        //Enable what we need
        glUseProgram(Line_ProgramID);

        glUniformMatrix4fv(Line_VP_ID, 1, GL_FALSE, &VP[0][0]);
        glUniform3f(Line_COL_ID,color.x,color.y,color.z);

        glEnableVertexAttribArray(Line_VertexPosAttribID);


        glBindBuffer( GL_ARRAY_BUFFER, buffer);
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

    }

    void draw_lines(GLuint buffer, ushort size, vec3 color)
    {
        draw_unicolor(buffer,size,color,GL_LINE_STRIP);
    }

    void draw_triangles(GLuint buffer, ushort size, vec3 color)
    {
        draw_unicolor(buffer,size,color,GL_TRIANGLE_FAN);
    }
    void draw_segments(GLuint buffer, ushort size, vec3 color)
    {
        draw_unicolor(buffer,size,color,GL_LINES);
    }
    void draw_triangles(GLuint buffer, ushort size, vec3 color,vec2 origin)
    {
        draw_ray(buffer,size,color,GL_TRIANGLE_FAN,origin,1);
    }



    //Try to see if this texture already exists, if not, load it
    ushort load_tex(fs::path path, ushort& w, ushort& h)
    {
        ushort t_s=textures.size();
        for (ushort t = 0; t < t_s; ++t)
        {
            texwrap& T = textures[t];
            if (T.is(path))
            {
                textures[t].reload();//This un-sets all animation settings.
                w = T.get_w();
                h = T.get_h();
                return t;
            }
        }
        //no catch here ... errors may be thrown, but they should be handled by the idiot who asked for a non-existing texture
        textures.push_back(texwrap(path,m_per_px));
        const texwrap& T = textures[t_s];
        w = T.get_w();
        h = T.get_h();

        return t_s;
    }

    ushort load_tex(fs::path path)
    {
        ushort W = 0;
        ushort H = 0;
        return load_tex(path,W,H);
    }

    //Create a texture for text strings
    ushort set_text(const string text)
    {
        text_boxes.push_back(texwrap(text,my_font,m_per_px));
        texts_used.push_back(true);//Now this is used

        return texts_used.size()-1;
    }
    void delete_text(ushort ID)
    {
        ushort T_s = texts_used.size();
        if (ID<T_s)
        {
            texts_used[ID]=false;//At least flag it as not in use

            //Now, delete anything not in use
            for (ushort I = T_s-1; I<T_s; --I)
            {
                if (texts_used[I])
                {
                    break;
                }
                else
                {
                    texts_used.pop_back();
                    text_boxes.pop_back();
                }
            }

        }
    }
    void overwrite_text(ushort ID,const string& text)
    {
        ushort T_s = texts_used.size();

        if (ID<T_s)
        {
            texts_used[ID]=true;
            text_boxes[ID]=texwrap(text,my_font ,m_per_px);
        }
    }

    void draw_tex(ushort tex, vec2 pos)
    {
        //Enable what we need
        glUseProgram(surf_ProgramID);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[tex].get_tex());
        glUniform1i(surf_colorTex_ID, 0);

        mat4 thisMVP = VP*translate(vec3(pos.x,pos.y,0))*textures[tex].getM();

        glUniformMatrix4fv(surf_VP_ID, 1, GL_FALSE, &thisMVP[0][0]);

        glEnableVertexAttribArray(surf_VertexPosAttribID);
        glEnableVertexAttribArray(surf_VertexUVAttribID);

        glBindBuffer( GL_ARRAY_BUFFER, surf_UVBuffer);
        glVertexAttribPointer
        (
            surf_VertexUVAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
            2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
            GL_FLOAT,            //Unit, this is single precition GL float
            GL_FALSE,             //Normalized? Nope
            0,                    //Stride and offset, I don't use these for anything
            (void*)0
        );

        glBindBuffer( GL_ARRAY_BUFFER, surf_PositionBuffer);
        glVertexAttribPointer
        (
            surf_VertexPosAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
            2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
            GL_FLOAT,            //Unit, this is single precition GL float
            GL_FALSE,             //Normalized? Nope
            0,                    //Stride and offset, I don't use these for anything
            (void*)0
        );

        //Use our most excellent element buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surf_ElementBuffer);
        //The actual drawing part of this
        glDrawElements( GL_TRIANGLES, surf_elements , GL_UNSIGNED_INT, NULL );

        //Disable everything activated
        glDisableVertexAttribArray(surf_VertexPosAttribID);
        glDisableVertexAttribArray(surf_VertexUVAttribID);
        glUseProgram(0);
    }

    //Same for text
    void draw_text(ushort tex, vec2 pos)
    {
        //Enable what we need
        glUseProgram(surf_ProgramID);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, text_boxes[tex].get_tex());
        glUniform1i(surf_colorTex_ID, 0);

        mat4 thisMVP = VP*translate(vec3(pos.x,pos.y,0))*text_boxes[tex].getM();

        glUniformMatrix4fv(surf_VP_ID, 1, GL_FALSE, &thisMVP[0][0]);

        glEnableVertexAttribArray(surf_VertexPosAttribID);
        glEnableVertexAttribArray(surf_VertexUVAttribID);

        glBindBuffer( GL_ARRAY_BUFFER, surf_UVBuffer);
        glVertexAttribPointer
        (
            surf_VertexUVAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
            2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
            GL_FLOAT,            //Unit, this is single precition GL float
            GL_FALSE,             //Normalized? Nope
            0,                    //Stride and offset, I don't use these for anything
            (void*)0
        );

        glBindBuffer( GL_ARRAY_BUFFER, surf_PositionBuffer);
        glVertexAttribPointer
        (
            surf_VertexPosAttribID,//Attribute location, you can either locate this in the program, or you can force the shader to use a particular location from inside the shader, I do the former
            2,                   //Number Number of the below unit per element (this is a 2D vector, so 2)
            GL_FLOAT,            //Unit, this is single precition GL float
            GL_FALSE,             //Normalized? Nope
            0,                    //Stride and offset, I don't use these for anything
            (void*)0
        );

        //Use our most excellent element buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surf_ElementBuffer);
        //The actual drawing part of this
        glDrawElements( GL_TRIANGLES, surf_elements , GL_UNSIGNED_INT, NULL );

        //Disable everything activated
        glDisableVertexAttribArray(surf_VertexPosAttribID);
        glDisableVertexAttribArray(surf_VertexUVAttribID);
        glUseProgram(0);
    }
}

