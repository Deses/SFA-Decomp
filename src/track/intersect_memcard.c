#include "dolphin/os/OSRtc.h"
#include "dolphin/dvd.h"
#include "dolphin/os/OSCache.h"
#include "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/printf.h"
#include "main/fileio.h"
#include "main/frame_timing.h"
#include "main/maketex.h"
#include "main/maketex_api.h"
#include "main/maketex_random_api.h"
#include "main/maketex_sequence_api.h"
#include "main/maketex_timer_api.h"
#include "main/mm.h"
#include "main/textrender_api.h"
#include "main/vecmath.h"
#include "string.h"
#include "track/intersect_card_api.h"

#include "global.h"
#include "dolphin/card.h"
#include "dolphin/mtx.h"
#include "track/intersect_hud_color_api.h"
#include "main/texture.h"
#include "main/dll/player_state.h"
#include "main/sky_interface.h"
#include "main/gametext_color_api.h"
#include "main/gametext_command_api.h"
#include "main/gametext_show_str_api.h"
#include "main/gameloop_api.h"
#include "MSL_C/PPCEABI/bare/H/trig16.h"
#include "main/camera.h"
#include "main/track_dolphin_api.h"
#include "main/object_render.h"
#include "main/screen_transition.h"
#include "dolphin/gx/GXPixel.h"
#include "main/newshadows.h"
#include "main/pad.h"
#include "main/pi_dolphin.h"
#include "main/shader_api.h"
#include "dolphin/gx/GXTransform.h"
#include "main/gametext_internal.h"
#include "main/model_engine.h"
#include "main/pi_flush_api.h"
#include "track/intersect_api.h"
#include "dolphin/os.h"

void loadMemCardImages(void);

typedef void (*GXSetAlphaCompareIntFn)(int comp0, int ref0, int op, int comp1, int ref1);

typedef struct ReflectionTextureMatrixLayout {
    Mtx modelView;
    Mtx lightPerspective;
    Mtx lightPerspectiveFlipY;
    Mtx lightPerspectiveScaled;
} ReflectionTextureMatrixLayout;

STATIC_ASSERT(offsetof(ReflectionTextureMatrixLayout, lightPerspectiveFlipY) == 0x60);
STATIC_ASSERT(offsetof(ReflectionTextureMatrixLayout, lightPerspectiveScaled) == 0x90);

volatile s32 gSaveCardState = 0xD;
char* sMemoryCardFileName = "Star Fox Adventures";
int gSaveCardBackdropColor = 0x404040FF;
int lbl_803DB70C[1] = {0};

u8* gSaveCardImageBuffer;
u8 gSaveCardFileOpen;
u8 gSaveCardIdentityCheckEnabled;
u8 gSaveCardRetry;
u32 gSaveCardChecksumLo;
u32 gSaveCardChecksumHi;
u32 gSaveCardSerialLo;
u32 gSaveCardSerialHi;
char* gSaveCardIoBuffer;
void* gSaveCardWorkArea;
void loadReflectionTexMtxs(void) {
    ReflectionTextureMatrixLayout* mtxs = (ReflectionTextureMatrixLayout*)gCameraModelViewMatrix;
    Mtx tmp;
    PSMTXConcat(mtxs->lightPerspectiveScaled, (MtxPtr)(u32)mtxs->modelView, tmp);
    GXLoadTexMtxImm(tmp, GX_TEXMTX0, GX_MTX3x4);
    PSMTXConcat(mtxs->lightPerspectiveFlipY, (MtxPtr)(u32)mtxs->modelView, tmp);
    GXLoadTexMtxImm(tmp, GX_TEXMTX2, GX_MTX3x4);
}

/*
 * Retail ships a locally-defined empty OSReport that disables debug
 * output.
 */
void OSReport(const char* msg, ...) {
}

/*
 * Card init / serial-no validation. Mounts slot 0; if the mount comes back
 * "no card filesystem" (-13) it remembers we need to format. On a check
 * error (-6) it runs CARDCheck; if that also returns -6 it formats. On a
 * clean mount (or after the recovery path) it reads the card serial and
 * compares against the cached pair (gSaveCardSerialHi/Lo). If the cached pair
 * is zero, or doesn't match the live card, the cache is rejected with a
 * "wrong card" error code (-0x55, gSaveCardState = 11). Otherwise CARDFormat
 * if we still owe one, else success: clear the cache, set state 13,
 * unmount, return 1.
 */
int cardFormatMemoryCard(void) {
    int need_format;
    int res;
    u64 serial;
    int ok;

    need_format = 0;
    if (cardProbe(0) == 0) {
        ok = 0;
    } else {
        gSaveCardWorkArea = mmAlloc(0xA000, -1, 0);
        if (gSaveCardWorkArea == 0) {
            gSaveCardState = 8;
            ok = 0;
        } else {
            ok = 1;
        }
    }
    if (ok == 0) {
        return 0;
    }
    gSaveCardState = 0;
    res = CARDMount(0, gSaveCardWorkArea, (void*)cardSetStatusNoCard2);
    if (res == -13) {
        need_format = 1;
    }
    if (res == -6) {
        res = CARDCheck(0);
        if (res == -6) {
            res = CARDFormat(0);
        }
    } else if (res == -13 || res == 0) {
        res = CARDGetSerialNo(0, &serial);
        if (res == 0) {
            u64 cache = *(u64*)&gSaveCardSerialHi;
            if (cache == 0 || cache != serial) {
                res = -0x55;
                gSaveCardState = 0xB;
            } else if (need_format) {
                res = CARDFormat(0);
            } else {
                CARDUnmount(0);
                mm_free(gSaveCardWorkArea);
                gSaveCardWorkArea = 0;
                gSaveCardState = 0xD;
                return 1;
            }
        }
    }
    CARDUnmount(0);
    mm_free(gSaveCardWorkArea);
    gSaveCardWorkArea = 0;
    switch (res) {
    case -2:
        gSaveCardState = 1;
        break;
    case -3:
        if (gSaveCardState != 3) {
            gSaveCardState = 2;
        }
        break;
    case -5:
        gSaveCardState = 4;
        break;
    case 0:
        gSaveCardState = 0xD;
        gSaveCardSerialLo = 0;
        gSaveCardSerialHi = 0;
        gSaveCardChecksumLo = 0;
        gSaveCardChecksumHi = 0;
        return 1;
    default:
        break;
    }
    return 0;
}

