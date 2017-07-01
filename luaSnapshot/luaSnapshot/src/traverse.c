#include <snapshot/snapshot.h>
#include <assert.h>
#include <stdlib.h>

static SnapshotNode* is_marked(const void *p)
{
    //TODO
    return NULL;
}

static void do_mark(const void *p)
{
    //TODO
}

SnapshotNodeTable* snapshot_traverse_table(lua_State *L, SnapshotNode *parent)
{
    int t = lua_type(L, -1);
    assert(t == LUA_TTABLE);
    const void * p = lua_topointer(L, -1);
    if (p == NULL)
    {
        lua_pop(L, 1);
        return NULL;
    }
    SnapshotNodeTable *nodeptr = is_marked(p);
    if (nodeptr != NULL)
    {
        assert(nodeptr->header.type == SNAPSHOT_TABLE);
        lua_pop(L, 1);
        return nodeptr;
    }
    do_mark(p);
    SnapshotNodeTable *nodeptr = (SnapshotNodeTable*)malloc(sizeof(SnapshotNodeTable));
    nodeptr->header.parents[nodeptr->header.parentNum++] = parent;
    nodeptr->header.address = p;
    nodeptr->header.type = SNAPSHOT_TABLE;

    /* Check metatable£¬weak key and weak value */
    nodeptr->weakk = false;
    nodeptr->weakv = false;
    if (lua_getmetatable(L, -1)) {
        lua_pushliteral(L, "__mode");
        lua_rawget(L, -2);
        if (lua_isstring(L, -1)) {
            const char *mode = lua_tostring(L, -1);
            if (strchr(mode, 'k')) {
                nodeptr->weakk = true;
            }
            if (strchr(mode, 'v')) {
                nodeptr->weakv = true;
            }
        }
        lua_pop(L, 1);
        luaL_checkstack(L, LUA_MINSTACK, NULL);
        nodeptr->metatable = snapshot_traverse_table(L, nodeptr);
    }

    /* Traverse keys and values */
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (nodeptr->weakv) {
            nodeptr->values[nodeptr->kvNum] = NULL;
        }
        else {
            nodeptr->values[nodeptr->kvNum] = snapshot_traverse_object(L, nodeptr);
        }
        if (nodeptr->weakk)
        {
            nodeptr->keys[nodeptr->kvNum] = NULL;
        }
        else
        {
            nodeptr->keys[nodeptr->kvNum] = snapshot_traverse_object(L, nodeptr);
        }
    }

    lua_pop(L, 1);
    return nodeptr;
}

SnapshotNodeFunction * snapshot_traverse_function(lua_State * L, SnapshotNode * parent)
{
    return NULL;
}

SnapshotNodeUserdata * snapshot_traverse_userdata(lua_State * L, SnapshotNode * parent)
{
    return NULL;
}

SnapshotNodeThread * snapshot_traverse_thread(lua_State * L, SnapshotNode * parent)
{
    return NULL;
}

SnapshotNode * snapshot_traverse_object(lua_State * L, SnapshotNode * parent)
{
    return NULL;
}
