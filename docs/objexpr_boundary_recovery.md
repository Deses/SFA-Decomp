# Expression and object-rendering boundary recovery

The former `main/objprint.c` combined 31 expression, sound, eye, and head-tracking
functions with nine rendering functions. The expression group now lives in
`main/objexpr.c`. Both units retain the original GC/1.3 compiler and the same
optimization profile. The look-at control byte is defined by its owning source
instead of supplied by a retail fallback object.

## Evidence

The EN text boundary is `0x8003B5E0`, between `characterHeadLookCalm` and
`objSetGlowColor`:

- All 91 retail relocations to the merged unit's expression data and external
  look-at flag originate in the first 31 functions. These cover the joint-key
  table, head-look jump table, two initialized small-data globals, literal pool,
  and flag. No rendering function references this data.
- All ten relocations to the merged unit's owned small-BSS originate in the
  nine rendering functions. These refer to the filter RGB values, glow RGBA
  values, enable bytes, and matrix override pointer.
- The expression flag is at `0x803DCC00`; rendering state begins independently
  eight-byte aligned at `0x803DCC08`. The normal compiler emits a one-byte flag
  section and an 18-byte rendering section. No forced alignment or padding
  declaration is needed to reproduce the retail addresses.
- The expression object emits the existing 68-byte table/jump-table data,
  eight-byte initialized small-data section, and 88-byte literal pool unchanged.
  The rendering object emits none of these sections. The old data extent also
  included four bytes of trailing alignment; its small-BSS extent included six.

The related Dinosaur Planet `objexpr.c` independently contains the same ten
joint keys, a one-bit look-at control, and the corresponding expression helpers.
Its `objprint.c` owns the model color multipliers, blend colors, and matrix
override. This corroborates the subsystem boundary and chosen filename. The
retail source-name inventory and cross-bundle source matrix contain no direct
`objexpr` or `expr` tag, so this does not claim a recovered GameCube filename.

The public look-at declaration moves from the misleading `objlib_api.h` to
`objexpr.h`. Its flag type retains its byte-sized bitfield layout, now with a
size assertion. The direct caller includes the new header. Unused imports of
the old header are removed without broader consumer cleanup. Existing rendering
parameter declarations, including the promoted K&R byte alpha parameter, are
preserved.

## Verification

All five versions pass `ninja all_source` and the native strict retail checksum
with both objects linked from source. Objdiff reports all 31 expression functions
(11,352 code bytes, 165 data bytes) and nine rendering functions (1,444 code bytes,
18 data bytes) exact in every version.

| Version | Expression text span | Look-at flag | Rendering state |
| --- | --- | --- | --- |
| GSAE01 | `0x80038988..0x8003B5E0` | `0x803DCC00` | `0x803DCC08` |
| GSAE01_rev1 | `0x80038A80..0x8003B6D8` | `0x803DD880` | `0x803DD888` |
| GSAJ01 | `0x800389A8..0x8003B600` | `0x803DCD20` | `0x803DCD28` |
| GSAP01 | `0x80038B1C..0x8003B774` | `0x803DE3F8` | `0x803DE400` |
| GSAP01_rev1 | `0x80038B1C..0x8003B774` | `0x803DE5B8` | `0x803DE5C0` |

The regional projection tool independently reproduces all new section ranges.
The small-data operand audit confirms the flag through four retail references
and each of the ten rendering globals through its store in every secondary
version. Existing verified configurations outside these units are preserved.

This recovers one byte of live state from the retail fallback objects and
distinguishes object extents from trailing alignment. It does not resolve the
remaining PAL `askProgressiveScanMode` register-allocation mismatch or the
unrelated external data pools.
