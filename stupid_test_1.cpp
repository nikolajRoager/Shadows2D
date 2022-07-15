#include<fstream>
#include<filesystem>


using namespace std;
int main(int argc, char* argv[])
{
    ofstream OUT("location.txt");
    OUT<<std::filesystem::current_path()<<endl;
    OUT.close();
    return 0;
}
