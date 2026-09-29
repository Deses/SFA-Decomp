#include "main/objtype.h"
#include "game/objects/object.h"
#include "main/vecmath.h"
#include "dolphin/mtx.h"
#include "dolphin/os/OSReport.h"
#include "MSL_C/PPCEABI/bare/H/math_api.h"
#include <string.h>

#define OBJTYPE_COUNT 0x54
#define OBJTYPE_INDEX_COUNT (OBJTYPE_COUNT + 1)
#define OBJTYPE_LIST_MAX 0x100

typedef struct ObjectTypeIndexTable {
    u8 offsets[OBJTYPE_INDEX_COUNT];
    u8 reserved[3];
} ObjectTypeIndexTable;
STATIC_ASSERT(sizeof(ObjectTypeIndexTable) == 0x58);

GameObject* gObjectTypeList[OBJTYPE_LIST_MAX];
extern ObjectTypeIndexTable gObjectTypeIndices;
u8 gObjectTypeListCount;

int objIsObjectType(GameObject* obj, int group) {
    GameObject** entry;
    u32 index;
    u32 limit;
    u32 limitXorIndex;
    int halfDiff;

    if ((group < 0) || (group >= OBJTYPE_COUNT)) {
        return 0;
    }
    index = gObjectTypeIndices.offsets[group];
    limit = gObjectTypeIndices.offsets[group + 1];
    for (entry = gObjectTypeList + index; ((int)index < (int)limit && (obj != *entry));
         entry = entry + 1, index = index + 1) {
    }
    limitXorIndex = limit ^ index;
    halfDiff = (int)limitXorIndex >> 1;
    limitXorIndex &= limit;
    return (u32)(halfDiff - limitXorIndex) >> 0x1f;
}

GameObject* objGetNearestType(int group, float* point, float* maxDistance) {
    GameObject** entry;
    GameObject* nearest;
    int index;
    int limit;
    float distanceSq;
    float bestDistanceSq;

    nearest = 0;
    bestDistanceSq = *maxDistance * *maxDistance;
    if ((group < 0) || (group >= OBJTYPE_COUNT)) {
        return 0;
    }
    index = gObjectTypeIndices.offsets[group];
    limit = gObjectTypeIndices.offsets[group + 1];
    entry = gObjectTypeList + index;
    while (index < limit) {
        if (*entry != 0) {
            distanceSq = PSVECSquareDistance((Vec*)point, &((GameObject*)*entry)->anim.worldPos);
            if (distanceSq < bestDistanceSq) {
                bestDistanceSq = distanceSq;
                nearest = *entry;
            }
            entry++;
            index++;
        }
    }
    if (nearest != 0) {
        *maxDistance = sqrtf(bestDistanceSq);
    }
    return nearest;
}

GameObject* objGetNearestTypeToExcludingSelf(int group, GameObject* obj, float* maxDistance) {
    GameObject** entry;
    GameObject* nearest;
    int index;
    int limit;
    float distanceSq;
    float bestDistanceSq;

    nearest = 0;
    if ((group < 0) || (group >= OBJTYPE_COUNT)) {
        return 0;
    }
    if (maxDistance != (float*)0x0) {
        bestDistanceSq = *maxDistance * *maxDistance;
    } else {
        bestDistanceSq = 3.4028235e38f;
    }
    index = gObjectTypeIndices.offsets[group];
    limit = gObjectTypeIndices.offsets[group + 1];
    entry = gObjectTypeList + index;
    while (index < limit) {
        if ((GameObject*)*entry != obj) {
            distanceSq = vec3f_distanceSquared(&obj->anim.worldPosX, &((GameObject*)*entry)->anim.worldPosX);
            if (distanceSq < bestDistanceSq) {
                bestDistanceSq = distanceSq;
                nearest = (GameObject*)*entry;
            }
        }
        entry++;
        index++;
    }
    if ((nearest != 0) && (maxDistance != (float*)0x0)) {
        *maxDistance = sqrtf(bestDistanceSq);
    }
    return nearest;
}