void cardSetIdentityCheckEnabled(u32 enable) {
    u8 v = enable;
    gSaveCardIdentityCheckEnabled = v;
    if (v != 0) {
        return;
    }
    gSaveCardSerialLo = 0;
    gSaveCardSerialHi = 0;
    gSaveCardChecksumLo = 0;
    gSaveCardChecksumHi = 0;
}

void cardSetStatusNeedInit(void) {
    gSaveCardState = 0xd;
}

s32 saveGameGetStatus(void) {
    return gSaveCardState;
}

int cardDeleteSaveFile(void) {
    int res;
    int ok;

    gSaveCardRetry = 0;

    do {
        if (cardProbe(0) == 0) {
            ok = 0;
        } else {
            gSaveCardWorkArea = mmAlloc(0xA000, -1, 0);
            if (gSaveCardWorkArea == 0) {
                gSaveCardState = 8;
                ok = 0;
            } else {
                ok = 1;
            }
        }
        if (ok == 0) {
            return 0;
        }
        gSaveCardState = 0;
        res = CARDMount(0, gSaveCardWorkArea, (CARDCallback)cardSetStatusNoCard2);
        if (res == 0 || res == -6) {
            res = CARDCheck(0);
        }
        if (res == 0) {
            res = CARDDelete(0, sMemoryCardFileName);
        }
        CARDUnmount(0);
        mm_free(gSaveCardWorkArea);
        gSaveCardWorkArea = 0;

        switch (res + 13) {
        case 11:
            gSaveCardState = 1;
            break;
        case 10:
            if (gSaveCardState != 3) {
                gSaveCardState = 2;
            }
            break;
        case 0:
            gSaveCardState = 6;
            break;
        case 8:
            gSaveCardState = 4;
            break;
        case 13:
            gSaveCardState = 13;
            return 1;
        }
        showMemCardError(0);
    } while (gSaveCardRetry != 0);
    return 0;
}

#if defined(VERSION_GSAP01) || defined(VERSION_GSAP01_rev1)
int cardWriteOptions(void* data) {
    int ret;
    gSaveCardRetry = 0;
    cardShowLoadingMsg(1);
    do {
        ret = saveGame_prepareAndWrite(0, 0, 0, NULL, data, saveGameWriteOptionsCb);
        showMemCardError(0);
        if (gSaveCardRetry != 0) {
            cardShowLoadingMsg(1);
        }
    } while (gSaveCardRetry != 0);
    return ret;
}
#endif

int _saveGame(int slot, void* save, void* data) {
    int ret;
    gSaveCardRetry = 0;
    cardShowLoadingMsg(1);
    do {
        ret = saveGame_prepareAndWrite(0, slot, 0, save, data, (SaveGameCallback)saveGameWriteSlotCb);
        showMemCardError(0);
        if (gSaveCardRetry != 0) {
            cardShowLoadingMsg(1);
        }
    } while (gSaveCardRetry != 0);
    return ret;
}

int maybeTryLoadSave(void* data) {
    int ret;
    gSaveCardRetry = 0;
    cardShowLoadingMsg(0);
    do {
        ret = saveGame_prepareAndWrite(1, 0, 0, data, NULL, (SaveGameCallback)saveGameReadGlobalsCb);
        showMemCardError(1);
        if (gSaveCardRetry != 0) {
            cardShowLoadingMsg(0);
        }
    } while (gSaveCardRetry != 0);
    return ret;
}

int loadSaveGame(int slot, void* save) {
    int ret;
    gSaveCardRetry = 0;
    cardShowLoadingMsg(0);
    do {
        ret = saveGame_prepareAndWrite(1, slot, 0, save, NULL, (SaveGameCallback)saveGameReadSlotCb);
        showMemCardError(0);
        if (gSaveCardRetry != 0) {
            cardShowLoadingMsg(0);
        }
    } while (gSaveCardRetry != 0);
    return ret;
}

int cardCreateSaveFile(u8 retry) {
    int ret;

    if (retry != 0) {
        gSaveCardRetry = 0;
        cardShowLoadingMsg(2);
    }
    do {
        ret = saveGame(0);
        if (ret != 0) {
            if (gSaveCardFileOpen != 0) {
                gSaveCardFileOpen = 0;
                CARDClose(&gSaveCardFileInfo.fileInfo);
            }
            CARDUnmount(0);
            mm_free(gSaveCardWorkArea);
            gSaveCardWorkArea = 0;
            gSaveCardState = 13;
            if (ret == 2) {
                ret = saveGame_prepareAndWrite(0, 0, 0, NULL, NULL, NULL);
            }
        }
        if (retry != 0) {
            showMemCardError(0);
        }
        if (gSaveCardRetry != 0) {
            cardShowLoadingMsg(2);
        }
    } while (gSaveCardRetry != 0 && retry != 0);
    return ret;
}
int cardProbe(u8 retry) {

    s32 memSize;
    s32 sectorSize;
    s32 res;

    if (retry != 0) {
        gSaveCardRetry = 0;
    }
    do {
        res = -1;
        while (res == -1) {
            res = CARDProbeEx(0, &memSize, &sectorSize);
        }
        if (res == 0) {
            if (sectorSize == 0x2000) {
                gSaveCardState = 13;
                return 1;
            }
            gSaveCardState = 7;
        } else if (res == -3) {
            gSaveCardState = 2;
        } else if (res == -2) {
            gSaveCardState = 1;
        } else {
            gSaveCardState = 0;
        }
        if (retry != 0) {
            showMemCardError(0);
        }
    } while (gSaveCardRetry != 0 && retry != 0);
    return 0;
}

