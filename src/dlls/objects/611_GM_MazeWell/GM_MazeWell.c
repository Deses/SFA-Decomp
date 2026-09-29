/*
 * GM_MazeWell (DLL 611) - the wishing/quest well in the maze area.
 *
 * The well watches a fixed set of nine quest/event game bits (the
 * quest-table rows below). While the well's hitbox is being touched
 * (INTERACT_FLAG_ACTIVATED) it scans those bits for a ready event: when
 * one fires it grants that row's reward bits, optionally unlocks a cheat
 * (rows 0-2), records the row's dialogue id as a pending trigger, and
 * runs sequence 0 with input disabled. The pending dialogue is shown the
 * next time event 1 is received (GM_MazeWell_SeqFn).
 *
 * On enter it stamps a savepoint at the player and plays maze-well music
 * (track 0x36, gated by game bit 0xEFC); leaving stops both. The prompt
 * is suppressed (INTERACT_FLAG_PROMPT_SUPPRESSED) whenever no watched
 * event is currently ready.
 */
#include "dlls/objects/611_GM_MazeWell.h"
#include "main/audio/music_api.h"
#include "main/dll/dll_0015_save_settings.h"
#include "main/game_ui_interface.h"
#include "main/gamebits.h"
#include "main/mapEventTypes.h"
#include "main/objprint_render_api.h"
#include "main/pad_api.h"
#include "sys/objects.h"
#include "main/objseq.h"
#include "main/textrender_api.h"
#include "main/object_render.h"
#include "dolphin/pad.h"
#include "main/shader_api.h"

/* The packed tables contain nine watched/reward bits but only eight follow-up
 * bits and dialogue IDs. Keep their retail offsets and the unchecked ninth row. */
#define QUEST_BIT_COUNT 9

/* music track toggled while the well is active (game bit is GAMEBIT_MAZEWELL_ACTIVE) */
#define MUSIC_MAZEWELL 0x36

#define MAZEWELL_DEFAULT_DIALOGUE 1316

/* Row indices into gGmMazeWellQuestBits.watched; rows 0-3 map 1:1 to enum CheatId, rows 4-7
 * grant no cheat, row 8 is the unused/dead 9th token. */
enum QuestWellRow {
    QUESTWELL_CREDITS = 0,       /* ThornTail Shop      -> CHEAT_SHOW_CREDITS */
    QUESTWELL_SEPIA = 1,         /* Cape Claw           -> CHEAT_SEPIA_MODE */
    QUESTWELL_MUSIC_TEST = 2,    /* Ice Mountain        -> CHEAT_MUSIC_TEST */
    QUESTWELL_DINO_LANGUAGE = 3, /* Moon Mtn Pass       -> CHEAT_DINO_LANGUAGE (non-Japanese only) */
    QUESTWELL_LIGHTFOOT = 4,     /* LightFoot Village   -> nothing */
    QUESTWELL_OCEAN_FP = 5,      /* Ocean Force Point   -> nothing */
    QUESTWELL_VOLCANO_FP = 6,    /* Volcano Force Point -> nothing */
    QUESTWELL_SNOWHORN = 7,      /* SnowHorn Wastes     -> nothing */
    QUESTWELL_UNUSED = 8         /* Nowhere - dead 9th token */
};

int GM_MazeWell_SeqFn(GameObject* obj, int unused, ObjSeqState* animUpdate) {
    GmmazewellState* state = obj->extra;
    int i;
    for (i = 0; i < animUpdate->eventCount; i++) {
        if (animUpdate->eventIds[i] == 1 && state->pendingDialogue != -1) {
            (*gGameUIInterface)->showNpcDialogue(state->pendingDialogue, 0x14, 0x8c, 0);
            state->pendingDialogue = -1;
        }
    }
    return 0;
}

int GM_MazeWell_getExtraSize(void) {
    return sizeof(GmmazewellState);
}

void GM_MazeWell_free(void) {
    mainSetBits(GAMEBIT_MAZEWELL_ACTIVE, 0);
    Music_Trigger(MUSIC_MAZEWELL, 0);
}

void GM_MazeWell_render(void* obj, int p2, int p3, int p4, int p5, s8 visible) {
    objRenderModelAndHitVolumes(obj, p2, p3, p4, p5, (double)1.0f);
}

/* The unused ninth row would read the first dialogue halfword as its follow-up
 * bit and the following descriptor's first word as its dialogue ID. Retain the
 * unchecked retail accesses through byte-derived pointers without widening the
 * allocation-backed tables. */
