# How GC/1.3 orders a TU's `.sdata2`, and what the orphan pool ranges were

Traced 2026-09-27 against the stock GC/1.3 `mwcceppc.exe`
(`4e502c38465500d4fda8d966b268151a6c74c730508e3d9b7efd23d1a6083715`) under LLDB in the
`../mwcc` offline Docker/QEMU sandbox; every traced object was byte-identical to an untraced
compile. Addresses below are compiler virtual addresses.

## Section choice for a named `const`

`GC13_Storage_Prepare(object, size, alternate)` (`0x4b31d0`, recovered in
`../mwcc/src/versions/GC_1_3/StoragePrepare.c`) picks the read-only small section
(`.sdata2`) only when `alternate` is 1, and it returns early once `object->section > 0`, so
the first caller wins. The definition path (`0x4b2f7b`) passes 1. The code-reference path
in the storage classifier (`0x4b4030`) always pushes 0. A named constant that code
references before its definition is processed is therefore placed in `.sdata`.

## Folding

The front end folds every read of a `const` scalar whose initializer it has already seen.
`-opt off` does not change this; neither does a braced initializer, an integer
initializer, taking the address, or `static`. A const aggregate is not folded, but under
CSE its second field is reached through a base register rather than `sym+4`.

## Creation order

A literal's slot follows its creation: float literals are interned by the expression
preparation pass (call at `0x433739`, return `0x43373e`); int-to-float conversion doubles
are created later in lowering (call at `0x433936`, via `0x44f631` -> `0x4e007d` ->
`0x4e0192`). Within one function, floats always precede that function's conversion
doubles. Named definitions enter the section when they are processed.

## Deferred TUs

`-inline ...,deferred` generates code only after the whole TU is parsed, emitting
functions in reverse source order. That reconciles everything retail shows for several
DLLs:

- The source runs top-down (descriptor callbacks first, helpers last). Retail `.text`
  order is the reverse.
- Named constants are declared `extern const` at the top and defined just before the
  `ObjectDescriptor`. Reads are parsed before the initializer is known, so they stay
  unfolded, and the definition runs before any code reference, so they land in `.sdata2`
  **ahead of** every literal.
- With `auto` inlining, a small `static` helper is inlined at every call site, but its
  out-of-line copy is still generated, and generated first when it is last in the file.
  mwld strips the copy (class A in `dead_strip_census.md`); its pool entries remain. That
  is why a retail pool can open with conversion doubles that no live function creates
  first. Such a helper must create only what the pool shows first: a bare
  `(f32)value` helper for a leading conversion double.

Recovered this way: `MagicPlant` (two stripped event helpers), `284` (the shared
`gStaffReaction*` constants that `283` reads), `LanternFire`, `SpiritDoorL`,
`BombPlantSp`, `75`. `InvisibleHi` and `SpiritDoorL` also needed `x / 64.0f`: MWCC turns a
power-of-two division into a multiply by the reciprocal, which retail's `0.015625` came
from, and a `* 0.015625f` literal is hoisted differently.

## Unresolved external constant declarations

C rejects `extern f32 x;` followed by `const f32 x = ...;` in one TU. For
`WCBouncyCra`, plain `extern f32` declarations reproduced retail's code, while
changing them to `extern const` changed register allocation. This was previously
interpreted as evidence that the definitions belonged to another TU. That
conclusion was too strong: the qualifier affected the reconstructed expression,
and the same-TU recovery below preserves both code and data.

`WCPushBlock`, `650`, `WORLDAstero`'s trailing pair and `ARWArwing`'s tuning values
still need ownership recovery. Neither an external declaration in reconstructed
source nor a qualifier-induced register change establishes an original boundary.

## Bouncy-crate constant ownership (2026-09-29)

The existing `WCBouncyCra.c` now defines its nine bounce parameters before the
functions and its cell-margin parameter immediately before the shared cell test.
Every reference to these ten symbols belongs to this TU. The complete pool is
52 bytes: nine floats, four bytes of natural double alignment, the cooldown's
conversion double, and the cell-margin float. Its descriptor remains last.

The reads use `*(const f32*)&name`, as already used by `rcp_dolphin.c`, to preserve
the named objects without anonymous literal duplicates. Plain scalar reads and
`*&name` or `(&name)[0]` fold the values and emit a second pool. Making these
definitions `static` also changes pool order by deferring emission until use;
external linkage is retained. No forced section, dummy data, extra source file,
or compiler-profile change is needed.

The local copy of the nearest-object distance is `const f32`. This retains the
retail floating-point register allocation after the constants gain their correct
qualifiers; the unqualified copy swaps two registers in the falloff calculation.
This is a verified reconstruction, not a claim to know the original spelling.

| Version | Complete `.sdata2` range |
| --- | --- |
| GSAE01 | `803E6D20..803E6D54` |
| GSAE01_rev1 | `803E79B8..803E79EC` |
| GSAJ01 | `803E6E40..803E6E74` |
| GSAP01 | `803E8550..803E8584` |
| GSAP01_rev1 | `803E8718..803E874C` |

All five versions retain ten exact functions and now own 108 exact data bytes,
up from 64. Both `ninja all_source` and the native strict retail checksum pass
with the complete pool linked from source. The adjacent push-block constants
remain outside this unit.
Regional projection reproduces the full unit's ranges in each secondary target.
An independent retail operand audit confirms all ten constants, their bytes,
and fourteen paired r2-relative references per secondary version.

## Asteroid render-scale recovery (2026-09-29)

The renderer's separate `gWorldAsteroidsRenderScale` reference does not need an
external definition. Replacing it with the `1.0f` render argument preserves all
nine functions and emits the exact eight-byte prefix at EN
`0x803E65D0..0x803E65D8`: the scale literal followed by natural alignment for the
existing conversion double. The claimed pool now spans
`0x803E65D0..0x803E65EC` (28 bytes), and the complete unit owns 84 exact data bytes.
The preceding word at `0x803E65CC` remains outside this unit.

The two trailing orbit constants are a different case. Replacing both with
`80.0f` and `145.0f` moves the radius multiplication and integer conversion past
the random-number calls, requires saving an additional floating-point register,
and changes their pool order. That probe is not retained; those definitions
still require recovery. No artificial data definitions or compiler changes are
used for the render scale.

All five versions pass `ninja all_source` and the native strict retail checksum
with the expanded pool linked from source. Each has nine exact functions and
84 exact data bytes. Regional projection reproduces the expanded ranges, and
the small-data operand audit confirms the scale's retail address and bytes in
each secondary version. The source and canonical header pass clang-format.
