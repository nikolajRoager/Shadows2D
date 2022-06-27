//Only include once, you can write pragma once, or you can use
#pragma once

/*THIS FILE
The public graphics, sound and control functions
These are put in the same header, so that we only need to include one header, the definitions are separate files though
*/



#include<string>
#include<vector>
#include<cstdint>

//operating independent file-system functions
#include<filesystem>

//Vector math
#include <glm/glm.hpp>


#include <SDL2/SDL.h>

//TBD HIDE THIS
#include <GL/glew.h>

//I use these types so much that these aliases are well worth it
using tex_index = uint16_t;
using uint = uint32_t;
using ulong = uint64_t;

using namespace std;

namespace fs = std::filesystem;


//To capture taps and arrowkeys we need more than 8 bit, and it feels wrong to refer to it as an int
using Char = int;

namespace IO
{
    //Watch me struggle to put these functions in any sort of logical order:  ---- indicate a new chapter ----  ==== indicates a larger chapter =====

    //==== Global functions ====

    // ---- One time functions, which change some properties ----
    //This calls all the associated init functions IN THE RIGHT ORDER, which is important
    void init(bool fullscreen,string window_header, fs::path tex, fs::path audio, fs::path scripture, fs::path shaders,fs::path keymaps, bool devmode=false, uint _w=320, uint _h=180);
    void end();

    // ---- Functions which must be called once per loop ----

    void post_loop();//Actually display all the things we made ready to display, and update events. This ALSO Updates window, polls input (keyboard mouse) and resets the image for the next frame, must be called each loop

    void print_ram();//Print all currently loaded textures, textboxes and sound effects alongside the number of users
    //==== Graphics =====
    namespace graphics
    {

        // ---- Texture memory management ----
        // Functions for loading, deleting and displaying different types of textures and sounds
        //The path is NOT ABSOLUTE path, it is texture_path/path
        tex_index load_tex(fs::path path, uint& w, uint& h, bool absolute_path=false);//Load a texture and gets its location in the texture list. Returns -1 = 65535 if it did not work, w and h will be set to the width and height (as they are references they can be read later, essentially a way of getting more than one output). If Absolute path is false, we assume the path is inside the texture directory, if not, it may be anywhere
        tex_index load_tex(fs::path path, bool absolute_path=false);//Same function, but do not return width and height
        void add_tex_user(tex_index ID);//Add another user to this texture, to avoid deleting textures in use
        void delete_tex(tex_index tex);//Wipe this texture from memory (if no-one else uses it)
        void set_animation(tex_index tex, uint n_frames_w, uint n_frames_h = 1);//Set this texture as actually an animation, the sprite sheet is n_frames_w wide and n_frames_h high


        //--Functions for loading text --
        //This is a part of the graphics, and could not be separated into a separate calligraphy namespace, because textboxes simply are textures which happen to contain text, we need access to the same internal functions to display them, and they are generated through OpenGL rendering, thus we can not separate it
        tex_index set_text(const string& text, bool force_unique=false);//Force unique forces this texture to be considered unique, i.e. another identical text won't be given a reference to this thing, this should only be set for text we plan to edit
        //Note that the text ID are not the same as texture ID's (even though they are the same class behind the scene), i.e. we can have texture 1 and text 1 be different. This is because text is typically changed much more often than textures
        void add_text_user(tex_index ID);//Same procedure as with textures
        void delete_text(tex_index ID);
        void overwrite_text(tex_index ID, const string& text);//Change this text with this ID


        //---Print ram ----
        void print_ram();//Print currently loaded text boxes to the external terminal (usually doesn't fit in the developer commandline)

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
        void draw_tex(int x, int y, tex_index tex, bool mirror = false, bool centered = true, bool inv_y = true);
        void animate_sprite(int x, int y, tex_index tex, uint frame, bool mirror = false, bool centered = true, bool inv_y = true);
        void draw_text(int x, int y, tex_index text, bool mirror, bool centered, bool inv_y);

