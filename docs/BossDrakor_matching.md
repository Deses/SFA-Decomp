# Boss Drakor version parity

`bossdrakor_update` and its complete unit match code and assigned data in all
five retail versions with the existing GC/1.3 compiler profile.

Two source changes close the remaining regional differences:

- Name the curve-walker pointer separately at each of the two `initCurve`
  calls. Keep the ordinary expression at the later curve-advance call.
- Preserve the raw speed for logging separately from the curve step in all
  versions that log it. PAL additionally scales the curve step for its
  non-EURGB60 mode; EN v1.1 retains the unscaled copy.

The pointer assignments change the compiler's interference graph without
adding instructions. A GC/1.3 trace of PAL v1.1 grows from 184 to 186 GPR
nodes. The two pointer values become distinct fixed-color aliases of `r3`.
The simplifier counts those aliases separately even though they have the same
physical color. At the second low-degree sweep, `obj` now has degree 29
instead of 27, so it survives the threshold of 29 until the next sweep.
It is consequently colored first: `obj=r31`, `state=r30`, `moveResult=r29`.
The resulting stream has zero retail differences, with no reserved register
or compiler setting change. EN and JP retain their different, exact allocation.

This supersedes the claims in `version_parity_frontier.md` that the register
rotation requires a reservation or that the graph is fixed by the emitted
instruction stream. Distinct source values can remain separate fixed-color
graph nodes while emitting the same instructions.

EN v1.1 also requires loading the raw speed into `f1` and copying it into
`f30` before the diagnostic call. Sharing the two-value PAL spelling recovers
that sequence and removes the separate v1.1 source arm.

## Validation

All 13 functions and every assigned data section are exact in each version:

| Version | Code bytes | Data bytes |
| --- | ---: | ---: |
| EN v1.0 | 6,380 | 472 |
| EN v1.1 | 6,444 | 496 |
| JP | 6,380 | 472 |
| PAL v1.0 | 6,480 | 504 |
| PAL v1.1 | 6,480 | 504 |

`ninja all_source` and the native `--matching` strict checksum target pass
for all five versions. Promoting the three previously incomplete manifests
therefore verifies the actual linked addresses, beyond normalized objdiff.
The generated DLL path audit also passes for slot 589.

EN v1.1 now has 100% matched and linked code. The remaining code holdouts
are PAL's `askProgressiveScanMode` and `GM_MazeWell_update`.

## Earlier EN helper recovery (2026-09-06)

The following records the earlier partial recovery; its remaining mismatches
were resolved by subsequent work and the version-parity fix above.

The September 6, 2026 follow-up starts at `8b8a2ec4ce`. EN GSAE01
BossDrakor improves from **99.91536% to 99.95298%** with the common game
compiler, GC/1.3. The unit remains `NonMatching`.

| Function | Bytes | Before | After | Remaining instruction differences |
| --- | ---: | ---: | ---: | --- |
| `bossdrakor_updateHeadTracking` | 524 | 99.42748% | 99.580154% | Eight words: `r0` and `r4` exchanged in the neck clamp |
| `bossdrakor_update` | 2192 | 99.89051% | 99.9635% | Three words: event counter uses `r25` instead of `r30` |

All other eleven functions remain byte-identical and exact. Every remaining
instruction difference is a register operand; the former `li` versus `mr`
zero-initialization mismatch in the joint loop is resolved. The number of
differing words alone therefore does not describe the objdiff improvement.

Three small `static inline` operations give the compiler the useful local
lifetimes: resetting the neck toward its rest angle, applying shake to the
five look-at joints, and initializing the air meter. The neck helper retains
a separate upper-bound result before the outer clamp merge. The shake helper
keeps its local index before the two short angles. Its float arguments read
the scale before the amount, preserving the retail load order.

Removing the TU's `-inline off` exception enables these helpers. The existing
`nopeephole,noschedule,nocse,nopropagation` options remain. Explicit inline
helpers emit no additional function bodies: `.text` is still 6,380 bytes
with the same thirteen functions and offsets. This is a source reconstruction
supported by code generation, not proof of the original helper names.

The complete assigned data is exact: `.data` 328 bytes, `.sdata` 24 bytes,
and `.sdata2` 120 bytes. Compared with the starting object, all section
relocations and all named symbol sections, offsets, sizes, and linkage are
unchanged. The generated slot path, descriptor position, TU boundaries, and
canonical header ownership are unchanged.

The neighboring Dinosaur Planet repository's `697_BossDrakor` is useful for
encounter context, but its state machine differs substantially; it does not
provide a direct implementation of these remaining EN functions.

Reproduce the per-function measurement with:

```sh
python3 configure.py --matching
ninja build/GSAE01/src/dlls/objects/589_BossDrakor/BossDrakor.o
python3 tools/unitfuzzy.py BossDrakor.c --all
```

Both `ninja all_source` and the strict matching `ninja` pass with 30-second
timeouts. Since BossDrakor remains `NonMatching`, the strict link still uses
its retail object. The separate formatting commit must preserve the source
object byte for byte; clang-format checks cover the TU and canonical header.
