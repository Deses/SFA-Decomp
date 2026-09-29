#include "main/render_mode_api.h"
#include "main/render_sequence_api.h"

int gRenderMode;
u8 gExpgfxUpdatingActivePools;

s16 renderModeSetOrGet(int mode) {
    if (mode != -1) {
        gRenderMode = mode;
        return mode;
    }
    return gRenderMode;
}

int ObjSeq_defaultActionCallback(int unused0, int unused1, int unused2, int unused3, int unused4, int unused5,
                                 int unused6) {
    return -0x1;
}
