/*THIS FILE
Definition for IO:input_devices functions
//This includes: mouse, keyboard and the text-input from the developer commandline (While these things sound separate, SDL treats them the same way, so separating them would not be practical)
*/


//Main sdl
#include <SDL2/SDL.h>


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

#include "IO.hpp"

//I use these types so much that these aliases are well worth it
using uchar= uint8_t;
using uint = uint32_t;
using ulong = uint64_t;

using namespace std;

namespace fs = std::filesystem;



//The SDL keyboard codes include things like up, down left right, etc. there are too many of these to fit into the normal 8-bit char, so SDL uses int, conveniently all regular letters (in the latin alphabet, not extended unicode) have the same number as the numerical value of them as chars. I like to rename this to Char so that I remember this is a keyboard character
using Char = int;

//We want to be able to see this normally hidden graphics function
namespace IO::graphics
{
    tex_index get_key_tex(Char Key);
    uint get_offset_x();
    uint get_offset_y();
    uint get_win_h();
    uint get_win_w();
    uint get_h();
    uint get_w();
    uchar get_text_v_space();
    uchar get_text_h();
    float get_inv_s_fac();
}



namespace IO::input_devices
{
    //--Internal variables and flags--
    bool quit=true;
    bool devmode=false;//If devmode is on we allow the developer command-line to be used, if not we ignore it

    //milli seconds since start of SDL
    ulong millis=0;
    ulong command_timer=0;//If the developer commandline is active, it will linger on the screen until millis>command_timer
    ulong deletion_timer=0;//A cooldown before deletions of text

    //Mouse down? l/r = left or right,
    bool l_mouse_down = false;
    bool r_mouse_down = false;
    bool l_p_mouse_down = false;//p indicates previous loop, this is because in some cases we don't want to spam certain actions just because the key is held down.
    bool r_p_mouse_down = false;
    int mouse_x, mouse_y;//mouse location, must be signed integers as this is what SDL sends them out as
    int scroll = 0;
    bool Lock_mouse=false;//Lock mouse to the center of the screen, and return its motion instead of position


    //Whether or not the selected keys are pressed
    bool up_press     = false;
    bool down_press   = false;
    bool left_press   = false;
    bool right_press  = false;
    bool A_press   = false;
    bool B_press  = false;
    bool tab_press = false;
    bool ctrl_press   = false;
    bool shift_press  = false;
    bool space_press  = false;
    bool esc_press  = false;

    //whether or not the keys were pressed last loop
    bool pup_press    = false;
    bool pdown_press  = false;
    bool pleft_press  = false;
    bool pright_press = false;
    bool pctrl_press  = false;
    bool pshift_press = false;
    bool pspace_press = false;
    bool pA_press   = false;
    bool pB_press  = false;
    bool ptab_press = false;
    bool pesc_press  = false;

    //Developer commandline and text handling
    //Text size and spacing
    uchar text_v_space;
    uchar text_h;

    uint h;//Height of the screen

    //Is the text input open
    bool query_text = false;
    string text_input = "";//The text being entered by the user

    //Recalculate displayed text
    bool new_text = false;

    //Do we look for a command, is set true once the commandline is opened
    bool search_command = false;
    bool new_command=false;//This flag tells us a new command is ready to be read
    string DEV_COM = ":";//This is a special key, which does not have its own keyboard key, on most layouts it is hidden behind a shift or alt + some other key. There only way to test for this is then to always enable text mode when in devmode. REMEMBER, the keyboard combination to write this is different for different keyboards (UK/US, Danish, etc.)

    vector<string> command_history;
    vector<tex_index > command_textures;//If developer terminal is activated, these are the textures of the command line

    uint command_cursor = 0;//Used to cycle through old command history

    tex_index input_text_texture;//Currently used command texture

    //Default keybindings, can (should) be changed
    Char UP_KEY = SDLK_UP;
    Char LEFT_KEY = SDLK_LEFT;
    Char RIGHT_KEY = SDLK_RIGHT;
    Char DOWN_KEY = SDLK_DOWN;
    Char TAB_KEY = SDLK_LSHIFT;
    Char ESC_KEY = SDLK_ESCAPE;
    Char SPACE_KEY =SDLK_SPACE;
    Char ENTER_KEY = SDLK_RETURN;
    Char DELETE_KEY=SDLK_BACKSPACE ;

    Char A_KEY = 'z';
    Char B_KEY = 'x';
    fs::path keymaps;//The file containing the keyboard map

