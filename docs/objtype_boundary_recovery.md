# Object-type translation-unit boundary recovery

The former `main/objlib.c` combined the nine object-type functions with the
following 26 message, contact, trigger, and path helpers. The object-type
functions now live in `main/objtype.c`, with the same compiler and optimization
profile as before. Their byte-sized list count and the remaining library's
32-bit callback count are real source definitions instead of external references
to automatic retail objects.

## Evidence

The EN boundary is `0x80037484`, between `objTypeInit` and `ObjMsg_Peek`.
Several independent section boundaries support it:

- The object-type list and index table occupy `0x803428F8..0x80342D50`;
  the callback array begins at `0x80342D50` and occupies the next 192 bytes.
- The object-type diagnostic is 38 bytes at `0x802CAE20`. The message diagnostic
  starts at `0x802CAE48`, after normal alignment padding.
- The nearest-type functions share a four-byte maximum-float literal at
  `0x803DE968`. The later helper pool starts at `0x803DE970`, independently
  eight-byte aligned. No reference uses the intervening zero word.
- The type-list count is a byte at `0x803DCBF0`; the callback count is a word at
  `0x803DCBF8`. Separate small-BSS sections reproduce these addresses without
  invented eight-byte objects or forced variable alignment.

The related Dinosaur Planet source independently places the object-type list,
index table, count, and eight corresponding functions in `objtype.c`; its
callback array and count belong to `objlib.c`. This supports the subsystem
boundary and the chosen filename. It does not establish an original GameCube
source filename: the retail source-name inventory and cross-bundle source
matrix contain no direct `objtype` or `objlib` file tag.

The old merged source explicitly defined an unused `gObjLibZero` and forced it
active to occupy the word between the two literal pools. With the separated
objects, the linker supplies that word as alignment padding. The synthetic
constant and all five force-active entries are removed.

The source keeps the existing byte count and 256-entry capacity behavior,
including its wraparound limitation. This recovery does not change that logic.
The symbol configurations now record the counts' accessed widths (one and four
bytes), rather than including the following alignment gaps in their sizes.

## Regional boundaries

| Version | Object-type text span | Type count | Callback count |
| --- | --- | --- | --- |
| GSAE01 | `0x80036c0c..0x80037484` | `0x803dcbf0` | `0x803dcbf8` |
| GSAE01_rev1 | `0x80036d04..0x8003757c` | `0x803dd870` | `0x803dd878` |
| GSAJ01 | `0x80036c2c..0x800374a4` | `0x803dcd10` | `0x803dcd18` |
| GSAP01 | `0x80036da0..0x80037618` | `0x803de3e8` | `0x803de3f0` |
| GSAP01_rev1 | `0x80036da0..0x80037618` | `0x803de5a8` | `0x803de5b0` |

## Verification

All five versions pass `ninja all_source` and their strict retail checksum with
both units linked from source. Objdiff reports all nine object-type functions
(2,168 code bytes, 1,155 data bytes) and all 26 remaining library functions
(5,380 code bytes, 300 data bytes) exact in every version.

The regional projection tool independently reproduces every new section range.
The small-data operand audit confirms the type count through seven retail
references and the callback count through eight references in each secondary
version. Existing verified configurations outside these two units are preserved.

The split removes five bytes of referenced count storage from the retail fallback
objects. It also distinguishes alignment from object sizes: the old counts each
claimed eight bytes, and the merged unit counted the inter-pool zero and the
string's trailing alignment as source data. Progress denominators therefore
change as well; the verified source definitions and exact final DOLs are the
measure of this recovery.
