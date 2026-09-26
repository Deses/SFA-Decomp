# objFuzzRenderCb matching

`objFuzzRenderCb` matches all 2,780 bytes (695 instructions) under the existing
GC/1.3 TU profile in EN v1.0, EN rev1, JP, PAL v1.0, and PAL rev1.

The remaining EN difference was 24 register operands: the projected-light flag
used `r30` instead of `r28`, while the TEV stage index used `r28` instead of
`r30`. Extracting the short-circuit light check into the explicit inline
`objFuzzHasProjectedLight` helper reproduces retail allocation. Its `u8` return
preserves the byte-width tests; inlining adds no call. The local is named
`hasProjectedLight` to describe its role in both stage setup and final counts.
This establishes a matching source spelling, not proof of the original helper.

Validation on 2026-09-26:

- Input DOL hashes verified against each version's configuration.
- `tools/unitfuzzy.py`: callback 100% in all five versions; EN improved from
  99.82734%. No other EN function changed score or instruction bytes.
- Allocated section sizes, alignments, and non-text contents unchanged.
- `clang-format --dry-run --Werror` passed for the TU and its internal header.
- `ninja all_source` and the EN strict retail checksum target passed.

The complete TU remains `NonMatching`; other functions still have differences.
