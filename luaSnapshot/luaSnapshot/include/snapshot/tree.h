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

typedef struct _SnapshotParent
{
    void *address;
} SnapshotParent;

typedef struct _SnapshotNode
{
    SnapshotParent **parents;
    size_t parentNum;
    size_t parentSize;
    void * address;
    SnapshotType type;
} SnapshotNode;

#endif