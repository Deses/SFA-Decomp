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
