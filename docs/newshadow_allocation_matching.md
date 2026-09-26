# allocLotsOfTextures allocator investigation

This investigation starts at `f6a299c172`. It does **not** finish the function:
the retained declaration-order change improves objdiff from **99.132484% to
99.139206%**. The common GC/1.3 compiler, complete TU profile, source boundaries,
and NonMatching status remain unchanged.

## Retained change

In the lightning fill, declare `base` before `lowoff` and `rowoff`. The source
statements, expressions, types, and scopes are unchanged. An exhaustive replay
of the seven existing lightning-local register identities predicted this order;
compiling it confirms the small improvement.

Only five instruction bytes change, all in `allocLotsOfTextures`. Its extent
stays 5,948 bytes / 1,487 instructions. The other 43 function bodies, every
allocated non-text section, named-symbol layout, and relocation record are
unchanged. No whole-unit or new exact-function credit is claimed.

## Remaining differences

Instruction indices below are zero-based within the EN function. The first 692
instructions already match, including both bump-map passes. All floating-point
register operands also agree in the retained source.

| Region | Remaining mnemonic-aligned differences |
| --- | ---: |
| Two I8 ramps, indices 693–796 | 40 register-operand rows |
| Lightning fill, indices 1029–1103 | 9 register-operand rows |
| Reflection gradient, indices 1253–1322 | 42 operand rows and 6 alignment rows |

The six gradient alignment rows describe **three moved address instructions**,
not six missing instructions. The final texture load and two address additions
appear before the last float-to-integer result is extracted; retail puts them
after that extraction and narrowing. Both functions contain 1,487 instructions.

## Verified compiler capture

The existing LLDB capture tool validates the stock compiler SHA-256, compares
the complete instrumented object with ordinary compilation, and validates final
IR against the emitted function. The baseline capture has 413 GPR graph nodes;
simplification and all 380 physical color decisions replay without a forced
high-degree removal.

- Compiler SHA-256:
  `4e502c38465500d4fda8d966b268151a6c74c730508e3d9b7efd23d1a6083715`
- Baseline ordinary/instrumented object SHA-256:
  `d555829baa38265fcdbedf2683ac7c25c40b37f79240d96ce47bdb2c5d1a0d4a`
- Retained EN candidate object SHA-256:
  `469a1cb44759b81d3251a8ef01246341bba9b462f4fa0d266e28f512a2176e8b`

Reproduce a capture of the current source with:

```sh
python3 tools/tricky_backend_trace.py --unit main/main/newshadows \
  --function allocLotsOfTextures --graph \
  --output build/alloc_texture_match/current_gpr
python3 tools/strucdiff.py main/main/newshadows allocLotsOfTextures 4
```

The local baseline capture is `build/alloc_texture_match/gpr/trace.json`.
Compiler virtual-register identities describe the reconstructed source, not
recovered retail variable names.

## Constraints for the next source experiment

In the baseline ramp, the narrowed byte is named source register v74, colored
r6; the column mask is generated register v166, colored r4. Retail needs r4
and r6 respectively. The inverse ramp has the same issue at v69/v154. The
captured graph permits the desired colors if those identities exchange scan
positions, but that diagnostic permutation is not a source solution.

The allocator model from `../mwcc/tools/search_gc13_register_order.py` reproduces
the complete baseline color vector. Exhaustively permuting the first ramp's
five existing local identities (120 orders) does not produce retail colors.
Permuting the lightning loop's seven existing identities (5,040 orders) cannot
finish that region either; the retained order minimizes differing instruction
positions within this fixed-graph family.

Changing the ramp byte local to `int` preserves the complete instruction stream
but moves narrowing to backend-generated registers v277/v288. The capture
comparison finds no mapped interference-edge differences or register-partition
conflicts, while two graph neighbors remain unmapped. The resulting colors are
worse. This identifies an earlier temporary-identity question; it does not
justify assuming that a source declaration can arbitrarily rename a generated
register.

Other tested pointer decompositions, local sharing, casts, helper boundaries,
and loop spellings reproduce the baseline or regress it. Extra gradient value
or pointer assignments frequently prevent the four-row inner loop from fully
unrolling, changing both code and floating-point register allocation. Those
variants are not retained. Further work should establish the relevant frontend
temporary/coalescing boundary before repeating declaration permutations.

## Validation

Fresh before/after compiles for EN, EN revision 1, JP, PAL, and PAL revision 1
give the same function scores and the same five changed instruction bytes.
Each input DOL is checked against its configured SHA-1 before comparison.
Object comparisons preserve all other functions, non-text sections, symbol
layouts, and relocation records in every version. The local commands and
results are recorded in `build/alloc_texture_match/regional_verification.json`.

`ninja all_source`, `ninja build/GSAE01/ok`, and the formatter checks pass.
Formatting introduces no additional source change. The strict DOL gate checks
integration using this NonMatching TU's retail object; objdiff remains the
evidence for the small source-code gain.
