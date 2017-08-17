//#define SNAPSHOT_BUILD_LIB 1
#include <snapshot/snapshot.h>
#include <stdbool.h>

#include <vector>
#include <map>
#include <string>

using std::vector;
using std::map;
using std::string;

vector<map<const void*, TSnapshotNode*> > virtualStack;
map<const void*, string> sourceTable;
map<const void*, bool> markTable;
SnapshotNode* result;
int nResult;

/* DEBUG_LOG */
static FILE* debug_fp = NULL;
static bool enable_debug = true;

static void debug_log(const char* fmt, ...)
{
    if (!enable_debug)
        return;
    if (debug_fp == NULL)
        debug_fp = fopen("snapshot.log", "w");
    if (debug_fp)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(debug_fp, fmt, args);
        va_end(args);
        fflush(debug_fp);
    }
}
/* DEBUG_LOG */

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
    snapshot_traverse_init();
    snapshot_traverse_table(L, NULL, "[registry]");
    snapshot_generate_result(L);
    snapshot_traverse_init();
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
