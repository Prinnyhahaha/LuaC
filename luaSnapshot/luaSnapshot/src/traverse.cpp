#include <snapshot/snapshot.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <map>
#include <string>

using std::vector;
using std::map;
using std::string;

extern vector<map<const void*, TSnapshotNode*> > virtualStack;
extern map<const void*, string> sourceTable;
extern map<const void*, bool> markTable;

/* DEBUG_LOG */
static FILE* debug_fp = NULL;
static bool enable_debug = true;

static void debug_log(const char* fmt, ...)
{
    if (!enable_debug)
        return;
    if (debug_fp == NULL)
        debug_fp = fopen("traverse.log", "w");
    if (debug_fp)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(debug_fp, fmt, args);
        va_end(args);
        fflush(debug_fp);
    }
}
/* DEBUG_LOG */

void snapshot_traverse_init()
{
    virtualStack.reserve(5);
    for (int i = 0; i < 5; i++)
    {
        virtualStack.push_back(map<const void*, TSnapshotNode*>());
    }
    for (size_t i = 0; i < virtualStack.size(); i++)
    {
        virtualStack[i].clear();
    }
    sourceTable.clear();
    markTable.clear();
}

static bool is_marked(const void *p)
{
    if (markTable.find(p) == markTable.end())
    {
        markTable[p] = true;
        return false;
    }
    return true;
}

static const void* readobject(lua_State *L, const void *parent, const char *desc)
{
    int t = lua_type(L, -1);
    int tidx = 0;
    switch (t) {
    case LUA_TTABLE:
        tidx = STACKPOS_TABLE;
        break;
    case LUA_TFUNCTION:
        tidx = STACKPOS_FUNCTION;
        break;
    case LUA_TTHREAD:
        tidx = STACKPOS_THREAD;
        break;
    case LUA_TUSERDATA:
        tidx = STACKPOS_USERDATA;
        break;
    case LUA_TSTRING:
        tidx = STACKPOS_STRING;
        break;
    default:
        return NULL;
    }

    const void * p = lua_topointer(L, -1);
    if (is_marked(p)) {
        if (virtualStack[tidx - 1].find(p) != virtualStack[tidx - 1].end())
        {
            virtualStack[tidx - 1][p]->parents[parent] = desc;
        }
        lua_pop(L, 1);
        return NULL;
    }

    TSnapshotNode* pnode = new TSnapshotNode();
    virtualStack[tidx - 1][p] = pnode;
    pnode->parents[parent] = desc;
    return p;
}

static const char* keystring(lua_State *L, int index, char *buffer)
{
    int t = lua_type(L, index);
    switch (t) {
    case LUA_TSTRING:
        return lua_tostring(L, index);
    case LUA_TNUMBER:
        sprintf(buffer, "[%lg]", lua_tonumber(L, index));
        break;
    case LUA_TBOOLEAN:
        sprintf(buffer, "[%s]", lua_toboolean(L, index) ? "true" : "false");
        break;
    case LUA_TNIL:
        sprintf(buffer, "[nil]");
        break;
    default:
        sprintf(buffer, "[%s:%p]", lua_typename(L, t), lua_topointer(L, index));
        break;
    }
    return buffer;
}