void _initCardAndDsp(void) {
    CARDInit();
}

void cardGetMessage(u32* buttons, u32* texts, u32* count) {
    if (gSaveCardIdentityCheckEnabled != 0 && (gSaveCardState == 7 || gSaveCardState == 9)) {
        gSaveCardState = 11;
    }
    switch (gSaveCardState) {
    case 0:
        *count = 0;
        gSaveCardState = 13;
        return;
    case 1:
        buttons[0] = 1;
        buttons[1] = 2;
        texts[0] = 0x325;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        *count = 2;
        return;
    case 2:
        buttons[0] = 1;
        buttons[1] = 2;
        texts[0] = 0x51A;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        *count = 2;
        return;
    case 3:
        buttons[0] = 1;
        buttons[1] = 2;
        texts[0] = 0x51A;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        *count = 2;
        return;
    case 4:
        buttons[0] = 1;
        buttons[1] = 2;
        texts[0] = 0x329;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        *count = 2;
        return;
    case 5:
        buttons[0] = 1;
        buttons[1] = 2;
        buttons[2] = 0;
        texts[0] = 0x51F;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        texts[3] = 0x326;
        *count = 3;
        return;
    case 6:
        buttons[0] = 1;
        buttons[1] = 2;
        buttons[2] = 0;
        texts[0] = 0x51E;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        texts[3] = 0x326;
        *count = 3;
        return;
    case 7:
        buttons[0] = 1;
        buttons[1] = 2;
        texts[0] = 0x51C;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        *count = 2;
        return;
    case 8:
        *count = 0;
        return;
    case 9:
        buttons[0] = 1;
        buttons[1] = 2;
        buttons[2] = 3;
        texts[0] = 0x32A;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        texts[3] = 0x520;
        *count = 3;
        return;
    case 10:
        buttons[0] = 2;
        buttons[1] = 4;
        texts[0] = 0x497;
        texts[1] = 0x51B;
        texts[2] = 0x522;
        *count = 2;
        return;
    case 11:
    case 12:
        buttons[0] = 1;
        buttons[1] = 2;
        texts[0] = 0x521;
        texts[1] = 0x51D;
        texts[2] = 0x51B;
        *count = 2;
        return;
    case 13:
    default:
        *count = 0;
        gSaveCardState = 13;
        return;
    }
}

void showMemCardError(u8 err) {
    int opts[8];
    int msgs[8];
    int count;
#if !defined(VERSION_GSAE01) && !defined(VERSION_GSAJ01)
    int measureLeft, measureRight, measureTop, measureBottom;
    int lineHeight;
    int textHeight;
#endif
    u32 saved;
    int* m;
    int y;
    int sel;
    int i;
    u8 submenu;
    int timer;
    u8 held;
    int j;
    int yy;
    GameTextDef* t;
    int v;

    sel = 0;
    submenu = 0;
    timer = 0;
    held = 0;
    gSaveCardRetry = 0;
#if !defined(VERSION_GSAE01) && !defined(VERSION_GSAJ01)
    if (OSGetLanguage() == OS_LANGUAGE_DUTCH && err != 0) {
        if (gSaveCardState == 2 || gSaveCardState == 3) {
            return;
        }
    }
#endif
    if (gSaveCardState == 0xd || (err != 0 && gSaveCardState == 0xc)) {
        return;
    }
    do {
        checkReset();
        padUpdate();
        mmFreeTick(0);
        timer += 0x3e8;
        waitNextFrame();
        saved = gSaveCardBackdropColor;
        hudDrawColored(newshadows_getReflectionColorTexture(), 0, 0, &saved, 0x200, 0);
        if (submenu != 0) {
            opts[0] = 6;
            opts[1] = 5;
            msgs[0] = 0x327;
            msgs[1] = 0x321;
            msgs[2] = 0x320;
            count = 2;
        } else {
            cardGetMessage((u32*)opts, (u32*)msgs, (u32*)&count);
        }
        gameTextSetColor(0xff, 0xc0, 0x40, 0xff);
        for (i = 0, m = msgs, y = 0x64; i < count + 1; m++, y += 0x14, i++) {
            t = gameTextGet(*m);
            yy = y + ((i > 0) ? 0x64 : 0);
            for (j = 0; j < t->count; j++) {
#if !defined(VERSION_GSAE01) && !defined(VERSION_GSAJ01)
                int rowHeight;
#endif
                gameTextShowStr(t->strings[j], 0, 0, yy);
#if !defined(VERSION_GSAE01) && !defined(VERSION_GSAJ01)
                gameTextMeasureStringBoundsAt(t->strings[j], 0, 0, 0, &measureLeft, &measureRight, &measureTop,
                                              &measureBottom);
                lineHeight = gGameTextFontMetrics[sLanguageNameTable[getCurLanguage()].fontId].lineHeight;
                textHeight = measureBottom - measureTop;
                rowHeight = (textHeight > lineHeight) ? textHeight
                                : gGameTextFontMetrics[sLanguageNameTable[getCurLanguage()].fontId].lineHeight;
                yy = rowHeight + yy;
                yy += 5;
#else
                yy += 0x18;
#endif
            }
            if (i == sel) {
                v = (int)(47.0f * fcos16HighPrecision(timer) + 208.0f);
                gameTextSetColor(v, v, v, 0xff);
            } else {
                gameTextSetColor(0xa0, 0xa0, 0xa0, 0xff);
            }
        }
        gameTextRun();
        GXFlush_(1, 0);
        if (padGetStickY(0) < 0 || padGetCY(0) < 0) {
            if (held == 0) {
                sel++;
                held = 1;
            }
        } else if (padGetStickY(0) > 0 || padGetCY(0) > 0) {
            if (held == 0) {
                sel--;
                held = 1;
            }
        } else {
            held = 0;
        }
        if (sel < 0) {
            sel = 0;
        } else if (sel > count - 1) {
            sel = count - 1;
        }
        if (getButtonsJustPressed(0) & 0x100) {
            switch (opts[sel]) {
            case 0:
                submenu = 1;
                sel = 0;
                break;
            case 1:
                gSaveCardState = 0xd;
                gSaveCardRetry = 1;
                break;
            case 2:
                gSaveGameEnabled = 0;
                gSaveCardState = 0xd;
                break;
            case 3:
                setGameState(6);
                gSaveGameEnabled = 0;
                gSaveCardState = 0xd;
                break;
            case 4:
                cardDeleteSaveFile();
                cardCreateSaveFile(0);
                if (gSaveCardState == 0xd) {
                    gSaveCardRetry = 1;
                }
                break;
            case 5:
                submenu = 0;
                if (cardFormatMemoryCard() != 0) {
                    cardCreateSaveFile(0);
                }
                if (gSaveCardState == 0xd) {
                    gSaveCardRetry = 1;
                }
                break;
            case 6:
                submenu = 0;
                break;
            default:
                gSaveCardState = 0xd;
            }
        }
    } while (gSaveCardState != 0xd);
}

