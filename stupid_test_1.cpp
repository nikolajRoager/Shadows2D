#include<iostream>
//Just include filesystem, don't actually use it for anything
#include<filesystem>


int main(int argc, char* argv[])
{
    std::cout<<"We are in "<<std::filesystem::current_path()<<endl;
}