void snapshot_traverse_table(lua_State *L, const void * parent, const char * desc)
{
    const void * t = readobject(L, parent, desc);
    if (t == NULL)
        return;
    debug_log("traverse table %p, parent %p as %s\n", t, parent, desc);

    /* GCtable + array + hashnode(not including key/value's GCobject) */
    cTValue* tv = L->top - 1;
    assert(tv->it == LJ_TTAB);
    GCtab* tbl = &((GCobj*)(uintptr_t)(tv->gcr.gcptr32))->tab;
    unsigned int size = sizeof(GCtab) + sizeof(TValue) * tbl->asize + sizeof(Node) * (tbl->hmask + 1);
    virtualStack[STACKPOS_TABLE - 1][t]->size = size;

    bool weakk = false;
    bool weakv = false;
    if (lua_getmetatable(L, -1)) {
        lua_pushliteral(L, "__mode");
        lua_rawget(L, -2);
        if (lua_isstring(L, -1)) {
            const char *mode = lua_tostring(L, -1);
            if (strchr(mode, 'k')) {
                weakk = true;
            }
            if (strchr(mode, 'v')) {
                weakv = true;
            }
        }
        lua_pop(L, 1);
        luaL_checkstack(L, LUA_MINSTACK, NULL);
        snapshot_traverse_table(L, t, "[metatable]");
    }

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (weakv) {
            lua_pop(L, 1);
        }
        else {
            char temp[50];
            const char * desc = keystring(L, -2, temp);
            snapshot_traverse_object(L, t, desc);
        }
        if (!weakk) {
            lua_pushvalue(L, -1);
            snapshot_traverse_object(L, t, "[key]");
        }
    }

    lua_pop(L, 1);
}

void snapshot_traverse_function(lua_State *L, const void * parent, const char *desc)
{
    const void * t = readobject(L, parent, desc);
    if (t == NULL)
        return;
    debug_log("traverse function %p, parent %p as %s\n", t, parent, desc);

    /* GCfunction + upvalue + protosize(not including upvalues' GCobject) */
    cTValue* tv = L->top - 1;
    assert(tv->it == LJ_TFUNC);
    GCfunc* func = &((GCobj*)(uintptr_t)(tv->gcr.gcptr32))->fn;
    unsigned int size;
    if (func->c.ffid == FF_LUA)
    {
        size = (sizeof(GCfuncL) - sizeof(GCRef) + sizeof(GCRef)*((MSize)func->l.nupvalues));
        size += ((GCproto*)(uintptr_t)(func->l.pc.ptr32) - 1)->sizept;
    }
    else
    {
        size = (sizeof(GCfuncC) - sizeof(TValue) + sizeof(TValue)*((MSize)func->c.nupvalues));
        size += ((GCproto*)(uintptr_t)(func->c.pc.ptr32) - 1)->sizept;
    }
    virtualStack[STACKPOS_FUNCTION - 1][t]->size = size;

#if LUA_VERSION_NUM == 501
    lua_getfenv(L, -1);
    if (lua_istable(L, -1)) {
        snapshot_traverse_table(L, t, "[environment]");
    }
    else {
        lua_pop(L, 1);
    }
#endif

    int i;
    for (i = 1;; i++) {
        const char *name = lua_getupvalue(L, -1, i);
        if (name == NULL)
            break;
        snapshot_traverse_object(L, t, name[0] ? name : "[upvalue]");
    }
    if (lua_iscfunction(L, -1)) {
        if (i == 1) {
            // light c function
            virtualStack[STACKPOS_FUNCTION - 1].erase(t);
        }
        lua_pop(L, 1);
    }
    else {
        lua_Debug ar;
        lua_getinfo(L, ">S", &ar);
        string b = "";
        b = b + ar.short_src;
        char tmp[128];
        sprintf(tmp, ":%d", ar.linedefined);
        b = b + tmp;
        sourceTable[t] = b;
    }
}

void snapshot_traverse_userdata(lua_State *L, const void * parent, const char *desc)
{
    const void * t = readobject(L, parent, desc);
    if (t == NULL)
        return;
    debug_log("traverse userdata %p, parent %p as %s\n", t, parent, desc);

    /* GCudata + payloadsize */
    cTValue* tv = L->top - 1;
    assert(tv->it == LJ_TUDATA);
    GCudata* ud = &((GCobj*)(uintptr_t)(tv->gcr.gcptr32))->ud;
    unsigned int size = sizeof(GCudata) + ud->len;
    virtualStack[STACKPOS_USERDATA - 1][t]->size = size;

    if (lua_getmetatable(L, -1)) {
        snapshot_traverse_table(L, t, "[metatable]");
    }

    lua_getuservalue(L, -1);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
    }
    else {
        snapshot_traverse_table(L, t, "[uservalue]");
        lua_pop(L, 1);
    }
}

