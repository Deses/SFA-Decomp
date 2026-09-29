/*
 * WCPushBlock (DLL 656) - the sliding push-block puzzle object in the
 * Walled City (WC). Two block variants (anim.bankIndex: VARIANT_A vs B)
 * ride a shared tile grid owned by a separate level-controller object,
 * found via objGetNearestTypeTo on controller group
 * WCPUSHBLOCK_CONTROLLER_GROUP; the controller's WCLevelContInterface
 * (vtable at controller+0x68) does all tile<->world mapping, move tracing
 * and tile-occupancy writes (A/B method pairs).
 *
 * Per-frame phase machine: INIT_MOVE places the block at its initial tile;
 * IDLE fades in, polls the player push and traces a move; SLIDING eases
 * toward the target tile with a looped sliding sfx and clamps; FADE_OUT/
 * FADE_IN reset to the initial cell when a move is rejected; LOCKED/SOLVED
 * swap to the locked texture. A vertical bob is applied each frame. Per-
 * variant solved/fade/count game bits drive the puzzle; reaching
 * WCPUSHBLOCK_REQUIRED_LOCK_COUNT latches the solved bit.
 *
 * The WC level controller's own act-1/act-2 mode machines (timers, save
 * points, map gating, music) live in WCLevelCont (DLL 653)
 * (wclevelcont_updateAct1State / wclevelcont_updateAct2State), not here.
 * Offsets/bit values inferred from code.
 */
#include "main/dll/WC/dll_0290_wcpushblock.h"

#include "MSL_C/PPCEABI/bare/H/math_api.h"
#include "main/frame_timing.h"
#include "main/gamebit_ids.h"
#include "main/gamebits.h"
#include "main/gameloop_gamebit_api.h"
#include "main/mapEventTypes.h"
#include "main/objhits.h"
#include "main/objtexture.h"
#include "main/objtype.h"
#include "sys/objects.h"
#include "main/dll/WC/dll_028D_wclevelcont.h"
#include "main/dll/player_api.h"
#include "game/objects/object.h"
#include "main/objfx.h"
#include "main/audio/sfx_ids.h"
#include "main/audio/sfx_trigger_ids.h"
#include "main/audio/sfx.h"
#include "main/object_render.h"
#include "dlls/object_descriptor.h"
#include "main/audio/sfx_keep_alive_api.h"
#include "main/audio/sfx_play_api.h"

#define WCPUSHBLOCK_EXTRA_SIZE          0x288
#define WCPUSHBLOCK_RENDER_TYPE_BASE    0x400
#define WCPUSHBLOCK_RENDER_TYPE_SHIFT   0xb
#define WCPUSHBLOCK_CONTROLLER_GROUP    9
#define WCPUSHBLOCK_MODEL_INDEX_OFFSET  0x19
#define WCPUSHBLOCK_INITIAL_TILE_OFFSET 0x1a

#define WCPUSHBLOCK_STATE_TARGET_X     0x26c
#define WCPUSHBLOCK_STATE_TARGET_Z     0x270
#define WCPUSHBLOCK_STATE_BASE_Y       0x274
#define WCPUSHBLOCK_STATE_BOB_Y        0x278
#define WCPUSHBLOCK_STATE_BOB_ANGLE    0x27c
#define WCPUSHBLOCK_STATE_TILE_X       0x27e
#define WCPUSHBLOCK_STATE_TILE_Y       0x280
#define WCPUSHBLOCK_STATE_PUSH_DIR     0x282
#define WCPUSHBLOCK_STATE_INITIAL_TILE 0x283
#define WCPUSHBLOCK_STATE_MOVE_RESULT  0x284
#define WCPUSHBLOCK_STATE_FLAGS        0x285
#define WCPUSHBLOCK_STATE_CONTROLLER   0x268

#define WCPUSHBLOCK_PHASE_INIT_MOVE 0
#define WCPUSHBLOCK_PHASE_IDLE      1
#define WCPUSHBLOCK_PHASE_SLIDING   2
#define WCPUSHBLOCK_PHASE_FADE_OUT  3
#define WCPUSHBLOCK_PHASE_LOCKED    4
#define WCPUSHBLOCK_PHASE_FADE_IN   5
#define WCPUSHBLOCK_PHASE_SOLVED    6

