#ifndef __TREE_H__
#define __TREE_H__

#include <stdbool.h>

typedef enum _SnapshotType
{
    SNAPSHOT_TABLE = 0,
    SNAPSHOT_LUA_FUNCTION = 1,
    SNAPSHOT_C_FUNCTION = 2,
    SNAPSHOT_USERDATA = 3,
    SNAPSHOT_THREAD = 4,
    SNAPSHOT_STRING = 5,
    SNAPSHOT_MASK = 7
} SnapshotType;

typedef struct _SnapshotNode
{
    const void * address;//Address of the GCobj
    SnapshotType type;//Type of the GCobj
    char * debuginfo;//Chunk name and line number, if any
    unsigned int size;//Memory of the GCobj contains, see traverse.c
    const void ** parents;//Address of Obj who refer to him
    char ** reference;//Infomation of the reference
    /** [metatable]
        [key] "%s"         //content of the string
        [environment]
        [upvalue]
        [uservalue]
        [stack] [%d]       //stack position
        [local] %s : %s:%d //variable name; chunk name; lines
        %s                 //key of the value
    */
    unsigned int nParent;//Numbers of references
} SnapshotNode;

#endif