/*
 * Per-frame "blocking" dialog renderer driven by the card-write retry
 * loops in _saveGame, maybeTryLoadSave, loadSaveGame and cardCreateSaveFile.
 * Pumps 60 frames of the GX/dialog
 * pipeline; on each frame either lets the active controller draw its own
 * popup (gScreenTransitionInterface[0]->vtbl[1]) or falls back to hudDrawColored
 * tinting the reflection texture with gSaveCardBackdropColor, then routes the OK/Cancel/back text
 * to gameTextShowAt based on the dialog kind passed in.
 */
void cardShowLoadingMsg(u8 kind) {
    GameObject** buttons;
    u32 saved;
    int frame;
    int j;
    int count;
    f32 rectAlpha;
    void (*draw)(int, int, int);
    u8 mode = kind;

    gameTextSetWindow(0);
    for (frame = 0; frame < 0x3C; frame++) {
        padUpdate();
        mmFreeTick(0);
        waitNextFrame();
        count = getButtonObjects(&buttons) & 0xFF;
        if ((u32)count != 0) {
            draw = (*gScreenTransitionInterface)->init;
            draw(0, 0, 0);
            rectAlpha = 0.0f;
            drawRect(rectAlpha, rectAlpha, 0x280, 0x1E0);
            for (j = 0; j < count; j++) {
                objRenderModelAndHitVolumes(buttons[j], 0, 0, 0, 0, 1.0f);
            }
            curUiDllDraw(0, 0, 0, 0);
        } else {
            saved = gSaveCardBackdropColor;
            hudDrawColored(newshadows_getReflectionColorTexture(), 0, 0, &saved, 0x200, 0);
        }
        gameTextSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        if (mode == 1) {
            gameTextShowAt(0x323, 0, 0xC8);
        } else if (mode == 2) {
            gameTextShowAt(0x573, 0, 0xC8);
        } else {
            gameTextShowAt(0x56C, 0, 0xC8);
        }
        gameTextRun();
        GXFlush_(1, 0);
    }
}

#if defined(VERSION_GSAP01) || defined(VERSION_GSAP01_rev1)
int saveGameWriteOptionsCb(int slot, int unused, void* save, void* data) {
    int ret;
    memcpy(gSaveCardIoBuffer + 0x1F14, data, 0xE4);
    ret = saveGame_doWrite(2);
    if (ret == 0) {
        ret = saveGame_doWrite(1);
    }
    return ret;
}
#endif

/*
 * Card-write callback dispatched through saveGame_prepareAndWrite from _saveGame.
 * Stages a per-slot 0x6EC-byte block plus the shared 0xE4-byte trailer
 * into the card-IO buffer (gSaveCardIoBuffer), then asks saveGame_doWrite(2) to
 * commit; if that fails it falls back to saveGame_doWrite(1).
 */
int saveGameWriteSlotCb(u8 slot, int unused, void* src1, void* src2) {
    int ret;
    memcpy(gSaveCardIoBuffer + slot * 0x6EC + 0xA50, src1, 0x6EC);
    memcpy(gSaveCardIoBuffer + 0x1F14, src2, 0xE4);
    ret = saveGame_doWrite(2);
    if (ret == 0) {
        ret = saveGame_doWrite(1);
    }
    return ret;
}

/*
 * Card-write callback dispatched through saveGame_prepareAndWrite from maybeTryLoadSave.
 * Copies the 0xE4-byte block at offset 0x1F14 in the card buffer (held in
 * gSaveCardIoBuffer) into the caller-supplied destination.
 */
int saveGameReadGlobalsCb(int saveId, int size, void* dst) {
    memcpy(dst, gSaveCardIoBuffer + 0x1F14, 0xE4);
    return 0;
}


static inline u64 saveGame_checksum(u64* p, int count) {
    u64 x;
    u16 i[1];
    u64 acc;

    x = 0;
    acc = 1;
    for (i[0] = (int)x; (int)i[0] < count; i[0]++) {
        x ^= p[i[0]];
        acc += p[i[0]];
    }
    return x ^ (acc + 13);
}

