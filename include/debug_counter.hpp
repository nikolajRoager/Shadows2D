//For debugging purpose only, count how many times basic operations happen

#include <string>
#include <vector>
#include<filesystem>
#include<fstream>
#include<cstdint>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>


using namespace std;
using namespace glm;
using uchar = uint8_t;
using ushort= uint16_t;
using uint = uint32_t;
using ulong = uint64_t;


class debug_counter
{
    static uint mesh_collision_calls;
    static uint mesh_collision_bail_early;//Bail early due to bounding box not matching
    static uint mesh_ray_edge_check;//Collision check between ray and edge in a mesh

    static void mesh_coll_call();
    static void mesh_coll_call();
    static void reset();

    static void report();
}
