# Map-owned environment-action index arrays

`main/shader.c` now defines two initialized two-element integer arrays shared
with the environment-action DLLs. They replace 16 bytes previously supplied by
an automatic retail object. This also corrects the sky array's former scalar
declaration and removes pointer arithmetic beyond that scalar.

## Ownership and layout evidence

In EN, the missing arrays occupy `0x803DB610..0x803DB620`, immediately before
the map-state globals already owned by `main/shader.c`. The sky DLL initializes
both words to `-1` and assigns both from its environment-action ID. Object DLLs
409 and 411 read element zero when replaying an environment action. The cloud
action DLL initializes its pair to `-1`, shifts element one into element zero,
and writes the new action index into element one.

The related Dinosaur Planet `map.c` independently defines two zero-initialized
`s32[2]` arrays, `D_80092A7C` and `D_80092A84`, before the corresponding map
state and layer-offset table. Its shrine consumers use the first array in the
same fallback/replay pattern. This corroborates map ownership, the two-element
shape, and declaration order; the retail references establish the actual SFA
addresses and consumers.

The arrays are named `gSky2EnvfxActIndices` and `gCloudActionEnvfxActIndices`.
Both declarations live in the shared environment-action API. The cloud pair's
address-derived name is removed from the DLL-specific header and all five
symbol configurations.

GC/1.3 emits an explicitly zero-initialized eight-byte array into `.sdata`
without section directives or special flags. A zero-initialized scalar instead
uses `.sbss`; that scalar behavior does not apply to these aggregates. Extending
the existing map unit's `.sdata` start by 16 bytes reproduces the complete
60-byte section with its old globals at unchanged offsets in the final DOL.

## Verification

All five versions pass `ninja all_source` and the native strict retail checksum.
Objdiff reports all code and data exact in the defining map unit and all four
direct consumer units: 198 functions total. The map unit now matches all 40,688
of its data bytes. No translation-unit boundary, compiler profile, or generated
DLL path changes.

| Version | Sky pair | Cloud pair | Map small-data end |
| --- | --- | --- | --- |
| GSAE01 | `0x803DB610` | `0x803DB618` | `0x803DB64C` |
| GSAE01_rev1 | `0x803DC270` | `0x803DC278` | `0x803DC2AC` |
| GSAJ01 | `0x803DB730` | `0x803DB738` | `0x803DB76C` |
| GSAP01 | `0x803DCE00` | `0x803DCE08` | `0x803DCE3C` |
| GSAP01_rev1 | `0x803DCFC0` | `0x803DCFC8` | `0x803DCFFC` |

The regional projection tool independently reproduces the expanded range in each
secondary version. The small-data operand audit confirms eight retail references
to the sky pair and four to the cloud pair in each version, with identical
initialized bytes. Unrelated verified regional configurations are preserved.