int saveGameReadSlotCb(u8 idx, int unused, void* dst) {
    memcpy(dst, (void*)(gSaveCardIoBuffer + idx * 1772 + 2640), 1772);
    return 0;
}

/* Checksums the save buffer, writes it to the memory card, then reads it
 * back and verifies the checksum. */
int saveGame_doWrite(int slot) {
    u64 x[1];
    u16 i[1];
    u64* p;
    u64 a[1];
    u64 chk;
    u64 chk2;
    int result;
    int offset;

    p = (u64*)gSaveCardIoBuffer;
    x[0] = 0;
    a[0] = 1;
    for (i[0] = (int)x[0]; (int)i[0] < 0x3ff; i[0]++) {
        x[0] = x[0] ^ p[i[0]];
        a[0] = a[0] + p[i[0]];
    }
    chk = x[0] ^ (a[0] + 13);
    p[0x3ff] = chk;
    DCFlushRange((void*)gSaveCardIoBuffer, 0x2000);
    result = CARDWrite(&gSaveCardFileInfo.fileInfo, (void*)gSaveCardIoBuffer, 0x2000, offset = (u8)slot << 13);
    if (result == -5) {
        CARDDelete(0, sMemoryCardFileName);
    }
    if (result == 0) {
        DCInvalidateRange((void*)gSaveCardIoBuffer, 0x2000);
        result = CARDRead(&gSaveCardFileInfo.fileInfo, (void*)gSaveCardIoBuffer, 0x2000, offset);
        if (result == 0) {
            u64 x2[1];
            u64 a2[1];
            p = (u64*)gSaveCardIoBuffer;
            x2[0] = 0;
            a2[0] = 1;
            for (i[0] = (int)x2[0]; (int)i[0] < 0x3ff; i[0]++) {
                x2[0] = x2[0] ^ p[i[0]];
                a2[0] = a2[0] + p[i[0]];
            }
            chk2 = x2[0] ^ (a2[0] + 13);
            if (chk != chk2) {
                result = -0x55;
                gSaveCardState = 10;
            } else {
                *(u64*)&gSaveCardChecksumHi = chk2;
            }
        }
    }
    return result;
}

/* Saves the game: verifies the existing save slots' checksums, rewrites
 * stale slots and card images, then runs the caller's callback and maps the
 * result to a status code. */
#if !defined(VERSION_GSAE01) && !defined(VERSION_GSAJ01)
static void saveCardBuildComment(void);
#endif
int saveGame_prepareAndWrite(int writeImages, int cbA, int cbB, void* cbC, void* cbD, SaveGameCallback cb) {
    u64 chk;
    u64 chk2;
    u64 c;
    u64 t;
    int result;
    void* m;

    m = mmAlloc(0x2000, -1, 0);
    gSaveCardIoBuffer = (char*)m;
    if (m == NULL) {
        gSaveCardState = 8;
        return 0;
    }
    if (saveGame(writeImages) == 0) {
        mm_free((void*)gSaveCardIoBuffer);
        gSaveCardIoBuffer = 0;
        return 0;
    }
    DCInvalidateRange((void*)gSaveCardIoBuffer, 0x2000);
    result = CARDRead(&gSaveCardFileInfo.fileInfo, (void*)gSaveCardIoBuffer, 0x2000, 0x2000);
    if (result == CARD_RESULT_READY) {
        c = saveGame_checksum((u64*)gSaveCardIoBuffer, 0x3ff);
        chk = c;
        if (c != ((u64*)gSaveCardIoBuffer)[0x3ff]) {
            DCInvalidateRange((void*)gSaveCardIoBuffer, 0x2000);
            result = CARDRead(&gSaveCardFileInfo.fileInfo, (void*)gSaveCardIoBuffer, 0x2000, 0x4000);
            if (result == CARD_RESULT_READY) {
                c = saveGame_checksum((u64*)gSaveCardIoBuffer, 0x3ff);
                chk = c;
                if (c == ((u64*)gSaveCardIoBuffer)[0x3ff]) {
                    result = saveGame_doWrite(1);
                } else {
                    result = -0x55;
                    gSaveCardState = 10;
                }
            }
        }
    }
    if (result == 0) {
        if (gSaveCardIdentityCheckEnabled != 0) {
            if (*(u64*)&gSaveCardChecksumHi != 0) {
                if (chk != *(u64*)&gSaveCardChecksumHi) {
                    result = -0x55;
                    gSaveCardState = 0xb;
                }
            } else {
                gSaveCardChecksumLo = (u32)chk;
                gSaveCardChecksumHi = (u32)(chk >> 32);
            }
        } else {
            gSaveCardChecksumLo = (u32)chk;
            gSaveCardChecksumHi = (u32)(chk >> 32);
        }
    }
    if (result == 0) {
        m = gSaveCardImageBuffer = mmAlloc(0x4000, -1, 0);
        if (m == NULL) {
            if (gSaveCardFileOpen != 0) {
                gSaveCardFileOpen = 0;
                CARDClose(&gSaveCardFileInfo.fileInfo);
            }
            CARDUnmount(0);
            mm_free(gSaveCardWorkArea);
            gSaveCardWorkArea = NULL;
            mm_free((void*)gSaveCardIoBuffer);
            gSaveCardIoBuffer = 0;
            gSaveCardState = 8;
            return 0;
        }
        result = CARDRead(&gSaveCardFileInfo.fileInfo, m, 0x2000, 0);
        if (result == CARD_RESULT_READY) {
#if defined(VERSION_GSAP01) || defined(VERSION_GSAP01_rev1)
            if ((u8)writeImages == 0) {
                saveCardBuildComment();
            }
#endif
            chk2 = saveGame_checksum((u64*)gSaveCardImageBuffer, 0x400);
            if (chk2 != *(u64*)(gSaveCardIoBuffer + 0xa40)) {
                if ((u8)writeImages != 0) {
                    result = -4;
                    gSaveCardState = 0xc;
                } else {
                    memset(gSaveCardImageBuffer, 0, 0x4000);
                    loadMemCardImages();
                    result = CARDWrite(&gSaveCardFileInfo.fileInfo, gSaveCardImageBuffer, 0x2000, 0);
                    if (result == CARD_RESULT_IOERROR) {
                        CARDDelete(0, sMemoryCardFileName);
                    }
                    if (result == CARD_RESULT_READY) {
                        t = *(u64*)(gSaveCardImageBuffer + 0x2a40);
                        if (t != *(u64*)(gSaveCardIoBuffer + 0xa40)) {
                            int writeResult;
                            *(u64*)(gSaveCardIoBuffer + 0xa40) = t;
#if !defined(VERSION_GSAE01) && !defined(VERSION_GSAJ01)
                            if (cb == NULL) {
                                writeResult = saveGame_doWrite(2);
                                if (writeResult == 0) {
                                    writeResult = saveGame_doWrite(1);
                                }
                                result = writeResult;
                            }
#else
                            writeResult = saveGame_doWrite(2);
                            if (writeResult == 0) {
                                writeResult = saveGame_doWrite(1);
                            }
                            result = writeResult;
#endif
                        }
                    }
                }
            }
        }
        mm_free(gSaveCardImageBuffer);
    }
    if (result == 0 && cb != NULL) {
        result = cb(cbA, cbB, cbC, cbD);
    }
    if (gSaveCardFileOpen != 0) {
        gSaveCardFileOpen = 0;
        CARDClose(&gSaveCardFileInfo.fileInfo);
    }
    CARDUnmount(0);
    mm_free(gSaveCardWorkArea);
    gSaveCardWorkArea = NULL;
    mm_free((void*)gSaveCardIoBuffer);
    gSaveCardIoBuffer = 0;
    switch (result) {
    case -5:
        gSaveCardState = 4;
        break;
    case 0:
        gSaveCardState = 0xd;
        return 1;
    case -4:
        break;
    }
    return 0;
}

