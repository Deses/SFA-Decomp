#ifndef MAIN_DLL_WC_DLL_028C_WCBOUNCYCRA_H
#define MAIN_DLL_WC_DLL_028C_WCBOUNCYCRA_H

#include "game/objects/object.h"
#include "game/objects/object_setup.h"

typedef struct WCBouncyCrateState {
    f32 homeY;
    u8 pad04[4];
    s16 cooldown;
    u8 flags;
    u8 bounceCount;
} WCBouncyCrateState;

STATIC_ASSERT(sizeof(WCBouncyCrateState) == 0x0C);
STATIC_ASSERT(offsetof(WCBouncyCrateState, cooldown) == 0x08);
STATIC_ASSERT(offsetof(WCBouncyCrateState, flags) == 0x0A);
STATIC_ASSERT(offsetof(WCBouncyCrateState, bounceCount) == 0x0B);

int WCBouncyCra_getExtraSize(void);
int WCBouncyCra_getObjectTypeId(void);
void WCBouncyCra_free(void);
void WCBouncyCra_render(GameObject* obj, int p2, int p3, int p4, int p5, s8 visible);
void WCBouncyCra_hitDetect(void);
void WCBouncyCra_update(GameObject* obj);
void WCBouncyCra_init(GameObject* obj, ObjPlacement* setup);
void WCBouncyCra_release(void);
void WCBouncyCra_initialise(void);

#endif
