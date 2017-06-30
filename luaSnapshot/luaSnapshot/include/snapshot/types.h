#ifndef __TYPES_H__
#define __TYPES_H__

typedef enum _SnapshotType
{
    SNAPSHOT_TABLE = 1,
    SNAPSHOT_FUNCTION = 2,
    SNAPSHOT_THREAD = 3,
    SNAPSHOT_USERDATA = 4,
    SNAPSHOT_SOURCE = 5,
    SNAPSHOT_MARK = 6,
} SnapshotType;

#endif
