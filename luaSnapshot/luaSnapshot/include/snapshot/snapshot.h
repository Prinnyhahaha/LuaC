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
#include <lualib.h>
#include <lj_obj.h>
#if LUA_VERSION_NUM == 501
#include <snapshot/lua51.h>
#endif

#include <snapshot/tree.h>

#define STACKPOS_TABLE 1
#define STACKPOS_FUNCTION 2
#define STACKPOS_USERDATA 3
#define STACKPOS_THREAD 4
#define STACKPOS_STRING 5
#define STACKPOS_SOURCE 6
#define STACKPOS_MARK 7

    SNAPSHOT_API void snapshot_initialize(lua_State *L);
    SNAPSHOT_API int snapshot_capture(lua_State *L);
    SNAPSHOT_API void snapshot_clear();
    SNAPSHOT_API int snapshot_get_gcobj_num();
    SNAPSHOT_API int snapshot_get_gcobjs(const int len, SnapshotNode* out);
    SNAPSHOT_API int snapshot_get_parent_num(int index);
    SNAPSHOT_API int snapshot_get_parents(const int index, const int len, const void** pout, char** rout);

    extern void snapshot_traverse_table(lua_State *L, lua_State *dL, const void* parent, const char* desc);
    extern void snapshot_traverse_function(lua_State *L, lua_State *dL, const void* parent, const char* desc);
    extern void snapshot_traverse_userdata(lua_State *L, lua_State *dL, const void* parent, const char* desc);
    extern void snapshot_traverse_thread(lua_State *L, lua_State *dL, const void* parent, const char* desc);
    extern void snapshot_traverse_object(lua_State *L, lua_State *dL, const void* parent, const char* desc);
    extern void snapshot_traverse_string(lua_State *L, lua_State *dL, const void* parent, const char* desc);

    extern void snapshot_generate_result(lua_State *L, lua_State *dL);
    extern void snapshot_destroy_result();

#ifdef __cplusplus
}
#endif

#endif