# Explicit-zero-data cleanup (2026-09-27)

All game MWCC units now use the default zero-data setting. The five
`explicit_zero_data on` overrides and their two dedicated compiler profiles
are removed. Library profiles, other game flags, compiler versions, TU
boundaries and matching status are unchanged.

## CloudRunner: a single segment's radius array

`DR_CloudRunner_setupPath` passes one segment to `curves_setSegmentCollision`,
whose implementation indexes the supplied radius array once per segment.
Represent mode 0's input as `f32 gDRCloudRunnerMode0SegmentRadii[1] = {0.0f}`
and pass the array directly. This is an independently addressed four-byte
object; it does not absorb the preceding mode-2 local radius or the following
two local-point radii.

GC/1.3 treats this initialized global array differently from a zero scalar:
it retains the array in `.sdata` with the default setting. All section bytes,
section sizes, symbol offsets and instructions are unchanged. The only object
differences are the renamed symbol and its one relocation. Path setup remains
508 bytes, and all 36 CloudRunner functions remain exact.

The mode-2 two-radius request still overruns its separate four-byte local radius
into this neighboring word. Neither the allocation nor that behavior is changed.
The older ownership audit correctly established separate storage, but its direct
address evidence could not distinguish a scalar from a one-element array. The
array matches both the API's count and the retail object without the override.

## pi_dolphin: trailing alignment padding

Remove the unused `lbl_803DB5E4 = 0` declaration and its regional symbol and
force-active entries. The final `.sdata` object is the seven-byte `"PC: %x"`
string. Its end is followed by five zero bytes before the next eight-byte-aligned
TU. The linker supplies those bytes without a source-level integer definition.

All five verified retail DOLs have the same string/padding/next-object sequence.
The removed word has no discovered SDA, address-construction or raw-pointer
references. The source section shrinks from 328 to 323 bytes; every surviving
symbol offset and relocation is unchanged. The extracted target retains the
trailing bytes as an anonymous gap, and objdiff reports all code and data exact.
The final EN DOL checksum independently confirms the padding reconstruction.

## Other users and common setting

`objprint_dolphin.c`, `pi_videoinit.c` and `pi_pathsearch.c` need no source edits.
Their complete before/after objects are byte-identical in all five versions.

The wider audit also tested enabling the flag on the other 773 game units.
Six units move referenced zero aggregate-initializer templates from `.sbss2`
to `.sdata2`: `shader.c`, `intersect_render.c`, engine slots 5 and 6,
DIMExplosio (458), and ECSH_Shrine (399). The default setting preserves these
existing matches. This audit establishes that the current source can share the
setting; it does not establish the original compiler command lines.

## Validation

- All five input DOL hashes verified against their configured hashes.
- All 25 affected unit/version combinations are 100% code and data exact in
  objdiff, including EN rev1, JP, PAL and PAL rev1. Their existing regional
  matching manifests already include these units.
- All 15 objects for the three source-unchanged units are byte-identical.
  CloudRunner's five objects differ only by the symbol rename; pi_dolphin's
  five differ only by the removed padding declaration and compiler metadata.
- `ninja all_source` and the strict EN retail checksum target pass, with
  30-second timeouts. The EN DOL remains byte-identical to retail.
- Complete retail links and links substituting all five selected source units
  reproduce the original DOLs for EN rev1 and JP.
- PAL and PAL rev1 full-link checks time out in the all-retail baseline, before
  source substitution. Their object comparisons pass; no complete PAL source
  link is claimed here.
- `clang-format -i` makes no additional changes to either edited TU or the
  CloudRunner header; all three pass `--dry-run --Werror`.
- Production game commands contain no `explicit_zero_data` override. The
  Dolphin SDK's existing `OSError.c` pragma is outside the game-code scope.
