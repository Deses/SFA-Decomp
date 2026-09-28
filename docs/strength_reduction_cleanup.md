# Strength-reduction cleanup (2026-09-27)

No production TU uses `nostrength`. Its five users now use the common
strength-reduction setting; the three unused override profiles are removed.
Other compiler flags, compiler versions, TU boundaries and matching status
are unchanged.

- `main/audio.c`: `musicInitMidiWad` reads the packed WAD size table through a
  byte offset scaled by `sizeof(int)`. The ordinary `int`-array spelling changed
  the loop's induction bookkeeping and register allocation with strength
  reduction enabled. The byte-offset read restores the complete object.
- `main/vecmath.c`: `mtx44_multSafe` uses consistent byte-pointer views for its
  matrix accesses and copy, replacing every pointer-through-`int` calculation
  in the function. Named row/column offsets, `sizeof(f32)` strides and a local
  result-element pointer preserve the retail accumulation and addressing.
  Direct float-array indexing changes both the multiplication loop and the
  final copy, so it is not interchangeable for matching this implementation.
- `dlls/objects/399_ECSH_Shrine/ECSH_Shrine.c`: each of the two cup-slot rotation
  loops takes a local pointer to the slot array. This preserves the retail
  unrolled accesses without the extra pointer increments generated for the
  repeated `puzzle->cupSlotMap[index]` spelling.
- `dlls/objects/462/462.c`: the contact scan keeps the engine record's base as
  a 32-bit address. Narrow casts at the count and object-pointer reads retain
  the canonical record layout and `offsetof` expression while preserving the
  retail indexed load. A moving typed pointer produces a different sequence.
- `dlls/objects/512/512.c`: no source change is needed.

Validation:

- All five input DOLs were hash-verified. All five affected TUs remain 100%
  code/data exact in EN, EN rev1, JP, PAL and PAL rev1 objdiff reports.
- All 25 complete before/after objects are byte-identical, including symbol
  tables and relocations. Existing regional matching manifests already include
  all five units.
- The effective compiler flags differ only by removal of `nostrength`, and
  generated build commands contain no remaining uses.
- `ninja all_source` and the strict EN retail checksum target pass.

Historical reports and general compiler probes still name the flag. These
source comparisons establish that the override is unnecessary for the current
reconstruction; they do not establish the original build settings.
