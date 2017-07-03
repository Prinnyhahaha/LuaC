//#define SNAPSHOT_BUILD_LIB 1
#include <snapshot/snapshot.h>
#include <stdbool.h>
#include <stdlib.h>

static bool inited = false;
static lua_State* dL = NULL;

void snapshot_initialize(lua_State *L)
{
    luaL_checkversion(L);
    inited = true;
}

int snapshot_capture(lua_State * L)
{
    if (!inited)
        return -1;
    dL = luaL_newstate();
    int i;
    for (i = 0; i < 7; i++) {
        lua_newtable(dL);
    }
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    //lua_getglobal(L, "_G");
    snapshot_traverse_table(L, dL, NULL, "[registry]");
    //snapshot_generate_result(L, dL);
    lua_close(dL);
    return 0;
}

int snapshot_dumb(lua_State * L)
{
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    return 0;
}

int luaopen_snapshot(lua_State *L) {
    snapshot_initialize(L);
    lua_pushcfunction(L, snapshot_capture);
    return 1;
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
