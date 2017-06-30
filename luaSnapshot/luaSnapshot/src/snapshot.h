#ifndef __SNAPSHOT_H__
#define __SNAPSHOT_H__

#include "lua.h"

int luaopen_snapshot(lua_State *L);
static int snapshot(lua_State *L);

#endif