/* Builds the memory card comment strings (Shift-JIS title on JP cards),
 * loads the banner/icon images from disc, and checksums both halves of the
 * card image buffer. */
void loadMemCardImages(void) {
    DVDFileInfo fi;
    u64* p;
    u16 i[1];
    u64 x[1];
    u64* q;
    u64 a[1];
    u64 chk;
    u64 x2[1];
    u64 a2[1];

    a[0] = 0;
#if defined(VERSION_GSAE01) || defined(VERSION_GSAJ01)
    if (gGameTextFontIsSjis != 0) {
        gSaveCardImageBuffer[0x00] = 0x83;
        gSaveCardImageBuffer[0x01] = 0x58;
        gSaveCardImageBuffer[0x02] = 0x83;
        gSaveCardImageBuffer[0x03] = 0x5e;
        gSaveCardImageBuffer[0x04] = 0x81;
        gSaveCardImageBuffer[0x05] = 0x5b;
        gSaveCardImageBuffer[0x06] = 0x83;
        gSaveCardImageBuffer[0x07] = 0x74;
        gSaveCardImageBuffer[0x08] = 0x83;
        gSaveCardImageBuffer[0x09] = 0x48;
        gSaveCardImageBuffer[0x0a] = 0x83;
        gSaveCardImageBuffer[0x0b] = 0x62;
        gSaveCardImageBuffer[0x0c] = 0x83;
        gSaveCardImageBuffer[0x0d] = 0x4e;
        gSaveCardImageBuffer[0x0e] = 0x83;
        gSaveCardImageBuffer[0x0f] = 0x58;
        gSaveCardImageBuffer[0x10] = 0x83;
        gSaveCardImageBuffer[0x11] = 0x41;
        gSaveCardImageBuffer[0x12] = 0x83;
        gSaveCardImageBuffer[0x13] = 0x68;
        gSaveCardImageBuffer[0x14] = 0x83;
        gSaveCardImageBuffer[0x15] = 0x78;
        gSaveCardImageBuffer[0x16] = 0x83;
        gSaveCardImageBuffer[0x17] = 0x93;
        gSaveCardImageBuffer[0x18] = 0x83;
        gSaveCardImageBuffer[0x19] = 0x60;
        gSaveCardImageBuffer[0x1a] = 0x83;
        gSaveCardImageBuffer[0x1b] = 0x83;
        gSaveCardImageBuffer[0x1c] = 0x81;
        gSaveCardImageBuffer[0x1d] = 0x5b;
        gSaveCardImageBuffer[0x1e] = 0x00;
        gSaveCardImageBuffer[0x1f] = 0x00;
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "STARFOX ADVENTURES");
    } else {
        sprintf((char*)gSaveCardImageBuffer, "Star Fox Adventures");
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "Dinosaur Planet");
    }
#else
    saveCardBuildComment();
