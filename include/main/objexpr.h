#ifndef MAIN_OBJEXPR_H_
#define MAIN_OBJEXPR_H_

#include "global.h"

typedef struct ObjLookAtControlFlags {
    u8 flip : 1;
    u8 rest : 7;
} ObjLookAtControlFlags;

STATIC_ASSERT(sizeof(ObjLookAtControlFlags) == 1);

extern ObjLookAtControlFlags gObjLookAtControlFlags;

void objSetLookAtFlip(int mode, u8 enabled);

#endif /* MAIN_OBJEXPR_H_ */