    void reload_keys();

    //Texture containing the current keys, remember, the player only knows the control when we show them
    tex_index UP_KEY_tex = -1;
    tex_index LEFT_KEY_tex = -1;
    tex_index RIGHT_KEY_tex= -1;
    tex_index DOWN_KEY_tex = -1 ;
    tex_index TAB_KEY_tex = -1;
    tex_index A_KEY_tex = -1;
    tex_index B_KEY_tex = -1;



    // ---- one time functions ----
    void updateMouse();//pre declare these things
    void set_devmode(bool dev);
    void init(fs::path keymaps, bool dev)
    {
        //We are not stopped
        quit = false;


        UP_KEY_tex = IO::graphics::get_key_tex(UP_KEY);
        LEFT_KEY_tex = IO::graphics::get_key_tex(LEFT_KEY);
        RIGHT_KEY_tex= IO::graphics::get_key_tex(RIGHT_KEY);
        DOWN_KEY_tex = IO::graphics::get_key_tex(DOWN_KEY);
        TAB_KEY_tex = IO::graphics::get_key_tex(TAB_KEY);
        A_KEY_tex = IO::graphics::get_key_tex(A_KEY);
        B_KEY_tex = IO::graphics::get_key_tex(B_KEY);

        //These are unlikely to change without calling a total reload
        text_v_space = IO::graphics::get_text_v_space();
        text_h= IO::graphics::get_text_h();
        h = IO::graphics::get_h();

        //Keep these ready, just in case ... yes, I do hardcode these
        string intro = "Developer tools activated";
        command_textures.push_back(IO::graphics::set_text(intro));
        intro = "Developer tools only in English (sorry)";
        command_textures.push_back(IO::graphics::set_text(intro));
        intro = "Enter command with \":\", run with enter";
        command_textures.push_back(IO::graphics::set_text(intro));
        intro = "Type \":help\" for list of commands";
        command_textures.push_back(IO::graphics::set_text(intro));
        intro = "Close this message with Enter";
        command_textures.push_back(IO::graphics::set_text(intro));

        input_text_texture = IO::graphics::set_text(text_input);

        Lock_mouse=false;
        updateMouse();

        set_devmode(dev);

    }
    void end()
    {
        //Reset command history
        command_history =vector<string>();


        //Delete all the texts we loaded
        for (tex_index text : command_textures)
        {
            IO::graphics::delete_text(text);
        }

        IO::graphics::delete_text(input_text_texture);

        IO::graphics::delete_tex(UP_KEY_tex        );
        IO::graphics::delete_tex(LEFT_KEY_tex      );
        IO::graphics::delete_tex(RIGHT_KEY_tex     );
        IO::graphics::delete_tex(DOWN_KEY_tex      );
        IO::graphics::delete_tex(TAB_KEY_tex   );
        IO::graphics::delete_tex(A_KEY_tex         );
        IO::graphics::delete_tex(B_KEY_tex         );

        command_textures= vector<tex_index >();

    }

    void reload_keys()
    {
        ifstream keys(keymaps);

        if (!keys.is_open())
        {
            throw std::runtime_error("File "+keymaps.string()+" could not be opened");
        }

        //TBD, really, intead of having a number of pre-defined allowed keys, could we define more keys without recompiling?

        string input;
        //The keys and their SDL key code are stored in one long list, this is so int that plain-text loading is fine
        while (getline(keys,input))//Read one line at the time
        {
            //Skip comments
            if (input[0]=='#')
                continue;

            //Now stream the line content word for word
            stringstream ss(input);
            string key="";
            ss >> key;

            if (key.compare("LEFT"))
                keys>>LEFT_KEY;
            if (key.compare("RIGHT"))
                keys>>RIGHT_KEY;
            if (key.compare("DOWN"))
                keys>>DOWN_KEY;
            if (key.compare("UP"))
                keys>>UP_KEY;
            if (key.compare("TAB"))
                keys>>DOWN_KEY;
            if (key.compare("A"))
                keys>>A_KEY;
            if (key.compare("B"))
                keys>>B_KEY;
            if (key.compare("ESC"))
                keys>>ESC_KEY;
            if (key.compare("SPACE"))
                keys>>SPACE_KEY;
            if (key.compare("ENTER"))
                keys>>ENTER_KEY;
            if (key.compare("DELETE"))
                keys>>DELETE_KEY;


        }
        keys.close();
    }
    //Enable calling developer commandline
    void set_devmode(bool dev)
    {
        //We need to be able to detect ':' being pressed to open the command prompt, the only way to do this for all keyboards is by reading text input. This adds a bit of overhead, but this is not a processing intense game anyway. Besides, this will not be enabled by default
        if (!devmode && dev)
        {
            cout<<"SET DEVELOPER MODE"<<endl;
            SDL_StartTextInput();
            command_timer = millis + 20000;

        }
        else if (devmode && !dev)
        {
            cout<<"UNSET DEVELOPER MODE"<<endl;
            SDL_StopTextInput();
        }
        devmode = dev;
    }

