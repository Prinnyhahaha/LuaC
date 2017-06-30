//#define SNAPSHOT_BUILD_LIB 1
#include <snapshot/snapshot.h>
#include <stdbool.h>

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
    return 0;
}

int main()
{
    lua_State *L = luaL_newstate();

    snapshot_initialize(L);
    int i = snapshot_capture(L);
    printf("%d\n", i);
    while (1);
    return 0;
}