static inline int mazeWellActivate(GameObject* obj, s16* questBits) {
    GmMazeWellQuestTables* tables = (GmMazeWellQuestTables*)questBits;
    GmmazewellState* state;
    int found;
    int itemIndex;
    s16* followup;
    s32* dialogue;
    for (itemIndex = 0;;) {
        if ((*gGameUIInterface)->isItemBeingUsed(questBits[itemIndex]) != 0) {
#if defined(VERSION_GSAP01) || defined(VERSION_GSAP01_rev1)
            state = obj->extra;
            switch (itemIndex) {
            case 0:
            case 1:
            case 2:
                mainSetBits(tables->reward[itemIndex], 1);
                saveFileStruct_unlockCheat((u8)itemIndex);
                break;
            }
            dialogue = (s32*)((u8*)tables + offsetof(GmMazeWellQuestTables, dialogue));
            state->pendingDialogue = dialogue[itemIndex];
            followup = (s16*)((u8*)tables + offsetof(GmMazeWellQuestTables, followup));
            mainSetBits(followup[itemIndex], 1);
#else
            if (gGameTextFontIsSjis != 0) {
                state = obj->extra;
                switch (itemIndex) {
                case 0:
                case 1:
                case 2:
                    mainSetBits(tables->reward[itemIndex], 1);
                    saveFileStruct_unlockCheat((u8)itemIndex);
                    break;
                }
                dialogue = (s32*)((u8*)tables + offsetof(GmMazeWellQuestTables, dialogue));
                state->pendingDialogue = dialogue[itemIndex];
                followup = (s16*)((u8*)tables + offsetof(GmMazeWellQuestTables, followup));
                mainSetBits(followup[itemIndex], 1);
            } else {
                state = obj->extra;
                dialogue = (s32*)((u8*)tables + offsetof(GmMazeWellQuestTables, dialogue));
                state->pendingDialogue = dialogue[itemIndex];
                switch (itemIndex) {
                case 3:
                    state->pendingDialogue = MAZEWELL_DEFAULT_DIALOGUE;
                case 0:
                case 1:
                case 2:
                    mainSetBits(tables->reward[itemIndex], 1);
                    saveFileStruct_unlockCheat((u8)itemIndex);
                    break;
                }
                followup = (s16*)((u8*)tables + offsetof(GmMazeWellQuestTables, followup));
                mainSetBits(followup[itemIndex], 1);
            }
#endif
            found = 1;
            break;
        }
        itemIndex++;
        if ((u32)itemIndex >= QUEST_BIT_COUNT) {
            found = 0;
            break;
        }
    }

    return found;
}
void GM_MazeWell_update(GameObject* obj) {
    GameObject* objId;
    s16* questBits = gGmMazeWellQuestBits.watched;
    GmMazeWellQuestTables* tables = (GmMazeWellQuestTables*)questBits;
    GmmazewellState* state = obj->extra;
    GameObject* player;
    int matchedBit;
    s16* questBitPtr;
    enum QuestWellRow i;

    if (state->savepointSet == 0) {
        player = Obj_GetPlayerObject();
        if (player != 0) {
            (*gMapEventInterface)->savePoint(&player->anim.localPosX, player->anim.rotX, 0, getCurMapLayer());
            state->savepointSet = 1;
        }
    }

    obj->anim.resetHitboxFlags &= ~INTERACT_FLAG_DISABLED;

    for (i = 0, questBitPtr = questBits;;) {
        if (mainGetBit(*questBitPtr) != 0) {
            matchedBit = tables->watched[i];
            break;
        }
        questBitPtr++;
        i++;
        if ((u32)i >= QUEST_BIT_COUNT) {
            matchedBit = 0;
            break;
        }
    }

    if (matchedBit != 0) {
        obj->anim.resetHitboxFlags &= ~INTERACT_FLAG_PROMPT_SUPPRESSED;
    } else {
        obj->anim.resetHitboxFlags |= INTERACT_FLAG_PROMPT_SUPPRESSED;
    }

    objId = obj;
    if ((objId->anim.resetHitboxFlags & INTERACT_FLAG_ACTIVATED) != 0) {
        int found = mazeWellActivate(obj, questBits);
        if (found != 0) {
            (*gObjectTriggerInterface)->runSequence(0, (void*)obj, -1);
            buttonDisable(0, PAD_BUTTON_A);
        }
    }

    objUpdateHitVolumeTransforms(obj);
}

void GM_MazeWell_init(GameObject* obj) {
    GmmazewellState* state = obj->extra;
    state->unk0 = 0;
    mainSetBits(GAMEBIT_MAZEWELL_ACTIVE, 1);
    Music_Trigger(MUSIC_MAZEWELL, 1);
    obj->animEventCallback = GM_MazeWell_SeqFn;
}

GmMazeWellQuestTables gGmMazeWellQuestBits = {
    {0xddc, 0xde2, 0xdde, 0xddd, 0xde0, 0xde3, 0xddf, 0xde1, 0xde4}, {0},
    {0xde5, 0xdeb, 0xde7, 0xde6, 0xde9, 0xdec, 0xde8, 0xdea, 0xded}, {0},
    {0xf34, 0xf3a, 0xf36, 0xf35, 0xf38, 0xf3b, 0xf37, 0xf39},        {1316, 1316, 1316, 1393, 1390, 1391, 1392, 1394}};
ObjectDescriptor gGmMazeWellObjDescriptor = {
    0,
    0,
    0,
    OBJECT_DESCRIPTOR_FLAGS_10_SLOTS,
    0,
    0,
    0,
    (ObjectDescriptorCallback)GM_MazeWell_init,
    (ObjectDescriptorCallback)GM_MazeWell_update,
    0,
    (ObjectDescriptorCallback)GM_MazeWell_render,
    (ObjectDescriptorCallback)GM_MazeWell_free,
    0,
    GM_MazeWell_getExtraSize,
};