    // ---- Functions which must be called once per loop ----
    //Polling and updating before and after ensures we can keep track of not only what is being done, but whether something has just changed
    void poll_events()
    {
        millis = SDL_GetTicks();

        //Save our old events
        pup_press    = up_press;
        pdown_press  = down_press;
        pleft_press  = left_press;
        pright_press = right_press;
        pA_press = A_press;
        pB_press = B_press;
        ptab_press = tab_press;
        pctrl_press  = ctrl_press;
        pshift_press = shift_press;
        pspace_press = space_press;
        pesc_press = esc_press ;

        r_p_mouse_down = r_mouse_down;
        l_p_mouse_down = l_mouse_down;

        scroll = 0;

        if (Lock_mouse)
        {
            mouse_x = 0;//In locked mouse mode, we return the mouse offset, not the  mouse position, we will be buffering the offset though so set this to 0 no
            mouse_y = 0;

        }

        //Some things are gotten as keyboard mod states, i.e. is the effect of shift present, this gets both caps-lock, left and right shift
        shift_press = (SDL_GetModState() & KMOD_SHIFT);
        ctrl_press = (SDL_GetModState() & KMOD_CTRL);


        //If we are looping through the developer commandline, reset the flag saying that new text was just input
        if (query_text)
        {
            new_text = false;
        }


        SDL_Event e;
        //Look at every single event, SDL queues events, so we can read more things happening at the same time
        while (SDL_PollEvent(&e) != 0)
        {
            //Match against those we actually care about

            switch (e.type)
            {
            //Death to the application!
            case SDL_QUIT:
                quit = true;
                break;

            //Keydown is only the act of pressing down this thing, releasing it is another act
            case  SDL_KEYDOWN:
                if (!devmode || !query_text)//Turn of controls while in the terminal
                {
                    //Switch don't work with non-const statements for some reason, so use if-else instead... it is likely going to be interpreted the same way by the compiler anyway
                    if (e.key.keysym.sym == UP_KEY)
                    {
                        up_press = true;
                    }//once again, we can detect multiple keys at the same time, they just give two events in the while loop, each event can only be one thing
                    else if (e.key.keysym.sym == DOWN_KEY)
                    {
                        down_press = true;
                    }
                    else if (e.key.keysym.sym == LEFT_KEY)
                    {
                        left_press = true;
                    }
                    else if (e.key.keysym.sym == RIGHT_KEY)
                    {
                        right_press = true;
                    }
                    else if (e.key.keysym.sym == A_KEY)
                    {
                        A_press = true;
                    }
                    else if (e.key.keysym.sym == B_KEY)
                    {
                        B_press = true;
                    }
                    else if (e.key.keysym.sym == TAB_KEY)
                    {
                        tab_press = true;
                    }
                    else if (e.key.keysym.sym == ESC_KEY)
                    {
                        esc_press = true;
                    }
                    else if (e.key.keysym.sym == SPACE_KEY)
                    {
                        space_press = true;
                    }
                    if (e.key.keysym.sym == ENTER_KEY)
                    {
                        //Remove commandline
                         command_timer = millis;
                    }
                }
                else
                {
                    if (e.key.keysym.sym == ENTER_KEY)
                    {
                        query_text = false;
                        new_text = true;
                    }
                    else if (e.key.keysym.sym == ESC_KEY)
                    {
                        query_text = false;
                        text_input = "";
                        esc_press = true;

                    }//Delete a character from the current text input, this is a little ugly
                    else if (e.key.keysym.sym == DELETE_KEY && text_input.length() > 0)
                    {
                        if (deletion_timer < millis)
                        {
                            command_cursor = 0;

                            deletion_timer = millis + 50;//Use this to get a min time between deletions


                            //Alright, we have no clue how many chars to remove, because non-English languages exist!!! and their special characters can't all fit in your normal 8 bit bytes. Thankfully UTF8 uses special flags to show which bytes are part of a longer character
                            tex_index ts = text_input.size();//start from the back and loop backwards until we have deleted a single character
                            for (tex_index i = 0; i < ts; ++i)
                            {
                                char c = text_input[ts - i - 1];

                                //We are going to keep cutting the spare bytes, regardless of whether this byte was the first or not in the last letter, it has to go
                                text_input.pop_back();

                                if ((c & 0xc0) != 0x80)//If the first two bits are 10, then this is the first byte in a charcater and we should stop deleting more now
                                    break;
                            }

                            new_text = true;

                        }
                    }


                }


                break;

            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT)
                    l_mouse_down = true;
                else if (e.button.button == SDL_BUTTON_RIGHT)
                    r_mouse_down = true;
                //Also make sure mouse position is current
                updateMouse();

                break;

            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT)
                    l_mouse_down = false;
                else if (e.button.button == SDL_BUTTON_RIGHT)
                    r_mouse_down = false;
                else if (e.button.button == SDL_BUTTON_MIDDLE)
                {//A common Linux intcut which here is made availible for all systems, because it is just so much more practical
                    if (query_text)
                    {
                        char* TEMP = nullptr;
                        TEMP = SDL_GetClipboardText();
                        if (TEMP != nullptr)
                            text_input.append(TEMP);
                        new_text = true;
                    }
                }

                //Also make sure mouse position is current
                updateMouse();
                break;

            case SDL_MOUSEMOTION:
                //Get mouse position
                updateMouse();

                break;

            //key just released
            case  SDL_KEYUP:
                //No controls if in terminal
                if (!devmode || !query_text)
                {
                    if (e.key.keysym.sym == UP_KEY)
                    {
                        up_press = false;
                    }
                    else if (e.key.keysym.sym == DOWN_KEY)
                    {
                        down_press = false;
                    }
                    else if (e.key.keysym.sym == LEFT_KEY)
                    {
                        left_press = false;
                    }
                    else if (e.key.keysym.sym == RIGHT_KEY)
                    {
                        right_press = false;
                    }
                    else if (e.key.keysym.sym == A_KEY)
                    {
                        A_press = false;
                    }
                    else if (e.key.keysym.sym == B_KEY)
                    {
                        B_press = false;
                    }
                    else if (e.key.keysym.sym == TAB_KEY)
                    {
                        tab_press = false;
                    }
                    else if (e.key.keysym.sym == ESC_KEY)
                    {
                        esc_press = false;
                    }
                    else if (e.key.keysym.sym == SPACE_KEY)
                    {
                        space_press = false;
                    }

                }
                else//We are having commandline active
                {
                    //Cycle through command history
                    if (e.key.keysym.sym == UP_KEY && search_command)
                    {
                        if (command_cursor != 0)
                        {
                            --command_cursor;
                        }
                        //Intentionally overwrite, even if not moving further back
                        if (command_history.size() != 0)
                            text_input = command_history[command_cursor];
                        new_text = true;
                    }
                    else if (e.key.keysym.sym == DOWN_KEY && search_command)
                    {

                        if (command_cursor < command_history.size() - 1 && command_history.size() != 0)
                        {
                            ++command_cursor;
                            text_input = command_history[command_cursor];
                        }
                        else
                        {
                            if (command_cursor == command_history.size() - 1)
                                ++command_cursor;
                            text_input = "";
                        }
                        new_text = true;
                    }
                    else if (e.key.keysym.sym == ESC_KEY)
                    {
                        text_input = "";
                        query_text = false;
                        esc_press = false;
                    }//Ok these multi-key combinations are not customizable
                    else if (e.key.keysym.sym == SDLK_c && (SDL_GetModState() & KMOD_CTRL))//ctrl+c: copy to system clipboard, this allows copy pasting out of the program
                    {
                        SDL_SetClipboardText(text_input.c_str());
                        new_text = true;
                    }
                    else if (e.key.keysym.sym == SDLK_v && (SDL_GetModState() & KMOD_CTRL))//ctrl+v: paste from the system clipboard
                    {
                        command_cursor = 0;
                        char* TEMP = nullptr;
                        TEMP = SDL_GetClipboardText();
                        if (TEMP != nullptr)
                            text_input.append(TEMP);
                        new_text = true;
                    }
                }
                break;

            case SDL_MOUSEWHEEL://How scrolled is it
                if (e.wheel.y != 0) // scroll up or down
                {
                    scroll = e.wheel.y;
                }
                break;

            case SDL_TEXTINPUT:
                if (query_text)//If we are currently typing text, append to the text
                {
                    //except if ctrl is held down (ctrl+c/v is handled elsewhere)
                    if (!(SDL_GetModState() & KMOD_CTRL))
                    {
                        command_cursor = 0;
                        text_input += e.text.text;
                        new_text = true;
                    }
                }
                else if (devmode)//Not currently typing, but may open console
                {//Look for command keys (they may be modified keys, which can not be detected with standard keydowns
                    if (DEV_COM.compare(string(e.text.text)) == 0)//Is this the command to open the commandline (":" by default)
                    {
                        query_text = true;
                        search_command = true;
                        text_input = "";
                        command_cursor = command_history.size();
                        new_text = true;

                        if (command_textures.size() == 0)
                        {
                            for (string& S : command_history)
                                command_textures.push_back(IO::graphics::set_text(S));
                        }

                    }

                }
                break;
            }//end switch (e.type)
        }//end while (SDL_PollEvent(&e) != 0)


        //If command was just activated, search command is active when the commandline is, and query text is set to false when enter is pressed
        if (search_command && !query_text)
        {
            command_timer = millis + 2000;
            search_command = false;
            new_command = true;
            command_history.push_back(text_input);
            command_textures.push_back(IO::graphics::set_text(text_input));
            cout<<"Command : "<<text_input<<endl;
        }
        //If the commandline is active and some new text has been entered, overwrite the current textbox
        if (search_command && new_text)
        {
            string temp_text_input = ":" + text_input;
            IO::graphics::overwrite_text(input_text_texture, temp_text_input);

        }


    }

    void mouse_lock(bool Lock)
    {
        Lock_mouse=Lock;
        //Center the mouse
        SDL_WarpMouseInWindow(IO::graphics::get_window(), IO::graphics::get_win_w()/(2), IO::graphics::get_win_h()/2);
        mouse_x =0;
        mouse_y =0;
    }


    void updateMouse()
    {
        int w_mouse_x,w_mouse_y;
        //Get mouse position, in a 2D world that is fairly easy
        SDL_GetMouseState(&w_mouse_x, &w_mouse_y);
        //These are pixels on the window, but 0,0 on the screen is not 0,0 internally, and the pixels are way smaller

        //Get some display info using our the secret functions known only to this namespace
        uint offset_x = IO::graphics::get_offset_x();
        uint offset_y = IO::graphics::get_offset_y();
        double inv_s_fac    = IO::graphics::get_inv_s_fac();
        uint win_h    = IO::graphics::get_win_h();
        //Counter scale and offset the mouse coordinates, which are from the top left corner of the window
        int this_mouse_x = (w_mouse_x - offset_x) * inv_s_fac;
        int this_mouse_y = (win_h - (w_mouse_y + offset_y)) * inv_s_fac;

        //Just read wherever the mouse is now
        if (!Lock_mouse)
        {
            mouse_x = this_mouse_x ;
            mouse_y = this_mouse_y ;
        }
        else
        {
            uint win_w    = IO::graphics::get_win_w();
            this_mouse_x -=IO::graphics::get_w()/2;
            this_mouse_y -=IO::graphics::get_h()/2;

            //Upload the mouse movement, note that we buffer positions, as update mouse may be called multiple times each frame, it will be reset on the next general update if need be)
            mouse_x += this_mouse_x;
            mouse_y += this_mouse_y;

            if (this_mouse_x!= 0 || this_mouse_y!= 0)
            {
                SDL_WarpMouseInWindow(IO::graphics::get_window(), win_w/(2), win_h/2);
            }
        }
    }

    void print_commandline()//If developer commandline is turned off, this does nothing.
    {
        uint com_size = command_textures.size();
        if (search_command)
        {
            IO::graphics::draw_text(text_h, text_h+text_v_space, input_text_texture, false, false, true);

        }
        //command terminal lingers after commandline is closed
        if (command_timer > millis || search_command)
        {

            for (uint i = com_size - 1; i < com_size; --i)//this is unsigned integers, i intend to overflow when below 0
            {
                int y = text_h * ((com_size - i) + 1)+text_v_space;

                if (y >= h)//Don't go outside the screen
                    break;

                IO::graphics::draw_text(text_h, y, command_textures[i], false, false, true);
            }
        }
    }

    // ---- input functions ----
    ulong get_millis()//Get current milli seconds since the graphical display started (Measured at poll events)
    {
        return millis;
    }

    //--- Mouse ---
    //Get position in pixels
    void get_mouse(int& x, int& y)
    {
        x=mouse_x;
        y=mouse_y;
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
            return l_mouse_down;

    }
    //Click is only true for one loop (useful for menu buttons), mouse press is true as long as the mouse is held down. Here is an example:
    /*
    Mouse button held down? no no no yes yes yes no yes yes
    mouse_click             0  0  0  1   0   0   0  1   0
    mouse_press             0  0  0  1   1   1   0  1   1
    */
    //SDL only allows integer scroll "steps" may be positive or negative
    int get_scroll()
    {
        return scroll;//I hope this is the scroll you wanted
    }


    //--- Keyboard keys held down ---

    //These keys are pre-defined keys we might want to look out for.
    //TBD, as the functions are defined, remapping is possible, but this should be possible without re-compiling

    //TBD, it seems -- wasteful, to define unique functions for each key, would it be possible to define a "key" class where this key then could map to whatever keys we need
    // Is a key currently held down?
    bool up_key()   { return up_press; }
    bool down_key() { return down_press; }
    bool left_key() { return left_press; }
    bool right_key(){ return right_press; }
    bool ctrl_key() { return ctrl_press; }
    bool shift_key(){ return shift_press; }
    bool space_key(){ return space_press; }
    bool esc_key()  { return esc_press; }
    bool A_key()    { return A_press; }
    bool B_key()    { return B_press; }
    bool tab_key()  { return tab_press; }
    bool should_quit(){return quit;}//Has the little x at the top of the window been pressed? Technically a button so it is here

    //Was a key just clicked (true) or just released (false)
    bool A_key_click(bool click)
    {
        return (click && A_press && !pA_press) || (!click && !A_press && pA_press);
    }
    bool B_key_click(bool click)
    {
        return (click && B_press && !pB_press) || (!click && !B_press && pB_press);
    }
    bool tab_key_click(bool click)
    {
        return (click && tab_press && !ptab_press) || (!click && !tab_press && ptab_press);
    }
    bool down_key_click(bool click)
    {
        return (click && down_press && !pdown_press) || (!click && !down_press && pdown_press);
    }
    bool right_key_click(bool click)
    {
        return (click && right_press && !pright_press) || (!click && !right_press && pright_press);
    }
    bool left_key_click(bool click)
    {
        return (click && left_press && !pleft_press) || (!click && !left_press && pleft_press);
    }
    bool up_key_click(bool click)
    {
        return (click && up_press && !pup_press) || (!click && !up_press && pup_press);
    }
    bool ctrl_key_click(bool click)
    {
        return (click && ctrl_press && !pctrl_press) || (!click && !ctrl_press && pctrl_press);
    }
    bool shift_key_click(bool click)
    {
        return (click && shift_press && !pshift_press) || (!click && !shift_press && pshift_press);
    }
    bool space_key_click(bool click)
    {
        return (click && space_press && !pspace_press) || (!click && !space_press && pspace_press);
    }
    bool esc_key_click(bool click)
    {
        return (click && esc_press && !pesc_press) || (!click && !esc_press && pesc_press);
    }

    // ---- Get current key-map as textures  ----
    //Get a texture containing the different bound keys, even if they have been re-mapped
    tex_index get_UP_key_tex()  {return UP_KEY_tex;}
    tex_index get_LEFT_key_tex(){return LEFT_KEY_tex;}
    tex_index get_RIGHT_key_tex(){return RIGHT_KEY_tex;}
    tex_index get_DOWN_key_tex(){return DOWN_KEY_tex;}
    tex_index get_TAB_KEY_tex (){return TAB_KEY_tex ;}
    tex_index get_A_key_tex() {return A_KEY_tex;}
    tex_index get_B_key_tex(){return B_KEY_tex;}

    //---- Developer commandline ----


    void print_lines(const vector<string>& wall_of_text)//Mostly used to print list of commands when given :help
    {

        for (const string& S : wall_of_text)
            command_textures.push_back(IO::graphics::set_text(S));
    }

    void print(const string& txt)
    {

        command_textures.push_back(IO::graphics::set_text(txt));
    }
    //returns true if a command was just entered by the human, if so cmd is set equal to the input
    bool get_command(string& cmd)
    {
        if (new_command)
        {
            cmd = std::move(text_input);
            text_input = "";//Empty text input, do not make cmd a reference to text_input!
            new_command = false;
            return true;
        }
        else
            return false;
    }
}
