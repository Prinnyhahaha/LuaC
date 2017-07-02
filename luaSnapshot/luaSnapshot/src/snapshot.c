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
    snapshot_traverse_table(L, dL, NULL, "[registry]");
    snapshot_generate_result(L, dL);
    lua_close(dL);

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        const char *p = lua_tostring(L, -2);
        const void *pp = lua_topointer(L, -2);
        printf("%s %p ", p, pp);
        printf("%s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return 1;
}

int snapshot_capture_ex(lua_State *L)
{
    snapshot_capture(L);
    lua_pop(L, 1);
    return 1;
}

int luaopen_snapshot(lua_State *L) {
    snapshot_initialize(L);
    lua_pushcfunction(L, snapshot_capture);
    return 1;
}

int main()
{
    lua_State *L = luaL_newstate();

    snapshot_initialize(L);
    int i = snapshot_capture(L);
    printf("===%d\n", i);
    i = snapshot_capture(L);
    printf("===%d\n", i);
    while (1);
    return 0;
}