#endif
    if (DVDOpen("opening.bnr", &fi)) {
        DVDRead(&fi, gSaveCardImageBuffer + 0x40, 0x1800, 0x20);
        DVDClose(&fi);
    }
    if (DVDOpen("card/memcardicon0.img", &fi)) {
        DVDRead(&fi, gSaveCardImageBuffer + 0x1840, 0x400, 0);
        DVDClose(&fi);
    }
    if (DVDOpen("card/memcardicon1.img", &fi)) {
        DVDRead(&fi, gSaveCardImageBuffer + 0x1c40, 0x400, 0);
        DVDClose(&fi);
    }
    if (DVDOpen("card/memcardicon2.img", &fi)) {
        DVDRead(&fi, gSaveCardImageBuffer + 0x2040, 0x400, 0);
        DVDClose(&fi);
    }
    if (DVDOpen("card/memcardicon3.img", &fi)) {
        DVDRead(&fi, gSaveCardImageBuffer + 0x2440, 0x400, 0);
        DVDClose(&fi);
    }
    if (DVDOpen("card/memcardicon0.pal", &fi)) {
        DVDRead(&fi, gSaveCardImageBuffer + 0x2840, 0x200, 0);
        DVDClose(&fi);
    }
    p = (u64*)gSaveCardImageBuffer;
    x[0] = 0;
    a[0] = 1;
    for (i[0] = (int)x[0]; (int)i[0] < 0x400; i[0]++) {
        x[0] = x[0] ^ p[i[0]];
        a[0] = a[0] + p[i[0]];
    }
    chk = x[0] ^ (a[0] + 13);
    ((u32*)p)[0xa91] = (u32)chk;
    ((u32*)p)[0xa90] = (u32)(chk >> 32);
    q = (u64*)gSaveCardImageBuffer;
    p = q + 0x400;
    x2[0] = 0;
    a2[0] = 1;
    for (i[0] = (int)x2[0]; (int)i[0] < 0x3ff; i[0]++) {
        x2[0] = x2[0] ^ p[i[0]];
        a2[0] = a2[0] + p[i[0]];
    }
    chk = x2[0] ^ (a2[0] + 13);
    ((u32*)q)[0xfff] = (u32)chk;
    ((u32*)q)[0xffe] = (u32)(chk >> 32);
    DCFlushRange(gSaveCardImageBuffer, 0x4000);
}
#if !defined(VERSION_GSAE01) && !defined(VERSION_GSAJ01)
static void saveCardBuildComment(void)
{
#if defined(VERSION_GSAE01_rev1)
    int language = getCurLanguage();
    if (language == OS_LANGUAGE_ITALIAN) {
        gSaveCardImageBuffer[0x00] = 0x83;
        gSaveCardImageBuffer[0x01] = 0x58;
        gSaveCardImageBuffer[0x02] = 0x83;
        gSaveCardImageBuffer[0x03] = 0x5e;
        gSaveCardImageBuffer[0x04] = 0x81;
        gSaveCardImageBuffer[0x05] = 0x5b;
        gSaveCardImageBuffer[0x06] = 0x83;
        gSaveCardImageBuffer[0x07] = 0x74;
        gSaveCardImageBuffer[0x08] = 0x83;
        gSaveCardImageBuffer[0x09] = 0x48;
        gSaveCardImageBuffer[0x0a] = 0x83;
        gSaveCardImageBuffer[0x0b] = 0x62;
        gSaveCardImageBuffer[0x0c] = 0x83;
        gSaveCardImageBuffer[0x0d] = 0x4e;
        gSaveCardImageBuffer[0x0e] = 0x83;
        gSaveCardImageBuffer[0x0f] = 0x58;
        gSaveCardImageBuffer[0x10] = 0x83;
        gSaveCardImageBuffer[0x11] = 0x41;
        gSaveCardImageBuffer[0x12] = 0x83;
        gSaveCardImageBuffer[0x13] = 0x68;
        gSaveCardImageBuffer[0x14] = 0x83;
        gSaveCardImageBuffer[0x15] = 0x78;
        gSaveCardImageBuffer[0x16] = 0x83;
        gSaveCardImageBuffer[0x17] = 0x93;
        gSaveCardImageBuffer[0x18] = 0x83;
        gSaveCardImageBuffer[0x19] = 0x60;
        gSaveCardImageBuffer[0x1a] = 0x83;
        gSaveCardImageBuffer[0x1b] = 0x83;
        gSaveCardImageBuffer[0x1c] = 0x81;
        gSaveCardImageBuffer[0x1d] = 0x5b;
        gSaveCardImageBuffer[0x1e] = 0x00;
        gSaveCardImageBuffer[0x1f] = 0x00;
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "STARFOX ADVENTURES");
    } else {
        sprintf((char*)gSaveCardImageBuffer, "Star Fox Adventures");
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "Dinosaur Planet");
    }
#else
    switch (getCurLanguage()) {
    case OS_LANGUAGE_GERMAN:
        sprintf((char*)gSaveCardImageBuffer, "Star Fox Adventures");
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "Dinosaur Planet");
        break;
    case OS_LANGUAGE_SPANISH:
        sprintf((char*)gSaveCardImageBuffer, "Star Fox Adventures");
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "Dinosaur Planet");
        break;
    case OS_LANGUAGE_DUTCH:
        sprintf((char*)gSaveCardImageBuffer, "Star Fox Adventures");
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "Dinosaur Planet");
        break;
    case OS_LANGUAGE_FRENCH:
        sprintf((char*)gSaveCardImageBuffer, "Star Fox Adventures");
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "Dinosaur Planet");
        break;
    default:
        sprintf((char*)gSaveCardImageBuffer, "Star Fox Adventures");
        sprintf((char*)(gSaveCardImageBuffer + 0x20), "Dinosaur Planet");
        break;
    }
#endif
}
#endif


/* Mounts the memory card, validates its serial number, opens or creates the
 * save file (writing the card image buffer for a fresh file), and maps any
 * CARD error to a status code. */
