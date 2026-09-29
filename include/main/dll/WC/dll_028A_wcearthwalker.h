#ifndef MAIN_DLL_WC_DLL_028A_WCEARTHWALKER_H
#define MAIN_DLL_WC_DLL_028A_WCEARTHWALKER_H

#include "global.h"
#include "main/objprint_character_api.h"
#include "game/objects/object.h"
#include "game/objects/object_setup.h"
#include "main/objHitReact.h"
#include "main/objseq.h"
#include "main/dll/curve_walker.h"

typedef struct EarthWalkerPlacement {
    ObjPlacement base;
    s8 spawnRot;
    u8 encounterType;
} EarthWalkerPlacement;

STATIC_ASSERT(offsetof(EarthWalkerPlacement, spawnRot) == 0x18);

typedef struct EarthWalkerState {
    u8 pad000[0x600];
    u8 animPhase;
    u8 pad601[0x610 - 0x601];
    u8 hitTriggerId;
    u8 moveLibFlags611;
    u8 pad612[0x624 - 0x612];
    CharacterEyeAnimState eyeAnimState;
    u8 pad64C[0x8];
    f32 hitReactStepScale;
    u8 interactionState;
    u8 flags;
    u8 hitReactState;
    u8 encounterType;
    s8 lastTriggeredState;
    u8 pad65D[0x660 - 0x65D];
} EarthWalkerState;

STATIC_ASSERT(sizeof(EarthWalkerState) == 0x660);
STATIC_ASSERT(offsetof(EarthWalkerState, animPhase) == 0x600);
STATIC_ASSERT(offsetof(EarthWalkerState, hitTriggerId) == 0x610);
STATIC_ASSERT(offsetof(EarthWalkerState, eyeAnimState) == 0x624);
STATIC_ASSERT(offsetof(EarthWalkerState, hitReactStepScale) == 0x654);
STATIC_ASSERT(offsetof(EarthWalkerState, interactionState) == 0x658);
STATIC_ASSERT(offsetof(EarthWalkerState, flags) == 0x659);
STATIC_ASSERT(offsetof(EarthWalkerState, hitReactState) == 0x65A);
STATIC_ASSERT(offsetof(EarthWalkerState, encounterType) == 0x65B);
STATIC_ASSERT(offsetof(EarthWalkerState, lastTriggeredState) == 0x65C);

extern ObjHitReactEntry gEarthWalkerHitReactEntries[];

int earthwalker_getExtraSize(void);
int earthwalker_getObjectTypeId(void);
void earthwalker_free(void);
void earthwalker_render(GameObject* obj, int p2, int p3, int p4, int p5, s8 visible);
void earthwalker_hitDetect(GameObject* obj);
void earthwalker_release(void);
void earthwalker_initialise(void);
void earthwalker_update(GameObject* obj);
int earthwalker_SeqFn(GameObject* ewObj, int unused, ObjSeqState* animUpdate, int shouldAdvanceMove);
void earthwalker_init(GameObject* obj, EarthWalkerPlacement* setup);

#endif
