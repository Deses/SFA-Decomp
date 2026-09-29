#ifndef DLLS_OBJECTS_611_GM_MAZEWELL_H_
#define DLLS_OBJECTS_611_GM_MAZEWELL_H_

#include "game/objects/object.h"
#include "global.h"
#include "main/objseq.h"
#include "dlls/object_descriptor.h"

typedef struct GmMazeWellQuestTables {
    s16 watched[9];
    u8 pad12[2];
    s16 reward[9];
    u8 pad26[2];
    s16 followup[8];
    s32 dialogue[8];
} GmMazeWellQuestTables;

STATIC_ASSERT(offsetof(GmMazeWellQuestTables, watched) == 0x00);
STATIC_ASSERT(offsetof(GmMazeWellQuestTables, reward) == 0x14);
STATIC_ASSERT(offsetof(GmMazeWellQuestTables, followup) == 0x28);
STATIC_ASSERT(offsetof(GmMazeWellQuestTables, dialogue) == 0x38);
STATIC_ASSERT(sizeof(GmMazeWellQuestTables) == 0x58);

typedef struct GmmazewellState {
    u8 unk0;             /* 0x00: cleared at init, never read */
    u8 savepointSet;     /* 0x01: savepoint stamped once player object is available */
    u8 pad2[2];          /* 0x02 */
    s32 pendingDialogue; /* 0x04: dialogue id queued for the next event 1 (-1 = none) */
} GmmazewellState;

STATIC_ASSERT(offsetof(GmmazewellState, pendingDialogue) == 0x4);
STATIC_ASSERT(sizeof(GmmazewellState) == 0x8);

int GM_MazeWell_SeqFn(GameObject* obj, int unused, ObjSeqState* animUpdate);
int GM_MazeWell_getExtraSize(void);
void GM_MazeWell_free(void);
void GM_MazeWell_render(void* obj, int p2, int p3, int p4, int p5, s8 visible);
void GM_MazeWell_update(GameObject* obj);
void GM_MazeWell_init(GameObject* obj);

extern GmMazeWellQuestTables gGmMazeWellQuestBits;
extern ObjectDescriptor gGmMazeWellObjDescriptor;

#endif /* DLLS_OBJECTS_611_GM_MAZEWELL_H_ */
