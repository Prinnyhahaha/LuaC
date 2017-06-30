#include <snapshot/lua51.h>

void luaL_checkversion(lua_State *L)
{
    if (lua_pushthread(L) == 0) {
        luaL_error(L, "Must require in main thread");
    }
    lua_setfield(L, LUA_REGISTRYINDEX, "mainthread");
}

void lua_rawgetp(lua_State *L, int idx, const void *p)
{
    if (idx < 0) {
        idx += lua_gettop(L) + 1;
    }
    lua_pushlightuserdata(L, (void *)p);
    lua_rawget(L, idx);
}

void lua_rawsetp(lua_State *L, int idx, const void *p)
{
    if (idx < 0) {
        idx += lua_gettop(L) + 1;
    }
    lua_pushlightuserdata(L, (void *)p);
    lua_insert(L, -2);
    lua_rawset(L, idx);
}

void lua_getuservalue(lua_State *L, int idx)
{
    lua_getfenv(L, idx);
}
