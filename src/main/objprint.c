#define OBJHITS_SETTERS_S16
#include "main/objprint_api.h"
#include "main/frame_timing.h"
#include "main/shader_api.h"
#include "main/debug.h"
#include "MSL_C/PPCEABI/bare/H/math_api.h"
#include "game/objects/object.h"
#include "main/model.h"
#include "main/obj_contact.h"
#include "main/obj_list.h"
#include "main/objhits.h"
#include "main/object_transform.h"
#include "main/vecmath.h"
#include "main/track_dolphin_api.h"
#include "dolphin/os.h"
#include "main/asset_load.h"
#include "main/audio/sfx.h"
#include "main/mm.h"
#include "main/objanim_internal.h"
#include "main/objfx.h"
#include "main/objHitReact_types.h"
#include "main/dll/dll_005A_staffcollision.h"
#include "dlls/objects/226.h"
#include "main/resource.h"
#include "dolphin/mtx.h"
#include "main/dll/objpathtransform_struct.h"
#include "main/game_ui_interface.h"
#include "main/lightmap_api.h"
#include "main/dll/player_api.h"
#include "sys/objects/lifecycle.h"
#include "sys/objects.h"
#include "main/objtype.h"
#include "main/obj_hit_region.h"
#include "main/obj_link.h"
#include "main/obj_message.h"
#include "main/obj_path.h"
#include "main/obj_query.h"
#include "main/obj_trigger.h"
#include "main/player_eye_anim.h"
#include "main/pad_api.h"
#include "main/audio/sfx_play_api.h"
#include "main/rcp_dolphin_render_api.h"
#include "main/texture.h"
#include "main/objprint_dolphin_api.h"
#include "main/curve_eval.h"
#include "main/objprint_anim_api.h"
#include "main/objprint_character_api.h"
#include "main/objprint_sound_api.h"
#include "main/newshadows.h"
#include "main/objtexture.h"
#include "main/object_render.h"
#include "main/dll/modgfx.h"
#include "dolphin/gx/GXLighting.h"
#include "dolphin/gx/GXPixel.h"
#include "MSL_C/PPCEABI/bare/H/inverse_trig.h"
#include "dolphin/gx/GXGeometry.h"
#include "dolphin/gx/GXTev.h"
#include "dolphin/gx/GXTransform.h"
#include "track/intersect_api.h"
#include "main/objprint_internal.h"
#include "main/audio/sfx_channel_query_api.h"
#include "main/audio/sfx_stop_channel_api.h"
#include "main/objprint_render_api.h"

s16 gObjColorFilterRed;
s16 gObjColorFilterGreen;
s16 gObjColorFilterBlue;
f32* gObjModelMatrixOverride;
u8 gObjGlowColorRed;
u8 gObjGlowColorGreen;
u8 gObjGlowColorBlue;
u8 gObjGlowColorAlpha;
u8 gObjGlowColorEnabled;
u8 gObjColorFilterEnabled;

/* The byte alpha parameter retains the public int API through default promotion. */
void objSetGlowColor(red, green, blue, alpha) int red, green, blue;
u8 alpha;
{
    gObjGlowColorRed = red;
    gObjGlowColorGreen = green;
    gObjGlowColorBlue = blue;
    gObjGlowColorEnabled = 1;
    gObjGlowColorAlpha = alpha;
}

void objSetColorFilter(s16 red, s16 green, s16 blue) {
    gObjColorFilterRed = red;
    gObjColorFilterGreen = green;
    gObjColorFilterBlue = blue;
    gObjColorFilterEnabled = 1;
}

