#ifndef MAIN_DLL_DR_DLL_027E_DRMUSICCONT_H_
#define MAIN_DLL_DR_DLL_027E_DRMUSICCONT_H_

#include "game/objects/object.h"
#include "dlls/objects/430_SH_LevelCon.h"
#include "global.h"


typedef struct DrMusicContFlags
{
    u8 restartPointSet : 1;
    u8 pad8_lo : 1;
    u8 prevGenerator1 : 1;
    u8 prevGenerator2 : 1;
    u8 prevGenerator3 : 1;
    u8 prevGenerator4 : 1;
    u8 shieldsDown : 1;
    u8 prevRobot1 : 1;
    u8 prevRobot2 : 1;
    u8 prevRobot3 : 1;
    u8 prevRobot4 : 1;
    u8 robotsChimePlayed : 1;
    u8 prevTowerSwitch1 : 1;
    u8 prevTowerSwitch2 : 1;
    u8 prevTowerSwitch3 : 1;
    u8 prevTowerSwitch4 : 1;
} DrMusicContFlags;

typedef struct DrmusiccontState
{
    GameBitLatchState gameBitLatch;
    f32 stingerTimer;
    DrMusicContFlags flags;
} DrmusiccontState;

STATIC_ASSERT(offsetof(DrmusiccontState, gameBitLatch) == 0x0);
STATIC_ASSERT(offsetof(DrmusiccontState, stingerTimer) == 0x4);
STATIC_ASSERT(offsetof(DrmusiccontState, flags) == 0x8);

int drmusiccont_getExtraSize(void);
int drmusiccont_getObjectTypeId(void);
void drmusiccont_free(int obj);
void drmusiccont_render(GameObject* obj, int p2, int p3, int p4, int p5, s8 visible);
void drmusiccont_hitDetect(void);
void drmusiccont_release(void);
void drmusiccont_initialise(void);
void drmusiccont_init(GameObject* obj);
void drmusiccont_update(GameObject* obj);

#endif /* MAIN_DLL_DR_DLL_027E_DRMUSICCONT_H_ */
