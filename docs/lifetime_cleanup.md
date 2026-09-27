# Lifetime optimization cleanup (2026-09-27)

No production TU uses `nolifetimes`. Its four users now retain their other
compiler settings with lifetime optimization enabled:

- `main/objprint_dolphin.c`
- `main/pi_videoinit.c`
- `dlls/objects/396_MMSH_Shrine/MMSH_Shrine.c`
- `dlls/objects/423/423.c`

The first three need no source changes. In slot 423,
`EdibleMushroom_updateBehavior` reused function-scope `dx` and `dz` locals
across four independent curve searches. Removing the flag swapped floating-point
registers in the last three searches: 18 instruction words changed, with the
operations, function size, data and relocations preserved.

Each search now declares its own `dz`, `dx` and `rangeSq` in the existing
curve-following block. This restores every retail instruction while keeping the
locals with their uses. Declaration order remains significant: declaring `dx`
before `dz` changes register allocation. No TU boundaries, compiler versions,
matching status or other optimization settings change.

Validation:

- All four complete TUs remain 100% code/data exact in EN, EN rev1, JP, PAL and
  PAL rev1 objdiff reports. All five input DOLs were hash-verified.
- All 20 before/after object comparisons preserve section contents, symbol
  offsets and relocations. Only anonymous literal-symbol numbering changes in
  `objprint_dolphin`, slot 396 and slot 423. Existing regional matching manifests
  already include all four TUs.
- Generated compiler commands contain no `nolifetimes`; the other effective
  compiler flags are unchanged.
- `ninja all_source` and the strict EN retail checksum target pass.

Historical reports and general compiler-probe tools still name the flag. Those
records describe previous experiments, not a requirement to disable the pass.