void staffUpdateSegmentTransforms(GameObject* staff, GameObject* owner, ObjModel* model, int a, int b, int c) {
    MtxPtr jointMatrix;
    int jointIndex;
    int attachmentIndex;
    int segmentIndex;
    Vec points[2];
    StaffState* state;

    if (OBJPRINT_MODEL_INSTANCE(staff)->attachPointCount >= 2 && staff->anim.classId == 0x2d) {
        state = (StaffState*)staff->extra;
        for (segmentIndex = 0; segmentIndex < state->geometrySegmentCount; segmentIndex++) {
            attachmentIndex = (segmentIndex << 1) + 1;
            if (attachmentIndex < OBJPRINT_MODEL_INSTANCE(staff)->attachPointCount) {
                jointIndex = OBJPRINT_MODEL_INSTANCE(staff)
                                 ->attachPoints[attachmentIndex + 1]
                                 .joints[OBJPRINT_ACTIVE_BANK_INDEX(staff)];
                jointMatrix = (MtxPtr)ObjModel_GetJointMatrix((u8*)model, jointIndex);
                points[1].x = OBJPRINT_MODEL_INSTANCE(staff)->attachPoints[attachmentIndex + 1].pos[0];
                points[1].y = OBJPRINT_MODEL_INSTANCE(staff)->attachPoints[attachmentIndex + 1].pos[1];
                points[1].z = OBJPRINT_MODEL_INSTANCE(staff)->attachPoints[attachmentIndex + 1].pos[2];
                PSMTXMultVec(jointMatrix, &points[1], &points[1]);
                points[1].x += playerMapOffsetX;
                points[1].z += playerMapOffsetZ;
                state->geometryPointBX[segmentIndex] = points[1].x;
                state->geometryPointBY[segmentIndex] = points[1].y;
                state->geometryPointBZ[segmentIndex] = points[1].z;
            }
            if (attachmentIndex < OBJPRINT_MODEL_INSTANCE(staff)->attachPointCount) {
                jointIndex = OBJPRINT_MODEL_INSTANCE(staff)
                                 ->attachPoints[attachmentIndex]
                                 .joints[OBJPRINT_ACTIVE_BANK_INDEX(staff)];
                jointMatrix =
                    (MtxPtr)(model->jointMatrices[model->bufferFlags & 1] + jointIndex * sizeof(ObjModelJointMatrix));
                points[0].x = OBJPRINT_MODEL_INSTANCE(staff)->attachPoints[attachmentIndex].pos[0];
                points[0].y = OBJPRINT_MODEL_INSTANCE(staff)->attachPoints[attachmentIndex].pos[1];
                points[0].z = OBJPRINT_MODEL_INSTANCE(staff)->attachPoints[attachmentIndex].pos[2];
                PSMTXMultVec(jointMatrix, &points[0], &points[0]);
                points[0].x += playerMapOffsetX;
                points[0].z += playerMapOffsetZ;
                state->geometryPointAX[segmentIndex] = points[0].x;
                state->geometryPointAY[segmentIndex] = points[0].y;
                state->geometryPointAZ[segmentIndex] = points[0].z;
            }
        }

        if (state->geometrySegmentCount != 0) {
            points[1].x = state->geometryPointBX[state->orientationSegmentIndex];
            points[1].y = state->geometryPointBY[state->orientationSegmentIndex];
            points[1].z = state->geometryPointBZ[state->orientationSegmentIndex];
            STAFF_INTERFACE(staff)->updateSwipe(staff, owner, &points[0]);
            points[1].x -= points[0].x;
            points[1].y -= points[0].y;
            points[1].z -= points[0].z;
            staff->anim.rotX = getAngle(points[1].x, points[1].z);
            staff->anim.rotY =
                (s16)(-getAngle(points[1].y, sqrtf(points[1].x * points[1].x + points[1].z * points[1].z)) + 0x4000);
            staff->anim.rotZ = 0;
        }
    }
}

void objRenderShadowIfVisible(GameObject* obj, int wpad0, int wpad1, int wpad2, int wpad3, int wpad4) {
    ObjModel** arr = obj->anim.modelBanks;
    s8 idx = obj->anim.bankIndex;
    if (arr[idx] != NULL) {
        objRenderShadow(obj);
    }
}

void objRenderModelAndHitVolumes(GameObject* obj, int p2, int p3, int p4, int p5, f32 scale) {
    ObjModel** table = obj->anim.modelBanks;
    (void)scale;
    if (table[obj->anim.bankIndex] != NULL) {
        objRenderModel(obj);
        if (obj->anim.hitVolumeTransforms != NULL) {
            objUpdateHitVolumeTransforms(obj);
        }
    }
}

void objSetModelMatrixOverride(f32* matrix) {
    gObjModelMatrixOverride = matrix;
}

void objRender(int a, int b, int c, int d, GameObject* obj, int flag) {
    void* sub;
    int i;
    void (*vfn)(GameObject*, int, int, int, int, int);

    if ((obj->objectFlags & OBJECT_OBJFLAG_FREED) != 0 || obj->ownerObj != NULL) {
        return;
    }
    if ((obj->anim.flags & OBJANIM_FLAG_HIDDEN) != 0) {
        return;
    }
    sub = (void*)obj->anim.parent;
    if (sub != NULL && (((GameObject*)sub)->anim.flags & OBJANIM_FLAG_HIDDEN) != 0) {
        return;
    }

    doNothing_beforeRenderObject(4);
    obj->objectFlags |= OBJECT_OBJFLAG_RENDERED;
    sub = (void*)obj->anim.dll;
    if (sub != NULL) {
        if ((obj->objectFlags & OBJECT_OBJFLAG_HIDDEN) == 0) {
            vfn =
                (void (*)(GameObject*, int, int, int, int, int))((ObjectInterface*)*(ObjectInterfaceHandle)sub)->render;
            if (vfn != NULL) {
                vfn(obj, a, b, c, d, flag);
            }
        } else if ((s8)flag != 0 && obj->anim.modelBanks[obj->anim.bankIndex] != NULL) {
            objRenderModel(obj);
            if (obj->anim.hitVolumeTransforms != NULL) {
                objUpdateHitVolumeTransforms(obj);
            }
        }
    } else if ((s8)flag != 0) {
        switch (obj->anim.romDefNo) {
        case 0:
        case 0x1f:
            playerRender((int)obj, a, b, c, d, flag);
            break;
        default:
            if (obj->anim.modelBanks[obj->anim.bankIndex] != NULL) {
                objRenderModel(obj);
                if (obj->anim.hitVolumeTransforms != NULL) {
                    objUpdateHitVolumeTransforms(obj);
                }
            }
            break;
        }
    }
    doNothing_afterRenderObject();
    for (i = 0; i < obj->childCount; i++) {
        GameObject* staff = obj->childObjs[i];
        if (staff->anim.classId == 0x2d) {
            staffUpdateSegmentTransforms(staff, obj, staff->anim.modelBanks[staff->anim.bankIndex], a, b, c);
        }
    }
}

u8 objGetAlphaCompareThreshold(void) {
    return gObjAlphaCompareThreshold;
}

void objSetAlphaCompareThreshold(u8 alpha) {
    gObjAlphaCompareThreshold = alpha;
}
