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

typedef struct {
    int key;
    int val;
} SeqSortPair;

static inline int maketex_indexOf(int* p, int n, int target) {
    int i;
    int j;
    i = 0;
    for (j = 0; j < n; j++) {
        if (*p++ == target) {
            return i;
        }
        i++;
    }
    return -1;
}
int arrayRemoveUnordered(int* array, int* count, int value) {
    int i;
    int len;
    len = *count;
    i = maketex_indexOf(array, len, value);
    if (i == -1) {
        return -1;
    }
    array[i] = array[len - 1];
    (*count)--;
    return i;
}

int arrayIndexOf(int* arr, int count, int target) {
    int idx = 0;
    int i;
    for (i = 0; i < count; i++) {
        int elem = *arr;
        arr++;
        if (elem == target) {
            return idx;
        }
        idx++;
    }
    return -1;
}

static inline int seqPairKey(SeqSortPair* pair) {
    return pair->key;
}

static inline int seqPairVal(SeqSortPair* pair) {
    return pair->val;
}

void seqPairTableSort(SeqSortPair* arr, int n) {
    int key;
    int val;
    int limit;
    int i;
    int j;
    int gap;

    gap = 1;
    limit = (n - 1) / 9;
    while (gap <= limit) {
        gap = gap * 3 + 1;
    }
    for (; gap > 0; gap /= 3) {
        for (i = gap + 1; i < n; i++) {
            key = seqPairKey(&arr[i]);
            val = seqPairVal(&arr[i]);
            j = i;
            while (j > gap && arr[j - gap].key > key) {
                arr[j].key = arr[j - gap].key;
                arr[j].val = arr[j - gap].val;
                j -= gap;
            }
            arr[j].key = key;
            arr[j].val = val;
        }
    }
    for (i = 1; i < n; i++) {
    }
}

int seqPairTableLookup(void* entries, int count, int key) {
    SeqSortPair* arr = entries;
    int lo, mid;
    int i;
    if (count <= 16) {
        for (i = 0; i != count; i++) {
            if (arr->key == key) {
                return arr->val;
            }
            arr++;
        }
        return 0;
    }
    lo = 0;
    do {
        mid = (count + lo) >> 1;
        if (key > arr[mid].key) {
            lo = mid;
        } else if (key == arr[mid].key) {
            return arr[mid].val;
        } else {
            count = mid;
        }
    } while (count <= lo);
    return 0;
}

/* Spin-delay then sort when the pair list is large enough. */
void seqPairTablePrepare(void* entries, int n) {
    SeqSortPair* arr = entries;
    int i;
    int j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
        }
    }
    if (n > 0x10) {
        seqPairTableSort(arr, n);
    }
}

int randomChanceOneIn(int n) {
    return randomGetRange(0, n * 60 / 60) == 0;
}

int timerIsActive(const f32* p) {
    return 0.0f != *p;
}

void storeZeroToFloatParam(f32* p) {
    *p = 0.0f;
}

void s16toFloat(f32* p, s16 val) {
    *p = (f32)val;
}

int timerCountDown(f32* p) {
    f32 timer = *p;
    f32 zero = 0.0f;
    if (timer != zero) {
        *p = timer - timeDelta;
        if (*p <= zero) {
            *p = zero;
            return 1;
        }
    }
    return 0;
}