        //=== Get functions, yeah get_windows should not be there, it breaks encapsulation, sorry about that
        SDL_Window * get_window();
        uint get_w();
        uint get_h();


        //--Mesh rendering functions--



        void draw_lines(GLuint buffer, ushort size,glm::vec3 color);
        void draw_triangles(GLuint buffer, ushort size,glm::vec3 color);
        void draw_triangles(GLuint buffer, ushort size,glm::vec3 color,glm::vec2 origin);
        void draw_segments(GLuint buffer, ushort size,glm::vec3 color);


        void activate_Ray();//Prepare to render to the ray texture
        void render_Ray();//Render to the ray texture

    }

    namespace audio
    {


        // --- Sound loading ---
        tex_index load_sound(string name);//Load this sound effect, and return its ID
        void delete_sound(tex_index ID);//Same procedures as with textures
        void add_sound_user(tex_index ID);

        // ---- Play sound functions ----
        void play_sound(tex_index ID);//"Display" this sound effect
        void loop_sound(tex_index ID);//Keep looping this sound as long as this is called every frame, for practical reasons we can keep at most 16 sounds looping at once

        //---Print ram ----
        void print_ram();//Print currently loaded sound effects to the external terminal (usually doesn't fit in the developer commandline)

    }




    //This includes: mouse, keyboard and the text-input from the developer commandline (While these things sound separate, SDL treats them the same way, so separating them would not be practical)
    namespace input_devices
    {


        // ---- input functions ----
        ulong get_millis();//Get current milli seconds since the graphical display started (Measured at poll events)

        //--- Mouse ---
        //Get position in pixels
        void get_mouse(int& x, int& y);
        bool mouse_click(bool right = false);//Was the mouse just clicked
        bool mouse_press(bool right = false);//Is the mouse being held down
        void mouse_lock(bool Lock=true);

        //Click is only true for one loop (useful for menu buttons), mouse press is true as long as the mouse is held down. Here is an example:
        /*
        Mouse button held down? no no no yes yes yes no yes yes
        mouse_click             0  0  0  1   0   0   0  1   0
        mouse_press             0  0  0  1   1   1   0  1   1
        */
        //SDL only allows integer scroll "steps" may be positive or negative
        int get_scroll();


        //--- Keyboard keys held down ---

        //These keys are pre-defined keys we might want to look out for.

        //TBD, it seems -- wasteful, to define unique functions for each key, would it be possible to define a "key" class where this key then could map to whatever keys we need
        // Is a key currently held down?
        bool up_key();
        bool down_key();
        bool left_key();
        bool right_key();
        bool A_key();
        bool B_key();
        bool tap_key();
        bool ctrl_key();
        bool shift_key();
        bool space_key();
        bool esc_key();
        bool should_quit();//Has the little x at the top of the window been pressed? Technically a button so it is here

        //Was a key just clicked (true) or just released (false)
        bool up_key_click(bool click=true);
        bool down_key_click(bool click=true);
        bool left_key_click(bool click=true);
        bool right_key_click(bool click=true);
        bool ctrl_key_click(bool click=true);
        bool shift_key_click(bool click=true);
        bool space_key_click(bool click=true);
        bool A_key_click(bool click=true);
        bool tab_key_click(bool click=true);
        bool B_key_click(bool click=true);
        bool esc_key_click(bool click=true);


        // ---- Get current key-map as textures  ----
        //Get a texture containing the different bound keys, even if they have been re-mapped
        tex_index get_UP_key_tex();
        tex_index get_LEFT_key_tex();
        tex_index get_RIGHT_key_tex();
        tex_index get_DOWN_key_tex();
        tex_index get_A_key_tex();
        tex_index get_B_key_tex();


        //---- Developer commandline ----

        //For interfacing with the developer commandline
        void print_lines(const vector<string>& wall_of_text);//Mostly used to print list of commands when given :help
        void print(const string& txt);//Junior version of the above .

        //returns true if a command was just entered by the human, if so cmd is set equal to the input
        bool get_command(string& cmd);//Get the currently activated command
    }


}