#define WCPUSHBLOCK_VARIANT_A           1
#define WCPUSHBLOCK_ALPHA_STEP_SHIFT    3
#define WCPUSHBLOCK_ALPHA_OPAQUE        0xff
#define WCPUSHBLOCK_TEXTURE_DEFAULT     0
#define WCPUSHBLOCK_TEXTURE_LOCKED      0x100
#define WCPUSHBLOCK_OBJFLAG_LOCKED      0x100
#define WCPUSHBLOCK_BOX_BURST_VARIANT_A 3
#define WCPUSHBLOCK_BOX_BURST_VARIANT_B 1

#define WCPUSHBLOCK_DIR_POS_X            0
#define WCPUSHBLOCK_DIR_NEG_X            1
#define WCPUSHBLOCK_DIR_POS_Z            2
#define WCPUSHBLOCK_DIR_NEG_Z            3
#define WCPUSHBLOCK_MOVE_RESULT_CONTINUE 1
#define WCPUSHBLOCK_MOVE_RESULT_LOCKED   2

#define WCPUSHBLOCK_REQUIRED_LOCK_COUNT 4U

#define WCPUSHBLOCK_CONTROLLER(state)   (((WCPushBlockRuntimeState*)(state))->block.controller)
#define WCPUSHBLOCK_IFACE               WC_LEVEL_CONT_INTERFACE(WCPUSHBLOCK_CONTROLLER(state))
#define WCPUSHBLOCK_TARGET_X(state)     (((WCPushBlockRuntimeState*)(state))->block.targetX)
#define WCPUSHBLOCK_TARGET_Z(state)     (((WCPushBlockRuntimeState*)(state))->block.targetZ)
#define WCPUSHBLOCK_BASE_Y(state)       (((WCPushBlockRuntimeState*)(state))->block.baseY)
#define WCPUSHBLOCK_BOB_Y(state)        (((WCPushBlockRuntimeState*)(state))->block.bobY)
#define WCPUSHBLOCK_BOB_ANGLE(state)    (((WCPushBlockRuntimeState*)(state))->block.bobAngle)
#define WCPUSHBLOCK_TILE_X(state)       (((WCPushBlockRuntimeState*)(state))->block.cellX)
#define WCPUSHBLOCK_TILE_Y(state)       (((WCPushBlockRuntimeState*)(state))->block.cellZ)
#define WCPUSHBLOCK_PUSH_DIR(state)     (((WCPushBlockRuntimeState*)(state))->block.pushDir)
#define WCPUSHBLOCK_INITIAL_TILE(state) (((WCPushBlockRuntimeState*)(state))->block.tileIndex)
#define WCPUSHBLOCK_MOVE_RESULT(state)  (((WCPushBlockRuntimeState*)(state))->moveResult)
#define WCPUSHBLOCK_FLAGS(state)        (((WCPushBlockRuntimeState*)(state))->flags)

/* Address reads preserve the named read-only pool without duplicate literals. */
const f32 WCBLOCK_PLAYER_CELL_MARGIN = 56.0f;

