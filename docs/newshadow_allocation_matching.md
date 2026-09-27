# allocLotsOfTextures allocator investigation

The current function reaches **99.169470%**, up from **99.132484%** at the
start of this investigation (`f6a299c172`). It remains incomplete. The initial
lightning declaration-order change reached **99.139206%**; the gradient follow-up
below provides the next improvement. The common GC/1.3 compiler, complete TU
profile, source boundaries, and NonMatching status remain unchanged.

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

## Gradient follow-up (2026-09-27)

Starting from `76ddcbaff9`, separate the gradient's pixel and tile pointers,
pack the horizontal component directly into the store expression, and use
`y * 8` for the row offset. The four-row loop guarantees that this offset equals
`(y & 3) * 8 + (y >> 2) * 32`. These changes keep the complete inner loop
unrolled and improve objdiff from **99.139206% to 99.156020%**.

LLDB captures reproduce the ordinary object exactly. Comparing their virtual
register roles shows that the four unrolled pointer chains now have separate
identities for the pixel and tile addresses. The cached horizontal component
moves from r5 to the retail r8 at instruction indices 1288, 1292, 1301, 1310,
and 1319. There are still 42 gradient operand-difference rows and three moved
address instructions: some rows now have fewer wrong operands. The ramp and
lightning regions are unchanged, as are all floating-point register operands.

The frontend trace and the recovered GC/1.3 routines in `../mwcc` explain why
simple copy chains fail to fix the ramps: expression propagation removes the
copies, then the shared-expression pass creates fresh mask temporaries. The
observed mask objects return from the factory at `0x4f4200` to `0x467861`,
inside the recovered shared-temporary routine. Likewise, propagating the
gradient's pointer definitions duplicates their expressions before the shared
pass materializes new address temporaries; splitting the pointers alone does
not fix the final store's evaluation order. These are compiler addresses and
observations about the reconstructed source, not retail source provenance.

The retained candidate's graph has 421 GPR nodes and 388 replayed physical
color decisions, with no forced high-degree removals. Ordinary and traced
objects have SHA-256
`abc4556a28d7d1c517758f65a5de761ea374f7d91f203032650bae5a86cbf503`.
Local captures and comparisons are under `build/newshadows_alloc_round2/`.

Fresh before/after compiles reproduce the same improvement in all five retail
versions, after verifying each original DOL's configured SHA-1. Only
`allocLotsOfTextures` changes: 27 bytes in 17 instructions, still 5,948 bytes
total. All other 43 function bodies, allocated non-text sections, and named
symbol layouts remain identical. Twenty relocation references acquire new
anonymous compiler labels; their offsets, types, addends, target sections, and
target offsets are unchanged. The local evidence is
`build/newshadows_alloc_round2/regional_verification.json`.

`ninja all_source`, a fresh `ninja build/GSAE01/ok`, and formatter checks pass.
The formatted source reproduces the traced object byte for byte.

## Lightning lifetime follow-up (2026-09-27)

Compute the lightning column mask and centered coordinate inside the pixel
loop, leaving the tile-column offset outside it. Both expressions depend only
on the outer coordinate; GC/1.3 moves their calculations back out of the loop
and preserves the complete instruction sequence. The different source
lifetimes separate the initial column mask from the combined column offset.
Declare `lowoff`, `base`, `off2`, `off`, `rowoff`, `i`, and `j` in that order.

A verified LLDB capture and exhaustive replay of the seven existing local
identities predict this declaration order. Ordinary compilation confirms
**99.169470%**, versus **99.156020%** before this change. Only eight instruction
bytes change, across seven instructions in the lightning fill. All 1,487
instructions remain; the rest of the function and all 43 other functions are
unchanged. The complete unit reaches **99.770580%**, with 43/44 functions exact.

The remaining differences are 40 operand rows in the ramps, eight in lightning,
and 42 in the gradient, plus the gradient's three displaced address
instructions. Named-local ordering alone does not finish the captured lightning
graph. Further pointer, scalar-field, inline-helper, conversion, and loop-shape
experiments did not produce a complete source match. The Dinosaur Planet
reference has no counterpart for these GameCube procedural texture fills.
Compiler profiles and `NonMatching` status remain unchanged.

Fresh baseline/candidate compiles reproduce the same eight changed bytes and
score improvement in all five versions, after checking each original DOL's
configured SHA-1. Allocated non-text sections and named symbol layouts are
identical. Twenty relocation references acquire new anonymous labels; their
offsets, types, addends, target sections, and target offsets are unchanged.
`ninja all_source` and a fresh strict EN checksum pass. Local verification and
compiler captures are under
`build/newshadows_finish/`; the retained ordinary/traced object SHA-256 is
`c33b9e8303bcfc025a3ce33be37d58607f56484c339a84d5c403550519ae7916`.
