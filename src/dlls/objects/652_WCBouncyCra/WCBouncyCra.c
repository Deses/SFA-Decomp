/* WCBouncyCra (DLL 652) */
#include "main/dll/WC/dll_028C_wcbouncycra.h"
#include "main/frame_timing.h"
#include "main/object_render.h"
#include "dlls/object_descriptor.h"
#include "main/objtype.h"

#define WBOUNCY_EXTRA_SIZE     0xc
#define WBOUNCY_FLAG_ACTIVE    1
#define WBOUNCY_TRIGGER_GROUP  3
#define WBOUNCY_RESET_COOLDOWN 0x28
#define WBOUNCY_MAX_BOUNCES    0xa

/* Address reads below preserve the named constant pool instead of making MWCC
 * emit duplicate literal copies. Keep these definitions in retail pool order. */
const f32 gBouncyCrateTriggerSearchRadius = 10000.0f;
const f32 gBouncyCrateZero = 0.0f;
const f32 gBouncyCrateNearDistance = 200.0f;
const f32 gBouncyCrateMaxLaunchSpeed = 2.0f;
const f32 gBouncyCrateFarDistance = 500.0f;
const f32 gBouncyCrateLaunchFalloffRange = 300.0f;
const f32 gBouncyCrateOne = 1.0f;
const f32 gBouncyCrateGravity = -0.14f;
const f32 gBouncyCrateRestitution = 0.8f;
int WCBouncyCra_getExtraSize(void) {
    return WBOUNCY_EXTRA_SIZE;
}

int WCBouncyCra_getObjectTypeId(void) {
    return 0;
}

void WCBouncyCra_free(void) {
}

void WCBouncyCra_render(GameObject* obj, int p2, int p3, int p4, int p5, s8 visible) {
    if (visible != 0) {
        objRenderModelAndHitVolumes(obj, p2, p3, p4, p5, (*(const f32*)&gBouncyCrateOne));
    }
}

void WCBouncyCra_hitDetect(void) {
}

void WCBouncyCra_update(GameObject* obj) {
    WCBouncyCrateState* state = obj->extra;

    if ((state->flags & WBOUNCY_FLAG_ACTIVE) == 0) {
        s16 n = (s16)((f32)state->cooldown - timeDelta);
        state->cooldown = n;
        if (n <= 0) {
            f32 dist;
            f32 v = (*(const f32*)&gBouncyCrateTriggerSearchRadius);

            if ((void*)objGetNearestTypeTo(WBOUNCY_TRIGGER_GROUP, obj, &v) == NULL) {
                dist = (*(const f32*)&gBouncyCrateZero);
            } else {
                const f32 vv = v;
                dist = (*(const f32*)&gBouncyCrateNearDistance);
                if (vv < dist) {
                    dist = (*(const f32*)&gBouncyCrateMaxLaunchSpeed);
                } else if (vv > (*(const f32*)&gBouncyCrateFarDistance)) {
                    dist = (*(const f32*)&gBouncyCrateZero);
                } else {
                    dist = (vv - dist) / (*(const f32*)&gBouncyCrateLaunchFalloffRange);
                    dist = (*(const f32*)&gBouncyCrateOne) - dist;
                    dist *= (*(const f32*)&gBouncyCrateMaxLaunchSpeed);
                }
            }
            obj->anim.velocityY = dist;
            state->flags |= WBOUNCY_FLAG_ACTIVE;
            state->bounceCount = 0;
        }
    } else {
        obj->anim.velocityY = (*(const f32*)&gBouncyCrateGravity) * timeDelta + obj->anim.velocityY;
        obj->anim.localPosY = obj->anim.velocityY * timeDelta + obj->anim.localPosY;
        if (obj->anim.localPosY <= state->homeY) {
            obj->anim.localPosY += (state->homeY - obj->anim.localPosY);
            obj->anim.velocityY = (*(const f32*)&gBouncyCrateRestitution) * -obj->anim.velocityY;
            state->bounceCount += 1;
            if (state->bounceCount > WBOUNCY_MAX_BOUNCES) {
                state->flags &= ~WBOUNCY_FLAG_ACTIVE;
                state->cooldown = WBOUNCY_RESET_COOLDOWN;
                obj->anim.localPosY = state->homeY;
                obj->anim.velocityY = (*(const f32*)&gBouncyCrateZero);
            }
        }
    }
}

void WCBouncyCra_init(GameObject* obj, ObjPlacement* setup) {
    WCBouncyCrateState* state = obj->extra;

    state->homeY = setup->posY;
    state->cooldown = WBOUNCY_RESET_COOLDOWN;
}

void WCBouncyCra_release(void) {
}

void WCBouncyCra_initialise(void) {
}

ObjectDescriptor gWCBouncyCraObjDescriptor = {
    0,
    0,
    0,
    OBJECT_DESCRIPTOR_FLAGS_10_SLOTS,
    (ObjectDescriptorCallback)WCBouncyCra_initialise,
    (ObjectDescriptorCallback)WCBouncyCra_release,
    0,
    (ObjectDescriptorCallback)WCBouncyCra_init,
    (ObjectDescriptorCallback)WCBouncyCra_update,
    (ObjectDescriptorCallback)WCBouncyCra_hitDetect,
    (ObjectDescriptorCallback)WCBouncyCra_render,
    (ObjectDescriptorCallback)WCBouncyCra_free,
    (ObjectDescriptorCallback)WCBouncyCra_getObjectTypeId,
    (ObjectDescriptorExtraSizeCallback)WCBouncyCra_getExtraSize,
};
