/*
 * DRMusicCont (DLL 638) - an invisible music/ambience controller for
 * its map.
 *
 * update overrides the cloud position, runs a one-shot env-fx and sky
 * setup the first frame, and drives several game-bit latches
 * (GameBitLatch_*). It watches two quads of "switch" game bits and a
 * third quad: completing the first two quads sets a progress bit and
 * plays a stinger, any change within a quad plays a mutter cue, and a
 * change in the third quad arms a short countdown (unk4) that fires a
 * one-shot sfx on expiry. It also toggles a map restart point based on
 * two more game bits. State: a f32 countdown at 0x4 and the
 * DrMusicContFlags cache at 0x8.
 */
#include "main/audio/sfx_play_api.h"
#include "main/audio/music_trigger_ids.h"
#include "main/frame_timing.h"
#include "main/gamebits.h"
#include "main/mapEventTypes.h"
#include "main/render_envfx_api.h"
#include "main/newclouds.h"
#include "main/sky_api.h"
#include "main/audio/sfx_trigger_ids.h"

#include "main/dll/DR/dll_027E_drmusiccont.h"
#include "main/object_render.h"

#define DRMUSICCONT_CLOUD_OVERRIDE_POS_X -15350.0f
#define DRMUSICCONT_CLOUD_OVERRIDE_POS_Y -1550.0f
#define DRMUSICCONT_CLOUD_OVERRIDE_POS_Z 10875.0f
#define DRMUSICCONT_STINGER_TIMER_DURATION 60.0f
#define DRMUSICCONT_RESTART_POINT_X -15697.0f
#define DRMUSICCONT_RESTART_POINT_Y -1501.0f
#define DRMUSICCONT_RESTART_POINT_Z 12928.0f

#define DRMUSICCONT_ENVFX_A 0x210
#define DRMUSICCONT_ENVFX_B 0x20f
#define DRMUSICCONT_ENVFX_C 0x212
#define DRMUSICCONT_ENVFX_D 0x1ea

int drmusiccont_getExtraSize(void)
{
    return 4;
}

int drmusiccont_getObjectTypeId(void)
{
    return 0;
}

void drmusiccont_free(int obj)
{
    cloudClearOverridePosition();
}

void drmusiccont_render(GameObject* obj, int p2, int p3, int p4, int p5, s8 visible)
{
    if (visible != 0)
    {
        objRenderModelAndHitVolumes(obj, p2, p3, p4, p5, 1.0f);
    }
}

void drmusiccont_hitDetect(void)
{
}

