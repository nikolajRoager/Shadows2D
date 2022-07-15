#include "my_filesystem.hpp"



bool myfs_exists(const my_path& A)
{
    //LOL, there is no way to check this without std::filesystem, but WINDOWS SUCKS so that is not going to happen
    return true;
}

//Not as good as std::filesystem, does not take into account multiple valid paths, but WINDOWS SUCKS
bool myfs_equivalent(const my_path& A, const my_path& B)
{
   return A.is(B);
}


my_path my_path::filename() const
{
    //Seperate out the filename at the end, and return it as a path
    stringstream ss(mystring);
    string file_name;
     while (std::getline(ss, file_name, '/')) {}

     return my_path(file_name);
}
