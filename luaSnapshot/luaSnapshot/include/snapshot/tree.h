#ifndef __TREE_H__
#define __TREE_H__

#include <stdbool.h>

typedef enum _SnapshotType
{
    SNAPSHOT_TABLE = 0,
    SNAPSHOT_FUNCTION = 1,
    SNAPSHOT_USERDATA = 2,
    SNAPSHOT_THREAD = 3,
    SNAPSHOT_SOURCE = 4,
    SNAPSHOT_MARK = 5,
    SNAPSHOT_MASK = 7
} SnapshotType;

typedef struct _SnapshotNode
{
    struct _SnapshotNode **parents;
    size_t parentNum;
    void * address;
    SnapshotType type;
} SnapshotNode;

typedef struct _SnapshotNodeTable
{
    SnapshotNode header;
    bool weakk;
    bool weakv;
    struct _SnapshotNodeTable *metatable;
    SnapshotNode **keys;
    SnapshotNode **values;
    size_t kvNum;
} SnapshotNodeTable;

typedef struct _SnapshotNodeFunction
{
    SnapshotNode header;
    SnapshotNodeTable *environment;
    SnapshotNode *upvalue;
} SnapshotNodeFunction;

typedef struct _SnapshotNodeUserdata
{
    SnapshotNode header;
    SnapshotNodeTable *metatable;
    SnapshotNodeTable *uservalue;
} SnapshotNodeUserdata;

typedef struct _SnapshotNodeThread
{
    SnapshotNode header;
    SnapshotNode *stack;
    SnapshotNodeTable *environment;
    SnapshotNode *upvalue;
} SnapshotNodeThread;

#endif