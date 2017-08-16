//#define SNAPSHOT_BUILD_LIB 1
#include <snapshot/snapshot.h>
#include <stdbool.h>
#include <stdlib.h>

#include <vector>
#include <map>
#include <string>

using std::vector;
using std::map;
using std::string;

static vector<map<const void*, SnapshotNode*>> virtualStack;
static map<const void*, string> sourceTable;
static map<const void*, bool> markTable;
static SnapshotNode* result;
static int nResult;

static bool inited = false;

void snapshot_initialize(lua_State *L)
{
    luaL_checkversion(L);
    inited = true;
}

int snapshot_capture(lua_State * L)
{
    if (!inited)
        return -1;
    snapshot_destroy_result();
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    //lua_getglobal(L, "_G");
    snapshot_traverse_table(L, NULL, "[registry]");
    snapshot_generate_result(L);
    return 0;
}

void snapshot_clear()
{
    snapshot_destroy_result();
}

int snapshot_get_gcobj_num()
{
    if (result)
        return nResult;
    return -1;
}

int snapshot_get_gcobjs(const int len, SnapshotNode * out)
{
    int i;
    for (i = 0; i < len; i++)
    {
        out[i] = result[i];
    }
    return 0;
}

int snapshot_get_parent_num(int index)
{
    if (result)
        return result[index].nParent;
    return -1;
}

int snapshot_get_parents(const int index, const int len, const void ** pout, char ** rout)
{
    int i;
    for (i = 0; i < len; i++)
    {
        pout[i] = result[index].parents[i];
        rout[i] = result[index].reference[i];
    }
    return 0;
}

int main()
{
    lua_State *L = luaL_newstate();
    luaopen_base(L);
    luaopen_table(L);
    luaopen_io(L);
    luaopen_string(L);
    luaopen_math(L);

    snapshot_initialize(L);
    snapshot_capture(L);
    while (1);
    return 0;
}
