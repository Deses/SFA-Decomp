#ifndef MAIN_RENDER_MODE_API_H_
#define MAIN_RENDER_MODE_API_H_

#include "types.h"

/* Pool-update marker: 0 idle, 1 expgfx, 2 engine 11. */
extern u8 gExpgfxUpdatingActivePools;

s16 renderModeSetOrGet(int mode);

#endif /* MAIN_RENDER_MODE_API_H_ */
