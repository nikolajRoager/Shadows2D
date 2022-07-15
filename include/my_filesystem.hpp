//WINDOWS SUCKS!
//The c++ std filesystem library, obviously, does not work on windows, so this is an alternative with exactly the same bloody functionalities, WHY MICROSOFT WHY! WHY IS SOMETHING MADE BY A BILLION DOLLAR COMPANY SO INFERIOR TO LINUX, WHICH IS BLOODY FREE

#include<string>
#include<sstream>

#pragma once

using namespace std;

class my_path
{
private:
    string mystring;

public:

    my_path(const char* pathname)
    {
        mystring = pathname;
    }
    my_path(string pathname="")
    {
        mystring = pathname;
    }

    my_path operator/(const my_path& that) const
    {

        return my_path(mystring+"/"+that.mystring);
    }

    string String() const
    {
        return mystring;
    }

    my_path filename() const;

    //Not as good as std::filesystem, but WINDOWS SUCKS
    bool is(const my_path& that) const
    { return that.mystring.compare(mystring)==0;}
};

bool myfs_exists(const my_path& A);
bool myfs_equivalent(const my_path& A, const my_path& B);
