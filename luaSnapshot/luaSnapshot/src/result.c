#include <snapshot/snapshot.h>

static int count_table(lua_State *L, int idx)
{
    int n = 0;
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        ++n;
        lua_pop(L, 1);
    }
    return n;
}

static void gen_table_desc(lua_State *dL, luaL_Buffer *b, const void * parent, const char *desc)
{
    char tmp[128];
    size_t l = sprintf(tmp, "%p : ", parent);
    luaL_addlstring(b, tmp, l);
    luaL_addstring(b, desc);
    luaL_addchar(b, '\n');
}

static void pdesc(lua_State *L, lua_State *dL, int idx, const char * typename)
{
    lua_pushnil(dL);
    while (lua_next(dL, idx) != 0) {
        luaL_Buffer b;
        luaL_buffinit(L, &b);
        const void * key = lua_touserdata(dL, -2);
        if (idx == STACKPOS_FUNCTION) {
            lua_rawgetp(dL, STACKPOS_SOURCE, key);
            if (lua_isnil(dL, -1)) {
                luaL_addstring(&b, "cfunction\n");
            }
            else {
                size_t l = 0;
                const char * s = lua_tolstring(dL, -1, &l);
                luaL_addlstring(&b, s, l);
                luaL_addchar(&b, '\n');
            }
            lua_pop(dL, 1);
        }
        else if (idx == STACKPOS_THREAD) {
            lua_rawgetp(dL, STACKPOS_SOURCE, key);
            size_t l = 0;
            const char * s = lua_tolstring(dL, -1, &l);
            luaL_addlstring(&b, s, l);
            luaL_addchar(&b, '\n');
            lua_pop(dL, 1);
        }
        else {
            luaL_addstring(&b, typename);
            luaL_addchar(&b, '\n');
        }
        lua_pushnil(dL);
        while (lua_next(dL, -2) != 0) {
            const void * parent = lua_touserdata(dL, -2);
            const char * desc = luaL_checkstring(dL, -1);
            gen_table_desc(dL, &b, parent, desc);
            lua_pop(dL, 1);
        }
        luaL_pushresult(&b);
        lua_rawsetp(L, -2, key);
        lua_pop(dL, 1);
    }
}

void snapshot_generate_result(lua_State *L, lua_State *dL) {
    int count = 0;
    count += count_table(dL, STACKPOS_TABLE);
    count += count_table(dL, STACKPOS_FUNCTION);
    count += count_table(dL, STACKPOS_USERDATA);
    count += count_table(dL, STACKPOS_THREAD);
    count += count_table(dL, STACKPOS_STRING);
    lua_createtable(L, 0, count);
    pdesc(L, dL, STACKPOS_TABLE, "table");
    pdesc(L, dL, STACKPOS_USERDATA, "userdata");
    pdesc(L, dL, STACKPOS_FUNCTION, "function");
    pdesc(L, dL, STACKPOS_THREAD, "thread");
    pdesc(L, dL, STACKPOS_STRING, "string");
}