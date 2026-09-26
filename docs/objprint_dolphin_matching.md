# objprint_dolphin matching

## objFuzzRenderCb

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

## addShaderLayerStages

`addShaderLayerStages` matches all 1,128 bytes (282 instructions) under the
existing GC/1.3 TU profile in EN v1.0, EN rev1, JP, PAL v1.0, and PAL rev1.
The 2026-09-26 follow-up improves it from 99.57447% to 100%.

The final change removes five redundant `(u8)` casts from `blendMode`, which
already has type `u8`, and declares the layer index, current layer, and previous
layer before the alpha and texture locals. The rendering operations and control
flow are unchanged. No compiler settings or TU boundaries change.

The casts were significant to allocation despite being redundant in C. With
them, the compiler graph contained 137 nodes, including five additional
coalesced temporaries. Simplification selected the channel-color pointer and
then the light-count argument as high-degree removals, leaving 22 instructions
with different register operands. Removing the casts reduces the graph to 132
nodes and eliminates those forced choices. A replay of this graph predicts the
retained local declaration order; ordinary compilation confirms the exact match.
Removing the casts alone does not match.

The final LLDB capture agrees with the ordinary compiler's complete raw object
hash, aligns all 282 instructions with retail, and replays all 95 physical
register choices with no high-degree removals. Reproduce it with:

```sh
python3 tools/tricky_backend_trace.py --unit main/main/objprint_dolphin \
    --function addShaderLayerStages --graph --output build/add_shader_layers_trace
python3 tools/unitfuzzy.py objprint_dolphin --symbol addShaderLayerStages
```

Validation on 2026-09-26:

- Each of the five input DOL hashes verified against its version configuration.
- Fresh before/after objects report 100% for this function in all five versions.
  Only its 25 instruction bytes change; all other functions, allocated section
  layouts, non-text contents, named-symbol layouts, and relocations are unchanged.
- `objFuzzRenderCb` remains exact; `objSetupRenderOpGxState` remains 99.67612%.
- The EN `ninja all_source`, strict retail checksum target, and formatter checks
  pass. Formatting preserves the raw object.

The complete TU remains `NonMatching`; other functions still have differences.
The exact function does not establish the original local declaration order.
