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

The 2026-09-26 pass improves `addShaderLayerStages` from 99.184395% to
99.57447%, and `objSetupRenderOpGxState` from 99.52429% to 99.67612%, in all
five versions. `objFuzzRenderCb` remains exact. The TU remains `NonMatching`.

The animation lookup now declares its material ID and slot traversal together.
The scroll helper takes the `ShaderLayer` and reads its material ID locally;
ordering its locals recovers the second lookup's retail registers. Both lookup
loops now have exact instruction operands, apart from their containing layer
pointer. Canonical shader, texture-reference, layer, color, and matrix types
replace the raw views in the active function, with names describing lighting,
material lookup, and texture scrolling.

There are still 22 instructions with register differences in the 282-instruction
function. LLDB captures of GC/1.3, checked against the ordinary compiler's raw
object hash, reproduce both graph simplification and physical register choices.
The remaining allocation removes the channel-color pointer at degree 31 / cost
15, then the light-count argument at degree 30 / cost 17. These decisions shift
the saved registers for the light count, layer index, layer pointers, and alpha.
Reordering outer declarations, sharing loop indices, extracting larger helpers,
and changing mode casts did not close this gap. No compiler settings changed.

Reproduce the capture with:

```sh
python3 tools/tricky_backend_trace.py --unit main/main/objprint_dolphin \
    --function addShaderLayerStages --graph --output build/add_shader_layers_trace
```

Validation: all five input DOL hashes checked; only the two functions above
change instruction bytes, and those changes are exclusively register operands.
Allocated section layout and non-text bytes are unchanged. The EN
`ninja all_source` and strict retail checksum targets pass.