void drmusiccont_update(GameObject* obj)
{
    DrmusiccontState* state = obj->extra;
    DrMusicContFlags* flags = &state->flags;
    u8 generator1;
    u8 generator2;
    u8 generator3;
    u8 generator4;
    u8 robot1;
    u8 robot2;
    u8 robot3;
    u8 robot4;
    u8 towerSwitch1;
    u8 towerSwitch2;
    u8 towerSwitch3;
    u8 towerSwitch4;

    cloudSetOverridePosition(DRMUSICCONT_CLOUD_OVERRIDE_POS_X, DRMUSICCONT_CLOUD_OVERRIDE_POS_Y,
                             DRMUSICCONT_CLOUD_OVERRIDE_POS_Z);
    if (obj->userData1 == 0) {
        if (mainGetBit(GAMEBIT_DRArwingRelated0E7B) == 0)
        {
            getEnvfxActImmediately(obj, obj, DRMUSICCONT_ENVFX_A, 0);
            getEnvfxActImmediately(obj, obj, DRMUSICCONT_ENVFX_B, 0);
            getEnvfxActImmediately(obj, obj, DRMUSICCONT_ENVFX_C, 0);
            getEnvfxActImmediately(obj, obj, DRMUSICCONT_ENVFX_D, 0);
            skySetLightIndex(0, 0.0f);
            mainSetBits(GAMEBIT_DRArwingRelated0E7B, 1);
        }
        obj->userData1 = 1;
    }

    GameBitLatch_Update(&state->gameBitLatch, 2, 0x1a7, 0x64b, 0xf0e, 0xe5);
    GameBitLatch_UpdateInverted(&state->gameBitLatch, 1, -1, -1, 0xe26, 0xb8);
    GameBitLatch_Update(&state->gameBitLatch, 4, -1, -1, GAMEBIT_SHRINE_MUSIC_LOCK,
                        MUSICTRIG_PU3_Adventure_c4);

    generator1 = (u8)mainGetBit(GAMEBIT_DR_RobotGenerator1Destroyed);
    generator2 = (u8)mainGetBit(GAMEBIT_DR_RobotGenerator2Destroyed);
    generator3 = (u8)mainGetBit(GAMEBIT_DR_RobotGenerator3Destroyed);
    generator4 = (u8)mainGetBit(GAMEBIT_DR_RobotGenerator4Destroyed);
    if (flags->shieldsDown == 0 && generator1 && generator2 && generator3 && generator4)
    {
        flags->shieldsDown = 1;
        mainSetBits(GAMEBIT_DR_ShutDownRobotShields, 1);
        Sfx_PlayFromObject(0, SFXTRIG_mpick1_b);
    }
    else if (generator1 != flags->prevGenerator1 || generator2 != flags->prevGenerator2 || generator3 != flags->prevGenerator3 || generator4 != flags->prevGenerator4)
    {
        Sfx_PlayFromObject(0, SFXTRIG_menuups16k);
    }
    flags->prevGenerator1 = generator1;
    flags->prevGenerator2 = generator2;
    flags->prevGenerator3 = generator3;
    flags->prevGenerator4 = generator4;

    robot1 = (u8)mainGetBit(GAMEBIT_DR_Robot1Destroyed);
    robot2 = (u8)mainGetBit(GAMEBIT_DR_Robot2Destroyed);
    robot3 = (u8)mainGetBit(GAMEBIT_DR_Robot3Destroyed);
    robot4 = (u8)mainGetBit(GAMEBIT_DR_Robot4Destroyed);
    if (flags->robotsChimePlayed == 0 && robot1 && robot2 && robot3 && robot4)
    {
        flags->robotsChimePlayed = 1;
        Sfx_PlayFromObject(0, SFXTRIG_mpick1_b);
    }
    else if (robot1 != flags->prevRobot1 || robot2 != flags->prevRobot2 || robot3 != flags->prevRobot3 || robot4 != flags->prevRobot4)
    {
        Sfx_PlayFromObject(0, SFXTRIG_menuups16k);
    }
    flags->prevRobot1 = robot1;
    flags->prevRobot2 = robot2;
    flags->prevRobot3 = robot3;
    flags->prevRobot4 = robot4;

    towerSwitch1 = (u8)mainGetBit(GAMEBIT_DR_TowerSwitch1);
    towerSwitch2 = (u8)mainGetBit(GAMEBIT_DR_TowerSwitch2);
    towerSwitch3 = (u8)mainGetBit(GAMEBIT_DR_TowerSwitch3);
    towerSwitch4 = (u8)mainGetBit(GAMEBIT_DR_TowerSwitch4);
    if (!(towerSwitch1 && towerSwitch2 && towerSwitch3 && towerSwitch4))
    {
        if (towerSwitch1 != flags->prevTowerSwitch1 || towerSwitch2 != flags->prevTowerSwitch2 || towerSwitch3 != flags->prevTowerSwitch3 || towerSwitch4 != flags->prevTowerSwitch4)
        {
            state->stingerTimer = DRMUSICCONT_STINGER_TIMER_DURATION;
        }
    }
    {
        f32 st = state->stingerTimer;
        f32 zero = 0.0f;
        if (st > zero)
        {
            state->stingerTimer = st - timeDelta;
            if (state->stingerTimer <= zero)
            {
                Sfx_PlayFromObject(0, SFXTRIG_sc_menuups16k_4bd);
            }
        }
    }
    flags->prevTowerSwitch1 = towerSwitch1;
    flags->prevTowerSwitch2 = towerSwitch2;
    flags->prevTowerSwitch3 = towerSwitch3;
    flags->prevTowerSwitch4 = towerSwitch4;

    if (flags->restartPointSet != 0)
    {
        if (mainGetBit(GAMEBIT_DR_HighTopRestartArmed) == 0 || mainGetBit(GAMEBIT_DR_RescuedHighTop) != 0)
        {
            (*gMapEventInterface)->clearRestartPoint();
            flags->restartPointSet = 0;
        }
    }
    else
    {
        if (mainGetBit(GAMEBIT_DR_HighTopRestartArmed) != 0 && mainGetBit(GAMEBIT_DR_RescuedHighTop) == 0)
        {
            f32 vec[3];
            vec[0] = DRMUSICCONT_RESTART_POINT_X;
            vec[1] = DRMUSICCONT_RESTART_POINT_Y;
            vec[2] = DRMUSICCONT_RESTART_POINT_Z;
            (*gMapEventInterface)->restartPoint(vec, 0x7fff, 0, 0);
            flags->restartPointSet = 1;
        }
    }
}

