#include "main/render_envfx_api.h"

#include "main/asset_load.h"
#include "main/dll/cloudaction_interface.h"
#include "main/mldf_fileid.h"
#include "main/newclouds.h"
#include "main/sky_interface.h"

typedef struct EnvfxActEntry {
    u8 pad0[0x2a];
    u16 fadeDurationA;
    u8 pad1[0x30];
    u8 kind;
    u8 pad2[3];
} EnvfxActEntry;

int getEnvfxActImmediately(void* a, void* b, u16 idx, int d) {
    u8 raw[0x80];
    EnvfxActEntry* e = (EnvfxActEntry*)(((u32)raw + 0x1f) & ~0x1f);

    getTabEntry(e, MLDF_FILEID_ENVFXACT_BIN, idx * 0x60, 0x60);
    if (e != NULL) {
        if (e->kind <= 2 || e->kind == 4) {
            (*gNewCloudsInterface)->updateEnvfxAct(a, b, e, d);
        } else if (e->kind == 3) {
            e->fadeDurationA = 0;
            (*gSky2Interface)->updateEnvfxAct(a, b, e, d, idx);
        } else if (e->kind == 5) {
            e->fadeDurationA = 0;
            (*gSkyInterface)->updateEnvfxAct(a, b, e, d);
        } else if (e->kind == 6) {
            (*gCloudActionInterface)->updateEnvfxAct(a, b, e, d, idx);
        }
    }
    return 0;
}

int getEnvfxAct(void* a, void* b, u16 idx, int d) {
    u8 raw[0x80];
    EnvfxActEntry* e = (EnvfxActEntry*)(((u32)raw + 0x1f) & ~0x1f);

    getTabEntry(e, MLDF_FILEID_ENVFXACT_BIN, idx * 0x60, 0x60);
    if (e != NULL) {
        if (e->kind <= 2 || e->kind == 4) {
            (*gNewCloudsInterface)->updateEnvfxAct(a, b, e, d);
        } else if (e->kind == 3) {
            (*gSky2Interface)->updateEnvfxAct(a, b, e, d, idx);
        } else if (e->kind == 5) {
            (*gSkyInterface)->updateEnvfxAct(a, b, e, d);
        } else if (e->kind == 6) {
            (*gCloudActionInterface)->updateEnvfxAct(a, b, e, d, idx);
        }
    }
    return 0;
}