GameObject* objGetNearestTypeTo(int group, GameObject* obj, float* maxDistance) {
    GameObject** entry;
    GameObject* nearest;
    GameObject* o;
    int index;
    int limit;
    float distanceSq;
    float bestDistanceSq;

    nearest = 0;
    if ((group < 0) || (group >= OBJTYPE_COUNT)) {
        return 0;
    }
    if (maxDistance != (float*)0x0) {
        bestDistanceSq = *maxDistance * *maxDistance;
    } else {
        bestDistanceSq = 3.4028235e38f;
    }
    o = obj;
    index = gObjectTypeIndices.offsets[group];
    limit = gObjectTypeIndices.offsets[group + 1];
    entry = gObjectTypeList + index;
    while (index < limit) {
        if ((GameObject*)*entry != o) {
            distanceSq = vec3f_distanceSquared(&o->anim.worldPosX, &((GameObject*)*entry)->anim.worldPosX);
            if (distanceSq < bestDistanceSq) {
                bestDistanceSq = distanceSq;
                nearest = (GameObject*)*entry;
            }
        }
        entry++;
        index++;
    }
    if ((nearest != 0) && (maxDistance != (float*)0x0)) {
        *maxDistance = sqrtf(bestDistanceSq);
    }
    return nearest;
}


GameObject** objGetAllOfType(int group, int* countOut) {
    if (group < 0 || group >= OBJTYPE_COUNT) {
        *countOut = 0;
        return 0x0;
    }
    *countOut = gObjectTypeIndices.offsets[group + 1] - gObjectTypeIndices.offsets[group];
    return gObjectTypeList + gObjectTypeIndices.offsets[group];
}

void objFreeObjectType(GameObject* obj, int group) {
    u8* offset;
    u8 count;
    int index;
    int limit;
    GameObject** entries;

    if ((group < 0) || (group >= OBJTYPE_COUNT)) {
        return;
    }
    offset = gObjectTypeIndices.offsets;
    index = offset[group];
    offset += group;
    limit = offset[1];
    entries = gObjectTypeList + index;
    while ((index < limit) && (*entries != obj)) {
        entries++;
        index++;
    }
    if (index >= limit) {
        return;
    }
    count = (gObjectTypeListCount -= 1);
    entries = gObjectTypeList + index;
    while (index < count) {
        *entries = entries[1];
        entries++;
        index++;
    }
    group++;
    offset = gObjectTypeIndices.offsets + group;
    while (group <= OBJTYPE_COUNT) {
        (*offset)--;
        offset++;
        group++;
    }
}

int objGetObjectType(GameObject* obj) {
    int group;
    int objectIndex;

    for (objectIndex = 0; objectIndex < (int)(u32)gObjectTypeListCount; objectIndex++) {
        GameObject* entryObj = gObjectTypeList[objectIndex];
        if (entryObj == obj) {
            group = 0;
            while (((int)(u32)gObjectTypeIndices.offsets[group] <= objectIndex) && (group < OBJTYPE_INDEX_COUNT)) {
                group++;
            }
            return group;
        }
    }
    return 0;
}

char sObjAddObjectTypeReachedMaxTypes[38] = "objAddObjectType: Reached MAXTYPES!!\n\000";

void objAddObjectType(GameObject* obj, int group) {
    u8* offset;
    int count;
    int index;
    int limit;
    int insertIndex;
    GameObject** entries;

    if ((group < 0) || (group >= OBJTYPE_COUNT)) {
        return;
    }
    if ((int)(u32)gObjectTypeListCount >= OBJTYPE_LIST_MAX) {
        OSReport(sObjAddObjectTypeReachedMaxTypes);
        return;
    }
    offset = gObjectTypeIndices.offsets;
    insertIndex = offset[group];
    offset += group;
    limit = offset[1];
    entries = gObjectTypeList + insertIndex;
    for (index = insertIndex; index < limit; index++) {
        if (*entries == obj) {
            return;
        }
        entries++;
    }
    insertIndex = (limit - insertIndex == 0) ? insertIndex : (limit - 1);
    gObjectTypeListCount++;
    count = (int)(u32)gObjectTypeListCount;
    count--;
    entries = gObjectTypeList + count;
    for (index = count; insertIndex < index; index--) {
        *entries = entries[-1];
        entries--;
    }
    gObjectTypeList[insertIndex] = obj;
    group++;
    offset = gObjectTypeIndices.offsets + group;
    while (group <= OBJTYPE_COUNT) {
        (*offset)++;
        offset++;
        group++;
    }
}

void objTypeInit(void) {
    memset(gObjectTypeIndices.offsets, 0, sizeof(gObjectTypeIndices.offsets));
    gObjectTypeListCount = 0;
    return;
}


ObjectTypeIndexTable gObjectTypeIndices;
