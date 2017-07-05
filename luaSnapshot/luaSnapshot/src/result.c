#include <snapshot/snapshot.h>
#include <stdarg.h>

SnapshotNode* result = NULL;
int resultIdx = 0;

static FILE* debug_fp = NULL;
static bool enable_debug = true;

static void debug_log(const char* fmt, ...)
{
    if (!enable_debug)
        return;
    if (debug_fp == NULL)
        debug_fp = fopen("result.log", "w");
    if (debug_fp)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(debug_fp, fmt, args);
        va_end(args);
        fflush(debug_fp);
    }
}

static int count_table(lua_State *L, int idx)
{
    int n = 0;
    lua_pushnil(L);
    while (lua_next(L, idx) != 0)
    {
        ++n;
        lua_pop(L, 1);
    }
    return n;
}

static void generate(lua_State* L, lua_State* dL, int idx)
{
    debug_log("generate %d...\n", idx);
    lua_pushnil(dL);
    while (lua_next(dL, idx) != 0) {
        const void* address = lua_touserdata(dL, -2);
        debug_log("  addr: %p\n", address);

        SnapshotType type;
        char* debug_info = NULL;
        switch (idx)
        {
        case STACKPOS_TABLE:
            type = SNAPSHOT_TABLE;
            debug_info = NULL;
            break;
        case STACKPOS_FUNCTION:
            lua_rawgetp(dL, STACKPOS_SOURCE, address);
            if (lua_isnil(dL, -1)) {
                type = SNAPSHOT_C_FUNCTION;
                debug_info = NULL;
            }
            else {
                size_t l = 0;
                const char * s = lua_tolstring(dL, -1, &l);
                type = SNAPSHOT_LUA_FUNCTION;
                debug_info = (char*)malloc(l + 1);
                strcpy(debug_info, s);
            }
            lua_pop(dL, 1);
            break;
        case STACKPOS_USERDATA:
            type = SNAPSHOT_USERDATA;
            debug_info = NULL;
            break;
        case STACKPOS_THREAD:
            lua_rawgetp(dL, STACKPOS_SOURCE, address);
            size_t l = 0;
            const char * s = lua_tolstring(dL, -1, &l);
            type = SNAPSHOT_THREAD;
            debug_info = (char*)malloc(l + 1);
            strcpy(debug_info, s);
            lua_pop(dL, 1);
            break;
        case STACKPOS_STRING:
            type = SNAPSHOT_STRING;
            debug_info = NULL;
            break;
        default:
            type = SNAPSHOT_MASK;
            debug_info = NULL;
            break;
        }
        if (debug_info)
        {
            debug_log("  type: %d %s\n", type, debug_info);
        }
        else
        {
            debug_log("  type: %d\n", type);
        }

        void * p = lua_touserdata(dL, -2);
        lua_pushlightuserdata(dL, p);
        lua_gettable(dL, STACKPOS_MARK);
        unsigned int size = lua_tointeger(dL, -1);
        lua_pop(dL, 1);
        debug_log("  size %u\n", size);

        unsigned int parentNum = 0;
        lua_pushnil(dL);
        while (lua_next(dL, -2) != 0) {
            parentNum++;
            lua_pop(dL, 1);
        }

        const void ** parents = (const void**)malloc(sizeof(void*) * parentNum);
        char ** reference = (char**)malloc(sizeof(char*) * parentNum);
        int parentIdx = 0;
        lua_pushnil(dL);
        while (lua_next(dL, -2) != 0) {
            const void * parent = lua_touserdata(dL, -2);
            const char * desc = luaL_checkstring(dL, -1);
            parents[parentIdx] = parent;
            reference[parentIdx] = (char*)malloc(strlen(desc) + 1);
            strcpy(reference[parentIdx], desc);
            parentIdx++;
            debug_log("    parent: %p, %s", parent, desc);
            lua_pop(dL, 1);
        }
        lua_pop(dL, 1);

        result[resultIdx].address = address;
        result[resultIdx].type = type;
        result[resultIdx].debuginfo = debug_info;
        result[resultIdx].size = size;
        result[resultIdx].parents = parents;
        result[resultIdx].reference = reference;
        result[resultIdx].parentNum = parentNum;
        resultIdx++;
    }
}

void snapshot_generate_result(lua_State *L, lua_State *dL)
{
    int num = 0;
    num += count_table(dL, STACKPOS_TABLE);
    num += count_table(dL, STACKPOS_FUNCTION);
    num += count_table(dL, STACKPOS_USERDATA);
    num += count_table(dL, STACKPOS_THREAD);
    num += count_table(dL, STACKPOS_STRING);
    debug_log("count: %d\n", num);
    result = (SnapshotNode*)malloc(sizeof(SnapshotNode) * num);
    debug_log("result: %p\n", result);
    if (result == NULL)
        return;
    resultIdx = 0;
    generate(L, dL, STACKPOS_TABLE);
    generate(L, dL, STACKPOS_FUNCTION);
    generate(L, dL, STACKPOS_USERDATA);
    generate(L, dL, STACKPOS_THREAD);
    generate(L, dL, STACKPOS_STRING);
}

void snapshot_destroy_result()
{
    if (result == NULL)
        return;
    int i, j;
    for (i = 0; i < resultIdx; i++)
    {
        if (result[i].debuginfo)
        {
            free(result[i].debuginfo);
            result[i].debuginfo = NULL;
        }
        for (j = 0; j < result[i].parentNum; j++)
        {
            free(result[i].reference[j]);
        }
        free(result[i].parents);
        free(result[i].reference);
        result[i].parents = NULL;
        result[i].reference = NULL;
    }
    free(result);
    result = NULL;
}
