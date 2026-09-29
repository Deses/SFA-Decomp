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

`WORLDAstero`'s trailing pair still needs ownership recovery.
Neither an external declaration in reconstructed
source nor a qualifier-induced register change establishes an original boundary.
The former `650` gap is resolved by the independently aligned pools and handler
ownership at the [650 / 651 boundary](dll_650_651_boundary.md).
The push-block pool is resolved by the [652 / 656 boundary](dll_652_656_boundary.md).
Arwing's tuning values belong to its existing TU, as recovered below.

## Bouncy-crate constant ownership (2026-09-29)

The existing `WCBouncyCra.c` defines its nine bounce parameters before the
functions. The complete pool is 48 bytes: nine floats, four bytes of natural
double alignment, and the cooldown's conversion double. Its descriptor remains
last. The first recovery also claimed the following cell-margin float and
helper; the subsequent [boundary audit](dll_652_656_boundary.md) establishes
that both start WCPushBlock's independently aligned pool and code.

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
| GSAE01 | `803E6D20..803E6D50` |
| GSAE01_rev1 | `803E79B8..803E79E8` |
| GSAJ01 | `803E6E40..803E6E70` |
| GSAP01 | `803E8550..803E8580` |
| GSAP01_rev1 | `803E8718..803E8748` |

After the boundary correction, all five versions retain nine exact functions
and own 104 exact data bytes. Both `ninja all_source` and the native strict
retail checksum pass with the complete pool linked from source. Regional
projection and the combined retail operand audit are recorded in the boundary
report.

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

## Arwing tuning pool (2026-09-29)

Arwing's 38 named tuning floats follow its 164-byte anonymous literal pool.
Defining them immediately before `arwarwing_initAttachments`, their first
consumer in source order, reproduces that layout. Earlier functions have
already emitted all anonymous literals needed by the later functions. Address
reads preserve the named constants without folding them into duplicate literals.
Definitions at the head of the file put the named pool too early; definitions
after their consumers let those references select writable small data first.
Neither failed placement establishes a separate source file.

The constants retain external linkage to preserve definition order, but no
other TU consumes them. The old public `extern f32` declarations are removed.
No code boundary, descriptor position, compiler setting, or generated path
changes. The resulting `.sdata2` is 316 bytes; the following four zeros are
natural alignment before DLL 667.

| Version | Complete `.sdata2` range |
| --- | --- |
| GSAE01 | `803E6EC8..803E7004` |
| GSAE01_rev1 | `803E7B60..803E7C9C` |
| GSAJ01 | `803E6FE8..803E7124` |
| GSAP01 | `803E86F8..803E8834` |
| GSAP01_rev1 | `803E88C0..803E89FC` |

All five versions retain 54 exact functions and now own 852 exact data bytes.
Both `ninja all_source` and the native strict retail checksum pass. Regional
projection reproduces the complete unit's ranges. The former 156-byte gap
disappears, leaving 44 unscored data bytes per version: the asteroid pair and
alignment, render/graphics small BSS, and the existing 24-byte metadata report
limitations.
The independent retail operand audit confirms all 38 constants, their bytes,
and 62 paired r2-relative references in each secondary version.
