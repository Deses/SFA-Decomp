# SDK BSS alignment and redundant padding objects

`OSReboot.c` already declares its 32-byte apploader header with 32-byte alignment.
`dvd.c` likewise aligns its 128-byte DVD transfer buffer to 32 bytes. Both source
objects emit `.bss` with alignment 32 in every retail version. The split configs
now record that existing alignment, so DTK lets the linker supply the intervening
zeros instead of generating separate retail padding objects.

The source declarations and compiler profiles are unchanged. The two SDK objects
retain all 49 exact functions and all 624 data bytes. This recovers no new C
functions or variables: it removes unnecessary binary inputs from the link and
corrects the progress denominator's classification of alignment bytes.

## Evidence

Before the correction, a complete EN source-object link matched retail both with
and without either BSS padding object. A complete extracted-object link required
those objects, because its generated sections lacked the source's alignment.
Adding `align:32` to the two BSS splits removes the automatic BSS padding units
while preserving the final DOL exactly. The storage is used for DVD reads and
cache operations; the alignment is independently present in the SDK source,
not chosen merely to improve the report.

| Version | Apploader BSS start | Preceding padding bytes | DVD BSS start | Preceding padding bytes |
| --- | --- | --- | --- | --- |
| GSAE01 | 0x803AD3C0 | 12 | 0x803ADF00 | 16 |
| GSAE01_rev1 | 0x803AE020 | 12 | 0x803AEB60 | 16 |
| GSAJ01 | 0x803AD4E0 | 12 | 0x803AE020 | 16 |
| GSAP01 | 0x803AEB60 | 12 | 0x803AF6A0 | 16 |
| GSAP01_rev1 | 0x803AED20 | 12 | 0x803AF860 | 16 |

All five versions pass `ninja all_source` and the native strict retail checksum
with no automatic BSS objects left. The other two tested zero-filled data gaps
still affect the source link and are retained.

The regional projection tool now parses and preserves explicit split alignment.
It rejects a projected start incompatible with that alignment instead of
silently discarding the requirement. Regression tests cover aligned round trips
and incompatible projections; all 44 version-progress tests pass.

Projection independently reproduces both complete SDK split blocks, including
their alignment, in all four secondary versions. The corrected EN extracted
objects also link exactly without either BSS padding object.

## DSP program and boot/FIFO data alignment (2026-09-29)

The two remaining data gaps also came from missing alignment requirements.
Unlike the BSS cases above, these required correcting the source declarations
as well as the extracted-object alignment:

- `CARDUnlockProgram` now has `ATTRIBUTE_ALIGN(32)`. Its complete 352-byte
  program is identical to `CardData` in the Mario Party 4, Metroid Prime, and
  Pikmin 2 SDK references, each explicitly aligned to 32 bytes. SFA passes the
  program directly to the DSP task's `iram_mmem_addr`, with length `0x160`.
- `gLoadingScreenTextures` now has `align:32` in the symbol configs. DTK uses
  that annotation to generate its byte-array declaration with
  `ATTRIBUTE_ALIGN(32)`. `videoInit` copies the textures elsewhere, then passes
  the original buffer address to `GXInit` as its FIFO. The SDK's
  `GXInitFifoBase` explicitly requires a 32-byte-aligned base. All five retail
  addresses satisfy this requirement.

Both `.data` splits also record `align:32`. No asset bytes, compiler profiles,
unit boundaries, or expected checksums change. The boot split remains a
representation of contiguous recovered data, not a claim about its original
translation-unit boundary.

| Version | DSP program start | Preceding padding | Boot/FIFO start | Preceding padding |
| --- | --- | --- | --- | --- |
| GSAE01 | `8032EBE0` | 16 | `802CC6A0` | 16 |
| GSAE01_rev1 | `8032F840` | 24 | `802CD260` | 24 |
| GSAJ01 | `8032ED00` | 16 | `802CC7C0` | 24 |
| GSAP01 | `80330380` | 24 | `802CDD40` | 0 |
| GSAP01_rev1 | `80330540` | 24 | `802CDE80` | 0 |

All five versions pass `ninja all_source` and the native strict retail checksum.
The six card-unlock functions and 360 data bytes remain exact, as do the boot
unit's 262584 data bytes. The linker now supplies the padding naturally; no
standalone automatic padding objects remain. This removes 32, 48, 40, 24, and
24 bytes respectively from the unowned-data totals, leaving 356 unscored bytes
in each version. It recovers alignment, not new executable code or asset data.

Independent links using only extracted retail objects also reproduce EN,
EN rev1, and JP exactly with these padding objects absent.
The optional all-extracted PAL links hit the 30-second linker timeout; the
normal PAL matching links above both pass. Regional projection reproduces both
complete split blocks and the boot-buffer symbol alignment in all four
secondary versions.
