#ifndef __SNAPSHOT_H__
#define __SNAPSHOT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
#ifdef SNAPSHOT_DLL
#ifdef SNAPSHOT_BUILD_LIB
#define SNAPSHOT_API __declspec(dllexport)
#else
#define SNAPSHOT_API __declspec(dllimport)
#endif
#else
#define SNAPSHOT_API extern
#endif
#else
#define SNAPSHOT_API extern
#endif

#include <lua.h>
#include <lauxlib.h>
#if LUA_VERSION_NUM == 501
#include <snapshot/lua51.h>
#endif

#include <snapshot/tree.h>

    SNAPSHOT_API void snapshot_initialize(lua_State *L);
    SNAPSHOT_API int snapshot_capture(lua_State *L);
    extern void snapshot_traverse_table(lua_State *L, SnapshotNode* parent);
    extern void snapshot_traverse_function(lua_State *L, SnapshotNode* parent);
    extern void snapshot_traverse_userdata(lua_State *L, SnapshotNode* parent);
    extern void snapshot_traverse_thread(lua_State *L, SnapshotNode* parent);
    extern void snapshot_traverse_object(lua_State *L, SnapshotNode* parent);

#ifdef __cplusplus
}
#endif

#endif