void snapshot_traverse_thread(lua_State *L, const void * parent, const char *desc)
{
    const void * t = readobject(L, parent, desc);
    if (t == NULL)
        return;
    debug_log("traverse thread %p, parent %p as %s\n", t, parent, desc);

    /* lua_State + stack(not include stack values' GCobject) */
    cTValue* tv = L->top - 1;
    assert(tv->it == LJ_TTHREAD);
    lua_State* th = &((GCobj*)(uintptr_t)(tv->gcr.gcptr32))->th;
    unsigned int size = sizeof(lua_State) + sizeof(TValue) * th->stacksize;
    virtualStack[STACKPOS_THREAD - 1][t]->size = size;

    int level = 0;
    lua_State *cL = lua_tothread(L, -1);
    if (cL == L) {
        level = 1;
    }
    else {
        int top = lua_gettop(cL);
        luaL_checkstack(cL, 1, NULL);
        int i;
        char tmp[128];
        for (i = 0; i < top; i++) {
            lua_pushvalue(cL, i + 1);
            sprintf(tmp, "[stack] [%d]", i + 1);
            snapshot_traverse_object(cL, cL, tmp);
        }
    }
    lua_Debug ar;
    string b = "";
    while (lua_getstack(cL, level, &ar)) {
        char tmp[128];
        lua_getinfo(cL, "Sl", &ar);
        b = b + ar.short_src;
        if (ar.currentline >= 0) {
            char tmp[128];
            sprintf(tmp, ":%d ", ar.currentline);
            b = b + tmp;
        }

        int i;
        for (i = 1;; i++) {
            const char * name = lua_getlocal(cL, &ar, i);
            if (name == NULL)
                break;
            snprintf(tmp, sizeof(tmp), "[local] %s : %s:%d", name, ar.short_src, ar.currentline);
            snapshot_traverse_object(cL, t, tmp);
        }

        ++level;
    }
    sourceTable[t] = b;
    lua_pop(L, 1);
}

void snapshot_traverse_object(lua_State *L, const void * parent, const char *desc)
{
    luaL_checkstack(L, LUA_MINSTACK, NULL);
    int t = lua_type(L, -1);
    switch (t) {
    case LUA_TTABLE:
        snapshot_traverse_table(L, parent, desc);
        break;
    case LUA_TUSERDATA:
        snapshot_traverse_userdata(L, parent, desc);
        break;
    case LUA_TFUNCTION:
        snapshot_traverse_function(L, parent, desc);
        break;
    case LUA_TTHREAD:
        snapshot_traverse_thread(L, parent, desc);
        break;
    case LUA_TSTRING:
        snapshot_traverse_string(L, parent, desc);
        break;
    default:
        debug_log("traverse non-GCobj\n");
        lua_pop(L, 1);
        break;
    }
}

void snapshot_traverse_string(lua_State * L, const void * parent, const char * desc)
{
    const char *str = lua_tostring(L, -1);
    char *tmp = (char*)malloc(strlen(str) + 4 + strlen(desc));
    sprintf(tmp, "%s \"%s\"", desc, str);
    const void * t = readobject(L, parent, tmp);
    if (t == NULL)
        return;
    debug_log("traverse string %p, parent %p as %s\n", t, parent, tmp);

    /* GCstr + payloadsize */
    cTValue* tv = L->top - 1;
    assert(tv->it == LJ_TSTR);
    GCstr* gcstr = &((GCobj*)(uintptr_t)(tv->gcr.gcptr32))->str;
    unsigned int size = sizeof(GCstr) + gcstr->len;
    virtualStack[STACKPOS_STRING - 1][t]->size = size;

    free(tmp);
    lua_pop(L, 1);
}