int saveGame(int writeImages) {
    u8 created;
    u8 fresh;
    int result;
    int ok;
    int ret;
    u64 serial;
    CARDStat stat;
    void* m;

    created = 0;
    fresh = 0;
    if (cardProbe(0) == 0) {
        ok = 0;
    } else {
        if ((gSaveCardWorkArea = mmAlloc(0xa000, -1, 0)) == NULL) {
            gSaveCardState = 8;
            ok = 0;
        } else {
            ok = 1;
        }
    }
    if (ok == 0) {
        return 0;
    }
    gSaveCardState = 0;
    result = CARDMount(0, gSaveCardWorkArea, (CARDCallback)cardSetStatusNoCard2);
    if (result == CARD_RESULT_BROKEN) {
        result = CARDCheck(0);
    }
    if (result == CARD_RESULT_READY || result == CARD_RESULT_ENCODING) {
        int err;
        result = CARDCheck(0);
        err = CARDGetSerialNo(0, &serial);
        if (err == CARD_RESULT_READY) {
            if (gSaveCardIdentityCheckEnabled != 0) {
                if (*(u64*)&gSaveCardSerialHi != 0) {
                    if (serial != *(u64*)&gSaveCardSerialHi) {
                        result = -0x55;
                        gSaveCardState = 0xb;
                    }
                } else {
                    *(u64*)&gSaveCardSerialHi = serial;
                }
            } else {
                *(u64*)&gSaveCardSerialHi = serial;
            }
        } else {
            result = err;
        }
    }
    if (result == CARD_RESULT_READY) {
        result = CARDOpen(0, sMemoryCardFileName, &gSaveCardFileInfo.fileInfo);
        if (result == CARD_RESULT_NOFILE && (u8)writeImages == 0) {
            created = 1;
            fresh = 1;
        }
        if (result == CARD_RESULT_READY) {
            gSaveCardFileOpen = 1;
        }
    }
    if (result == CARD_RESULT_READY) {
        result = CARDGetStatus(0, gSaveCardFileInfo.fileInfo.fileNo, &stat);
        if (result == CARD_RESULT_READY) {
            if (stat.iconAddr == 0xffffffff || stat.commentAddr == 0xffffffff) {
                if ((u8)writeImages != 0) {
                    result = CARD_RESULT_NOFILE;
                } else {
                    fresh = 1;
                }
            }
        }
    }
    if (fresh != 0) {
        m = mmAlloc(0x4000, -1, 0);
        gSaveCardImageBuffer = m;
        if (m != NULL) {
            memset(m, 0, 0x4000);
            loadMemCardImages();
        } else {
            gSaveCardState = 8;
            CARDUnmount(0);
            mm_free(gSaveCardWorkArea);
            gSaveCardWorkArea = NULL;
            return 0;
        }
    }
    if (created != 0) {
        result = CARDCreate(0, sMemoryCardFileName, 0x6000, &gSaveCardFileInfo.fileInfo);
    }
    if (fresh != 0) {
        if (result == CARD_RESULT_READY) {
            result = CARDWrite(&gSaveCardFileInfo.fileInfo, gSaveCardImageBuffer, 0x4000, 0);
            if (result == CARD_RESULT_READY) {
                result = CARDWrite(&gSaveCardFileInfo.fileInfo, gSaveCardImageBuffer + 0x2000, 0x2000, 0x4000);
            }
            if (result == CARD_RESULT_IOERROR) {
                CARDDelete(0, sMemoryCardFileName);
            }
            if (created != 0 && result == CARD_RESULT_READY) {
                result = CARDGetStatus(0, gSaveCardFileInfo.fileInfo.fileNo, &stat);
            }
            if (result == CARD_RESULT_READY) {
                stat.commentAddr = 0;
                stat.bannerFormat = (stat.bannerFormat & ~0x3) | 2;
                stat.iconAddr = 0x40;
                stat.bannerFormat = (stat.bannerFormat & ~0x4) | 4;
                stat.iconFormat = (stat.iconFormat & ~0x3) | 1;
                stat.iconSpeed = (stat.iconSpeed & ~0x3) | 3;
                stat.iconFormat = (stat.iconFormat & ~0xc) | 4;
                stat.iconSpeed = (stat.iconSpeed & ~0xc) | 0xc;
                stat.iconFormat = (stat.iconFormat & ~0x30) | 0x10;
                stat.iconSpeed = (stat.iconSpeed & ~0x30) | 0x30;
                stat.iconFormat = (stat.iconFormat & ~0xc0) | 0x40;
                stat.iconSpeed = (stat.iconSpeed & ~0xc0) | 0xc0;
                stat.iconSpeed &= ~0x300;
                result = CARDSetStatus(0, gSaveCardFileInfo.fileInfo.fileNo, &stat);
                if (result == CARD_RESULT_READY) {
                    *(u64*)&gSaveCardChecksumHi = *(u64*)(gSaveCardImageBuffer + 0x3ff8);
                }
            }
        }
        mm_free(gSaveCardImageBuffer);
    }
    switch (result) {
    case CARD_RESULT_READY:
        if (fresh != 0) {
            return 1;
        }
        return 2;
    case CARD_RESULT_UNLOCKED:
        gSaveCardState = 1;
        ret = 0;
        break;
    case CARD_RESULT_NOCARD:
        if ((int)gSaveCardState != 3) {
            gSaveCardState = 2;
        }
        ret = 0;
        break;
    case CARD_RESULT_NOFILE:
        gSaveCardState = 0xc;
        ret = 0;
        break;
    case CARD_RESULT_IOERROR:
        gSaveCardState = 4;
        ret = 0;
        break;
    case CARD_RESULT_BROKEN:
        gSaveCardState = 5;
        ret = 0;
        break;
    case CARD_RESULT_ENCODING:
        gSaveCardState = 6;
        ret = 0;
        break;
    case CARD_RESULT_NOENT:
    case CARD_RESULT_INSSPACE:
        gSaveCardState = 9;
        ret = 0;
        break;
    case -0x55:
        ret = 0;
        break;
    default:
        ret = 0;
        break;
    }
    if (gSaveCardFileOpen != 0) {
        gSaveCardFileOpen = 0;
        CARDClose(&gSaveCardFileInfo.fileInfo);
    }
    CARDUnmount(0);
    mm_free(gSaveCardWorkArea);
    gSaveCardWorkArea = NULL;
    return ret;
}

void cardSetStatusNoCard2(void) {
    gSaveCardState = 0x3;
}