int wcblock_isPlayerAwayFromStoredCell(GameObject* obj, WCBlockState* state, GameObject* player) {
    ObjAnimComponent* objAnim;
    GameObject* playerObj;
    f32 cellX;
    f32 cellZ;
    f32 pos;
    f32 min;
    f32 max;
    WCLevelContInterface* iface;

    objAnim = &obj->anim;
    if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
        iface->getInitialTileXYA(state->tileIndex, &state->cellX, &state->cellZ,
                                 (iface = WC_LEVEL_CONT_INTERFACE(state->controller)));
        iface->tileAToWorldPos(obj, state->cellX, state->cellZ, &cellX, &cellZ,
                               (iface = WC_LEVEL_CONT_INTERFACE(state->controller)));
    } else {
        iface->getInitialTileXYB(state->tileIndex, &state->cellX, &state->cellZ,
                                 (iface = WC_LEVEL_CONT_INTERFACE(state->controller)));
        iface->tileBToWorldPos(obj, state->cellX, state->cellZ, &cellX, &cellZ,
                               (iface = WC_LEVEL_CONT_INTERFACE(state->controller)));
    }

    min = cellX - (*(const f32*)&WCBLOCK_PLAYER_CELL_MARGIN);
    playerObj = player;
    pos = playerObj->anim.localPosX;
    max = (*(const f32*)&WCBLOCK_PLAYER_CELL_MARGIN) + cellX;
    if (pos > max || pos < min) {
        return 1;
    }

    {
        f32 posZ;
        f32 minZ;
        f32 maxZ;

        minZ = cellZ - (*(const f32*)&WCBLOCK_PLAYER_CELL_MARGIN);
        posZ = playerObj->anim.localPosZ;
        maxZ = (*(const f32*)&WCBLOCK_PLAYER_CELL_MARGIN) + cellZ;
        if (posZ > maxZ || posZ < minZ) {
            return 1;
        }
    }

    return 0;
}

const f32 gWcPushBlockOne = 1.0f;
const f32 gWcPushBlockControllerSearchRange = 100000.0f;
const f32 gWcPushBlockBurstScale = 2.0f;
const f32 gWcPushBlockBurstHorizontalExtent = 65.0f;
const f32 gWcPushBlockZero = 0.0f;
const f32 gWcPushBlockSlideSfxSpeedThreshold = 0.25f;
const f32 gWcPushBlockSlideSfxVolumeRange = 126.0f;
const f32 gWcPushBlockSlideSfxMaxSpeed = 1.25f;
const f32 gWcPushBlockSlideSfxMaxVolume = 127.0f;
const f32 gWcPushBlockSlideSfxVolumeScale = 0.5f;
const f32 gWcPushBlockMaxSlideSpeed = 1.5f;
const f32 gWcPushBlockSlideAccel = 0.05f;
const f32 gWcPushBlockMinSlideSpeed = -1.5f;
const f32 gWcPushBlockBobAngleSpeed = 250.0f;
const f32 gWcPushBlockBobAmplitude = 3.0f;
const f32 gWcPushBlockPi = 3.1415927f;
const f32 gWcPushBlockAngleScale = 32768.0f;
int wcpushblock_getExtraSize(void) {
    return sizeof(WCPushBlockRuntimeState);
}

int wcpushblock_getObjectTypeId(GameObject* obj) {
    ObjAnimComponent* objAnim = &obj->anim;
    int modelIndex = ((WCPushBlockSetup*)obj->anim.placementData)->modelIndex;
    int modelCount = objAnim->modelInstance->modelCount;

    if (modelIndex >= modelCount) {
        modelIndex = 0;
    }
    return (modelIndex << WCPUSHBLOCK_RENDER_TYPE_SHIFT) | WCPUSHBLOCK_RENDER_TYPE_BASE;
}

void wcpushblock_free(void) {
}

void wcpushblock_render(GameObject* obj, int p2, int p3, int p4, int p5, s8 visible) {
    if (visible != 0) {
        objRenderModelAndHitVolumes(obj, p2, p3, p4, p5, (*(const f32*)&gWcPushBlockOne));
    }
}

void wcpushblock_hitDetect(void) {
}

ObjectDescriptor gWCPushBlockObjDescriptor = {
    0,
    0,
    0,
    OBJECT_DESCRIPTOR_FLAGS_10_SLOTS,
    (ObjectDescriptorCallback)wcpushblock_initialise,
    (ObjectDescriptorCallback)wcpushblock_release,
    0,
    (ObjectDescriptorCallback)wcpushblock_init,
    (ObjectDescriptorCallback)wcpushblock_update,
    (ObjectDescriptorCallback)wcpushblock_hitDetect,
    (ObjectDescriptorCallback)wcpushblock_render,
    (ObjectDescriptorCallback)wcpushblock_free,
    (ObjectDescriptorCallback)wcpushblock_getObjectTypeId,
    (ObjectDescriptorExtraSizeCallback)wcpushblock_getExtraSize,
};

