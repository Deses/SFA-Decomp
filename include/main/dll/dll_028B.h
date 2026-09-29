#ifndef MAIN_DLL_DLL_028B_H
#define MAIN_DLL_DLL_028B_H

#include "global.h"
#include "main/objprint_character_api.h"
#include "main/dll/curve_walker.h"
#include "main/dll/baddie_state.h"
#include "main/dll/dll_002E_moveLib.h"
#include "game/objects/object.h"

typedef struct Dll28BState
{
    BaddieState baddie;
    MoveLibState moveLib;
    CharacterEyeAnimState eyeAnimState;
    u8 pad9A8[0x9B0 - 0x9A8];
    RomCurveWalker route;
    f32 playerDistance;
    f32 randomTimer;
    u8 flagsAC0;
    u8 padAC1[0xAC4 - 0xAC1];
} Dll28BState;

typedef struct Dll28BMoveBlendData
{
    int values[4];
} Dll28BMoveBlendData;

STATIC_ASSERT(sizeof(Dll28BState) == 0xAC4);
STATIC_ASSERT(offsetof(Dll28BState, moveLib) == 0x35C);
STATIC_ASSERT(offsetof(Dll28BState, eyeAnimState) == 0x980);
STATIC_ASSERT(offsetof(Dll28BState, route) == 0x9B0);
STATIC_ASSERT(offsetof(Dll28BState, playerDistance) == 0xAB8);
STATIC_ASSERT(offsetof(Dll28BState, route.posX) == 0xA18);
STATIC_ASSERT(offsetof(Dll28BState, randomTimer) == 0xABC);
STATIC_ASSERT(offsetof(Dll28BState, flagsAC0) == 0xAC0);
STATIC_ASSERT(sizeof(Dll28BMoveBlendData) == 0x10);

extern const Dll28BMoveBlendData gDll28BMoveBlendDataA;
extern const Dll28BMoveBlendData gDll28BMoveBlendDataB;
extern void* gDll28BSubstateHandlers[4];
extern void* gDll28BStateHandlers[4];


int dll_28B_getExtraSize(void);
int dll_28B_getObjectTypeId(void);
void dll_28B_free(GameObject* obj);
void dll_28B_render(GameObject* obj, int p2, int p3, int p4, int p5, s8 visible);
void dll_28B_hitDetect_nop(void);
void dll_28B_update(GameObject* obj);
void dll_28B_init(GameObject* obj);
void dll_28B_release_nop(void);
void dll_28B_initialise(void);


int dll_28B_substateHandler0(void);
int dll_28B_stateHandler0(void);
int dll_28B_substateHandler3(GameObject* obj, struct BaddieState* ai);
int dll_28B_substateHandler2(GameObject* obj, struct BaddieState* ai);
int dll_28B_substateHandler1(GameObject* obj, struct BaddieState* ai);
int dll_28B_stateHandler3(GameObject* obj, struct BaddieState* ai);
int dll_28B_stateHandler2(GameObject* obj, struct BaddieState* ai);
int dll_28B_stateHandler1(GameObject* obj, struct BaddieState* ai);

#endif
