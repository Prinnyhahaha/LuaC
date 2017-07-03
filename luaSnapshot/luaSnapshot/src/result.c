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
    FILE* fp = fopen(typename, "w");
    lua_pushnil(dL);
    while (lua_next(dL, idx) != 0) {
        const void* key = lua_touserdata(dL, -2);
        void * p = lua_touserdata(dL, -2);
        lua_pushlightuserdata(dL, p);
        lua_gettable(dL, STACKPOS_MARK);
        unsigned int size = lua_tointeger(dL, -1);
        lua_pop(dL, 1);

        fprintf(fp, "ADDR: %p -> ", key);
        if (idx == STACKPOS_FUNCTION) {
            lua_rawgetp(dL, STACKPOS_SOURCE, key);
            if (lua_isnil(dL, -1)) {
                fprintf(fp, "cfunction [size: %u]\n", size);
            }
            else {
                size_t l = 0;
                const char * s = lua_tolstring(dL, -1, &l);
                fprintf(fp, "%s [size: %u]\n", s, size);
            }
            lua_pop(dL, 1);
        }
        else if (idx == STACKPOS_THREAD) {
            lua_rawgetp(dL, STACKPOS_SOURCE, key);
            size_t l = 0;
            const char * s = lua_tolstring(dL, -1, &l);
            fprintf(fp, "%s [size: %u]\n", s, size);
            lua_pop(dL, 1);
        }
        else {
            fprintf(fp, "%s [size: %u]\n", typename, size);
        }
        lua_pushnil(dL);
        while (lua_next(dL, -2) != 0) {
            const void * parent = lua_touserdata(dL, -2);
            const char * desc = luaL_checkstring(dL, -1);
            fprintf(fp, "    %p : %s\n", parent, desc);
            lua_pop(dL, 1);
        }
        lua_pop(dL, 1);

        //luaL_Buffer b;
        //luaL_buffinit(L, &b);
        //const void * key = lua_touserdata(dL, -2);

        //void * p = lua_touserdata(dL, -2);
        //lua_pushlightuserdata(dL, p);
        //lua_gettable(dL, STACKPOS_MARK);
        //unsigned int size = lua_tointeger(dL, -1);
        //lua_pop(dL, 1);

        //if (idx == STACKPOS_FUNCTION) {
        //    lua_rawgetp(dL, STACKPOS_SOURCE, key);
        //    if (lua_isnil(dL, -1)) {
        //        char tmp[50];
        //        sprintf(tmp, "cfunction size : %u", size);
        //        luaL_addstring(&b, tmp);
        //        luaL_addchar(&b, '\n');
        //    }
        //    else {
        //        size_t l = 0;
        //        const char * s = lua_tolstring(dL, -1, &l);
        //        luaL_addlstring(&b, s, l);
        //        char tmp[50];
        //        sprintf(tmp, " size : %u", size);
        //        luaL_addstring(&b, tmp);
        //        luaL_addchar(&b, '\n');
        //    }
        //    lua_pop(dL, 1);
        //}
        //else if (idx == STACKPOS_THREAD) {
        //    lua_rawgetp(dL, STACKPOS_SOURCE, key);
        //    size_t l = 0;
        //    const char * s = lua_tolstring(dL, -1, &l);
        //    luaL_addlstring(&b, s, l);
        //    char tmp[50];
        //    sprintf(tmp, " size : %u", size);
        //    luaL_addstring(&b, tmp);
        //    luaL_addchar(&b, '\n');
        //    lua_pop(dL, 1);
        //}
        //else {
        //    char tmp[50];
        //    sprintf(tmp, "%s size : %u", typename, size);
        //    luaL_addstring(&b, tmp);
        //    luaL_addchar(&b, '\n');
        //}
        //lua_pushnil(dL);
        //while (lua_next(dL, -2) != 0) {
        //    const void * parent = lua_touserdata(dL, -2);
        //    const char * desc = luaL_checkstring(dL, -1);
        //    gen_table_desc(dL, &b, parent, desc);
        //    lua_pop(dL, 1);
        //}
        //luaL_pushresult(&b);

        //fprintf(fp, "%p ", key);
        //fprintf(fp, "%s", lua_tostring(L, -1));
        //lua_pop(L, 1);

        //lua_pop(dL, 1);
    }
    fclose(fp);
    fp = NULL;
}

void snapshot_generate_result(lua_State *L, lua_State *dL) {
    pdesc(L, dL, STACKPOS_TABLE, "table");
    pdesc(L, dL, STACKPOS_USERDATA, "userdata");
    pdesc(L, dL, STACKPOS_FUNCTION, "function");
    pdesc(L, dL, STACKPOS_THREAD, "thread");
    pdesc(L, dL, STACKPOS_STRING, "string");
}