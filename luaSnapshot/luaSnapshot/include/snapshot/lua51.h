#ifndef __LUA51_EX_H__
#define __LUA51_EX_H__

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
    void luaL_checkversion(lua_State *L);
    void lua_rawgetp(lua_State *L, int idx, const void *p);
    void lua_rawsetp(lua_State *L, int idx, const void *p);
    void lua_getuservalue(lua_State *L, int idx);
}

#endif