void wcpushblock_update(GameObject* obj) {

    f32 dt;
    ObjAnimComponent* objAnim = &obj->anim;
    WCPushBlockRuntimeState* state = obj->extra;
    GameObject* player = (GameObject*)Obj_GetPlayerObject();
    f32 range = (*(const f32*)&gWcPushBlockControllerSearchRange);
    f32 sfxVolume;
    ObjTextureRuntimeSlot* tex;
    int reachedTarget;

    if ((void*)WCPUSHBLOCK_CONTROLLER(state) == 0) {
        WCPUSHBLOCK_CONTROLLER(state) = (GameObject*)objGetNearestTypeTo(WCPUSHBLOCK_CONTROLLER_GROUP, obj, &range);
        objAnim->alpha = 0;
        return;
    }
    tex = objFindTexture(obj, 0, 0);
    if (tex != 0) {
        tex->textureId = WCPUSHBLOCK_TEXTURE_DEFAULT;
    }
    obj->objectFlags &= ~WCPUSHBLOCK_OBJFLAG_LOCKED;

    if (WCPUSHBLOCK_FLAGS(state).phase != WCPUSHBLOCK_PHASE_SOLVED) {
        if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
            if (mainGetBit(GAMEBIT_WC_PushBlockASolved) != 0) {
                WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_SOLVED;
                WCPUSHBLOCK_IFACE->getSolvedTileXYA(WCPUSHBLOCK_INITIAL_TILE(state), &state->block.cellX,
                                                    &state->block.cellZ, WCPUSHBLOCK_IFACE);
                WCPUSHBLOCK_IFACE->tileAToWorldPos(obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state),
                                                   &obj->anim.localPosX, &obj->anim.localPosZ, WCPUSHBLOCK_IFACE);
            } else if (mainGetBit(GAMEBIT_WC_PushBlockAFade) != 0) {
                WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_FADE_OUT;
            }
        } else {
            if (mainGetBit(GAMEBIT_WC_PushBlockBSolved) != 0) {
                WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_SOLVED;
                WCPUSHBLOCK_IFACE->getSolvedTileXYB(WCPUSHBLOCK_INITIAL_TILE(state), &state->block.cellX,
                                                    &state->block.cellZ, WCPUSHBLOCK_IFACE);
                WCPUSHBLOCK_IFACE->tileBToWorldPos(obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state),
                                                   &obj->anim.localPosX, &obj->anim.localPosZ, WCPUSHBLOCK_IFACE);
            } else if (mainGetBit(GAMEBIT_WC_PushBlockBFade) != 0) {
                WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_FADE_OUT;
            }
        }
    }

    {
        u32 phase = WCPUSHBLOCK_FLAGS(state).phase;
        if (phase != WCPUSHBLOCK_PHASE_FADE_OUT && phase != WCPUSHBLOCK_PHASE_FADE_IN) {
            if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
                objfx_spawnBoxBurst(obj, 1, (*(const f32*)&gWcPushBlockBurstScale), WCPUSHBLOCK_BOX_BURST_VARIANT_A, 1,
                                    50, (*(const f32*)&gWcPushBlockBurstHorizontalExtent),
                                    (*(const f32*)&gWcPushBlockBurstScale),
                                    (*(const f32*)&gWcPushBlockBurstHorizontalExtent), NULL, 0);
            } else {
                objfx_spawnBoxBurst(obj, 1, (*(const f32*)&gWcPushBlockBurstScale), WCPUSHBLOCK_BOX_BURST_VARIANT_B, 1,
                                    50, (*(const f32*)&gWcPushBlockBurstHorizontalExtent),
                                    (*(const f32*)&gWcPushBlockBurstScale),
                                    (*(const f32*)&gWcPushBlockBurstHorizontalExtent), NULL, 0);
            }
        }
    }

    switch (WCPUSHBLOCK_FLAGS(state).phase) {
    case WCPUSHBLOCK_PHASE_INIT_MOVE:
        if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
            WCPUSHBLOCK_IFACE->getInitialTileXYA(WCPUSHBLOCK_INITIAL_TILE(state), &state->block.cellX,
                                                 &state->block.cellZ, WCPUSHBLOCK_IFACE);
            WCPUSHBLOCK_IFACE->tileAToWorldPos(obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state),
                                               &obj->anim.localPosX, &obj->anim.localPosZ, WCPUSHBLOCK_IFACE);
        } else {
            WCPUSHBLOCK_IFACE->getInitialTileXYB(WCPUSHBLOCK_INITIAL_TILE(state), &state->block.cellX,
                                                 &state->block.cellZ, WCPUSHBLOCK_IFACE);
            WCPUSHBLOCK_IFACE->tileBToWorldPos(obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state),
                                               &obj->anim.localPosX, &obj->anim.localPosZ, WCPUSHBLOCK_IFACE);
        }
        WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_IDLE;
        break;
    case WCPUSHBLOCK_PHASE_IDLE: {
        int a = objAnim->alpha + framesThisStep * 8;
        if (a > WCPUSHBLOCK_ALPHA_OPAQUE) {
            a = WCPUSHBLOCK_ALPHA_OPAQUE;
        }
        objAnim->alpha = a;
    }
        {
            f32 zero = (*(const f32*)&gWcPushBlockZero);
            obj->anim.velocityX = zero;
            obj->anim.velocityZ = zero;
        }
        if (playerIsPushingObject(player, obj, &state->block.pushDir) != 0) {
            if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
                if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_POS_X) {
                    WCPUSHBLOCK_MOVE_RESULT(state) = WCPUSHBLOCK_IFACE->traceMoveA(
                        obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), &state->block.targetX,
                        &state->block.targetZ, -1, 0, WCPUSHBLOCK_IFACE);
                } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_NEG_X) {
                    WCPUSHBLOCK_MOVE_RESULT(state) = WCPUSHBLOCK_IFACE->traceMoveA(
                        obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), &state->block.targetX,
                        &state->block.targetZ, 1, 0, WCPUSHBLOCK_IFACE);
                } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_POS_Z) {
                    WCPUSHBLOCK_MOVE_RESULT(state) = WCPUSHBLOCK_IFACE->traceMoveA(
                        obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), &state->block.targetX,
                        &state->block.targetZ, 0, -1, WCPUSHBLOCK_IFACE);
                } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_NEG_Z) {
                    WCPUSHBLOCK_MOVE_RESULT(state) = WCPUSHBLOCK_IFACE->traceMoveA(
                        obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), &state->block.targetX,
                        &state->block.targetZ, 0, 1, WCPUSHBLOCK_IFACE);
                }
            } else {
                if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_POS_X) {
                    WCPUSHBLOCK_MOVE_RESULT(state) = WCPUSHBLOCK_IFACE->traceMoveB(
                        obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), &state->block.targetX,
                        &state->block.targetZ, -1, 0, WCPUSHBLOCK_IFACE);
                } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_NEG_X) {
                    WCPUSHBLOCK_MOVE_RESULT(state) = WCPUSHBLOCK_IFACE->traceMoveB(
                        obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), &state->block.targetX,
                        &state->block.targetZ, 1, 0, WCPUSHBLOCK_IFACE);
                } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_POS_Z) {
                    WCPUSHBLOCK_MOVE_RESULT(state) = WCPUSHBLOCK_IFACE->traceMoveB(
                        obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), &state->block.targetX,
                        &state->block.targetZ, 0, -1, WCPUSHBLOCK_IFACE);
                } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_NEG_Z) {
                    WCPUSHBLOCK_MOVE_RESULT(state) = WCPUSHBLOCK_IFACE->traceMoveB(
                        obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), &state->block.targetX,
                        &state->block.targetZ, 0, 1, WCPUSHBLOCK_IFACE);
                }
            }
            if (WCPUSHBLOCK_TARGET_X(state) == obj->anim.localPosX &&
                WCPUSHBLOCK_TARGET_Z(state) == obj->anim.localPosY) {
                ;
            } else {
                WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_SLIDING;
            }
        }
        break;
    case WCPUSHBLOCK_PHASE_SLIDING: {
        f32 zero = (*(const f32*)&gWcPushBlockZero);
        f32 vx = obj->anim.velocityX;
        if (zero != vx || zero != obj->anim.velocityZ) {
            f32 speed = sqrtf(vx * vx + obj->anim.velocityZ * obj->anim.velocityZ) -
                        (*(const f32*)&gWcPushBlockSlideSfxSpeedThreshold);
            if (speed < (*(const f32*)&gWcPushBlockZero)) {
                speed = (*(const f32*)&gWcPushBlockZero);
            }
            sfxVolume = (*(const f32*)&gWcPushBlockOne) + (*(const f32*)&gWcPushBlockSlideSfxVolumeRange) * speed /
                                                              (*(const f32*)&gWcPushBlockSlideSfxMaxSpeed);
            if (sfxVolume > (*(const f32*)&gWcPushBlockSlideSfxMaxVolume)) {
                sfxVolume = (*(const f32*)&gWcPushBlockSlideSfxMaxVolume);
            }
            Sfx_KeepAliveLoopedObjectSound(obj, SFXTRIG_en_treedrum16_c8);
            Sfx_SetObjectSfxVolume(obj, SFXTRIG_en_treedrum16_c8, sfxVolume,
                                   (*(const f32*)&gWcPushBlockSlideSfxVolumeScale));
            WCPUSHBLOCK_FLAGS(state).sfxActive = 1;
        }
    }
        dt = timeDelta;
        objMove(obj, obj->anim.velocityX * dt, (*(const f32*)&gWcPushBlockZero), obj->anim.velocityZ * dt);
        reachedTarget = 0;
        {
            if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_POS_X) {
                if (obj->anim.velocityX < (*(const f32*)&gWcPushBlockMaxSlideSpeed)) {
                    obj->anim.velocityX = (*(const f32*)&gWcPushBlockSlideAccel) * timeDelta + obj->anim.velocityX;
                }
                {
                    f32 tx;
                    if (obj->anim.localPosX >= (tx = WCPUSHBLOCK_TARGET_X(state))) {
                        obj->anim.localPosX = tx;
                        reachedTarget = 1;
                    }
                }
            } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_NEG_X) {
                if (obj->anim.velocityX > (*(const f32*)&gWcPushBlockMinSlideSpeed)) {
                    obj->anim.velocityX = obj->anim.velocityX - (*(const f32*)&gWcPushBlockSlideAccel) * timeDelta;
                }
                {
                    f32 tx;
                    if (obj->anim.localPosX <= (tx = WCPUSHBLOCK_TARGET_X(state))) {
                        obj->anim.localPosX = tx;
                        reachedTarget = 1;
                    }
                }
            } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_POS_Z) {
                if (obj->anim.velocityZ < (*(const f32*)&gWcPushBlockMaxSlideSpeed)) {
                    obj->anim.velocityZ = (*(const f32*)&gWcPushBlockSlideAccel) * timeDelta + obj->anim.velocityZ;
                }
                {
                    f32 tz;
                    if (obj->anim.localPosZ >= (tz = WCPUSHBLOCK_TARGET_Z(state))) {
                        obj->anim.localPosZ = tz;
                        reachedTarget = 1;
                    }
                }
            } else if (WCPUSHBLOCK_PUSH_DIR(state) == WCPUSHBLOCK_DIR_NEG_Z) {
                if (obj->anim.velocityZ > (*(const f32*)&gWcPushBlockMinSlideSpeed)) {
                    obj->anim.velocityZ = obj->anim.velocityZ - (*(const f32*)&gWcPushBlockSlideAccel) * timeDelta;
                }
                {
                    f32 tz;
                    if (obj->anim.localPosZ <= (tz = WCPUSHBLOCK_TARGET_Z(state))) {
                        obj->anim.localPosZ = tz;
                        reachedTarget = 1;
                    }
                }
            }
        }
        if (obj->anim.velocityX > (*(const f32*)&gWcPushBlockMaxSlideSpeed)) {
            obj->anim.velocityX = (*(const f32*)&gWcPushBlockMaxSlideSpeed);
        }
        if (obj->anim.velocityX < (*(const f32*)&gWcPushBlockMinSlideSpeed)) {
            obj->anim.velocityX = (*(const f32*)&gWcPushBlockMinSlideSpeed);
        }
        if (obj->anim.velocityZ > (*(const f32*)&gWcPushBlockMaxSlideSpeed)) {
            obj->anim.velocityZ = (*(const f32*)&gWcPushBlockMaxSlideSpeed);
        }
        if (obj->anim.velocityZ < (*(const f32*)&gWcPushBlockMinSlideSpeed)) {
            obj->anim.velocityZ = (*(const f32*)&gWcPushBlockMinSlideSpeed);
        }
        if (reachedTarget == 0) {
            break;
        }
        {
            f32 zero = (*(const f32*)&gWcPushBlockZero);
            obj->anim.velocityX = zero;
            obj->anim.velocityZ = zero;
        }
        {
            u32 moveResult = WCPUSHBLOCK_MOVE_RESULT(state);
            if (moveResult == WCPUSHBLOCK_MOVE_RESULT_LOCKED) {
                WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_LOCKED;
                if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
                    if (gameBitIncrement(GAMEBIT_WC_PushBlockACount) != WCPUSHBLOCK_REQUIRED_LOCK_COUNT) {
                        Sfx_PlayFromObject(0, SFXTRIG_sc_menuups16k_ca);
                    }
                } else {
                    if (gameBitIncrement(GAMEBIT_WC_PushBlockBCount) != WCPUSHBLOCK_REQUIRED_LOCK_COUNT) {
                        Sfx_PlayFromObject(0, SFXTRIG_sc_menuups16k_ca);
                    }
                }
            } else if (moveResult == WCPUSHBLOCK_MOVE_RESULT_CONTINUE) {
                WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_IDLE;
                if (WCPUSHBLOCK_FLAGS(state).sfxActive != 0) {
                    WCPUSHBLOCK_FLAGS(state).sfxActive = 0;
                    Sfx_PlayFromObject(obj, SFXTRIG_mv_bflconc1);
                }
            } else {
                if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
                    mainSetBits(GAMEBIT_WC_PushBlockAFade, 1);
                } else {
                    mainSetBits(GAMEBIT_WC_PushBlockBFade, 1);
                }
            }
        }
        if (WCPUSHBLOCK_FLAGS(state).phase != WCPUSHBLOCK_PHASE_FADE_OUT) {
            if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
                WCPUSHBLOCK_IFACE->setTileA(0, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), WCPUSHBLOCK_IFACE);
                WCPUSHBLOCK_IFACE->worldPosToTileA(obj, obj->anim.localPosX, obj->anim.localPosZ, &state->block.cellX,
                                                   &state->block.cellZ, WCPUSHBLOCK_IFACE);
                WCPUSHBLOCK_IFACE->setTileA(WCPUSHBLOCK_INITIAL_TILE(state), WCPUSHBLOCK_TILE_X(state),
                                            WCPUSHBLOCK_TILE_Y(state), WCPUSHBLOCK_IFACE);
            } else {
                WCPUSHBLOCK_IFACE->setTileB(0, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state), WCPUSHBLOCK_IFACE);
                WCPUSHBLOCK_IFACE->worldPosToTileB(obj, obj->anim.localPosX, obj->anim.localPosZ, &state->block.cellX,
                                                   &state->block.cellZ, WCPUSHBLOCK_IFACE);
                WCPUSHBLOCK_IFACE->setTileB(WCPUSHBLOCK_INITIAL_TILE(state), WCPUSHBLOCK_TILE_X(state),
                                            WCPUSHBLOCK_TILE_Y(state), WCPUSHBLOCK_IFACE);
            }
        }
        break;
    case WCPUSHBLOCK_PHASE_FADE_OUT:
        ObjHits_DisableObject(obj);
        if (objAnim->alpha == WCPUSHBLOCK_ALPHA_OPAQUE) {
            Sfx_PlayFromObject(obj, SFXTRIG_wp_iceywindlp16_cb);
        }
        {
            int a = objAnim->alpha - (framesThisStep << WCPUSHBLOCK_ALPHA_STEP_SHIFT);
            if (a < 0) {
                a = 0;
            }
            objAnim->alpha = a;
        }
        if (objAnim->alpha == 0) {
            if (wcblock_isPlayerAwayFromStoredCell(obj, &state->block, Obj_GetPlayerObject()) != 0) {
                if (objAnim->bankIndex == WCPUSHBLOCK_VARIANT_A) {
                    WCPUSHBLOCK_IFACE->getInitialTileXYA(WCPUSHBLOCK_INITIAL_TILE(state), &state->block.cellX,
                                                         &state->block.cellZ, WCPUSHBLOCK_IFACE);
                    WCPUSHBLOCK_IFACE->tileAToWorldPos(obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state),
                                                       &obj->anim.localPosX, &obj->anim.localPosZ, WCPUSHBLOCK_IFACE);
                } else {
                    WCPUSHBLOCK_IFACE->getInitialTileXYB(WCPUSHBLOCK_INITIAL_TILE(state), &state->block.cellX,
                                                         &state->block.cellZ, WCPUSHBLOCK_IFACE);
                    WCPUSHBLOCK_IFACE->tileBToWorldPos(obj, WCPUSHBLOCK_TILE_X(state), WCPUSHBLOCK_TILE_Y(state),
                                                       &obj->anim.localPosX, &obj->anim.localPosZ, WCPUSHBLOCK_IFACE);
                }
                WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_FADE_IN;
            }
        }
        break;
    case WCPUSHBLOCK_PHASE_FADE_IN:
        if (objAnim->alpha == 0) {
            ObjHits_EnableObject(obj);
            Sfx_PlayFromObject(0, SFXTRIG_en_cvdrip1c_cc);
        }
        {
            int a = objAnim->alpha + (framesThisStep << WCPUSHBLOCK_ALPHA_STEP_SHIFT);
            if (a > WCPUSHBLOCK_ALPHA_OPAQUE) {
                a = WCPUSHBLOCK_ALPHA_OPAQUE;
            }
            objAnim->alpha = a;
        }
        if (objAnim->alpha >= WCPUSHBLOCK_ALPHA_OPAQUE) {
            WCPUSHBLOCK_FLAGS(state).phase = WCPUSHBLOCK_PHASE_IDLE;
        }
        break;
    case WCPUSHBLOCK_PHASE_SOLVED:
        objAnim->alpha = WCPUSHBLOCK_ALPHA_OPAQUE;
    case WCPUSHBLOCK_PHASE_LOCKED:
        tex = objFindTexture(obj, 0, 0);
        if (tex != 0) {
            tex->textureId = WCPUSHBLOCK_TEXTURE_LOCKED;
        }
        obj->objectFlags |= WCPUSHBLOCK_OBJFLAG_LOCKED;
        break;
    }

    WCPUSHBLOCK_BOB_ANGLE(state) =
        (*(const f32*)&gWcPushBlockBobAngleSpeed) * timeDelta + (f32)(u32)WCPUSHBLOCK_BOB_ANGLE(state);
    WCPUSHBLOCK_BOB_Y(state) = (*(const f32*)&gWcPushBlockBobAmplitude) *
                               mathSinf((*(const f32*)&gWcPushBlockPi) * (f32)(u32)WCPUSHBLOCK_BOB_ANGLE(state) /
                                        (*(const f32*)&gWcPushBlockAngleScale));
    obj->anim.localPosY = WCPUSHBLOCK_BASE_Y(state) + WCPUSHBLOCK_BOB_Y(state);
}

void wcpushblock_init(GameObject* obj, WCPushBlockSetup* setup) {
    ObjAnimComponent* objAnim = &obj->anim;
    WCPushBlockRuntimeState* state = obj->extra;

    objAnim->alpha = 0;
    objAnim->bankIndex = setup->modelIndex;
    if (objAnim->bankIndex >= objAnim->modelInstance->modelCount) {
        objAnim->bankIndex = 0;
    }
    ObjHitbox_SetStateIndex(obj, obj->anim.hitReactState, objAnim->bankIndex);
    state->block.tileIndex = setup->initialTile;
    state->block.baseY = 5.0f + setup->base.posY;
}

void wcpushblock_release(void) {
}

void wcpushblock_initialise(void) {
}

#undef WCPUSHBLOCK_IFACE