void drmusiccont_init(GameObject* obj)
{
    DrmusiccontState* state = obj->extra;
    DrMusicContFlags* flags = &state->flags;

    flags->prevGenerator1 = mainGetBit(GAMEBIT_DR_RobotGenerator1Destroyed);
    flags->prevGenerator2 = mainGetBit(GAMEBIT_DR_RobotGenerator2Destroyed);
    flags->prevGenerator3 = mainGetBit(GAMEBIT_DR_RobotGenerator3Destroyed);
    flags->prevGenerator4 = mainGetBit(GAMEBIT_DR_RobotGenerator4Destroyed);
    flags->shieldsDown = mainGetBit(GAMEBIT_DR_ShutDownRobotShields);
    flags->prevRobot1 = mainGetBit(GAMEBIT_DR_Robot1Destroyed);
    flags->prevRobot2 = mainGetBit(GAMEBIT_DR_Robot2Destroyed);
    flags->prevRobot3 = mainGetBit(GAMEBIT_DR_Robot3Destroyed);
    flags->prevRobot4 = mainGetBit(GAMEBIT_DR_Robot4Destroyed);
    flags->robotsChimePlayed = mainGetBit(GAMEBIT_DR_RobotsDestroyedChimePlayed);
    flags->prevTowerSwitch1 = mainGetBit(GAMEBIT_DR_TowerSwitch1);
    flags->prevTowerSwitch2 = mainGetBit(GAMEBIT_DR_TowerSwitch2);
    flags->prevTowerSwitch3 = mainGetBit(GAMEBIT_DR_TowerSwitch3);
    flags->prevTowerSwitch4 = mainGetBit(GAMEBIT_DR_TowerSwitch4);
}

void drmusiccont_release(void)
{
}

void drmusiccont_initialise(void)
{
}

ObjectDescriptor gDrMusicContObjDescriptor = {
    0,
    0,
    0,
    OBJECT_DESCRIPTOR_FLAGS_10_SLOTS,
    (ObjectDescriptorCallback)drmusiccont_initialise,
    (ObjectDescriptorCallback)drmusiccont_release,
    0,
    (ObjectDescriptorCallback)drmusiccont_init,
    (ObjectDescriptorCallback)drmusiccont_update,
    (ObjectDescriptorCallback)drmusiccont_hitDetect,
    (ObjectDescriptorCallback)drmusiccont_render,
    (ObjectDescriptorCallback)drmusiccont_free,
    (ObjectDescriptorCallback)drmusiccont_getObjectTypeId,
    (ObjectDescriptorExtraSizeCallback)drmusiccont_getExtraSize,
};
