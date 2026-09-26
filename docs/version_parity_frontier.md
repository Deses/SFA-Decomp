# The version-parity frontier — seven units, one function each

`complete_code_percent` for the non-EN versions is gated by exactly seven shared units, and in every
one of them a **single function** is unmatched. Closing all seven closes the whole gap:
`tools/version_progress.py GSAP01` plus the per-unit rows give **+3.883 pts**, and the measured gap is
3.883 (92.7016 -> 96.5895). The same seven units gate `GSAE01_rev1` and `GSAP01_rev1`.

Read this before spending builds on any of them: each row's refutation list is measured, not guessed.

| unit | pts | function | shape | PAL GPR band |
| --- | --- | --- | --- | --- |
| `dlls/engine/0/0.c` | +2.620 | `pauseMenuDraw` | 1 web on the wrong home | 6 |
| `dlls/objects/653_WCLevelCont` | +0.295 | `wclevelcont_update` | `f0`/`f1` scratch pair | 2 |
| `main/gameloop.c` | +0.248 | `askProgressiveScanMode` | band rotation | 6 (EN 4) |
| `dlls/engine/2/maketex.c` | +0.236 | `loadMemCardImages` | 2 instructions too long | 1 |
| `dlls/engine/53/53.c` | +0.229 | `SaveSelectScreen_render` | band rotation | 9 |
| `dlls/objects/589_BossDrakor` | +0.222 | `bossdrakor_update` | band rotation | 7 |
| `dlls/objects/611_GM_MazeWell` | +0.034 | `GM_MazeWell_update` | `r28`/`r29` exchange | 4 |

Two arithmetic checks worth keeping, because they prove the "one function" claim rather than assuming
it: engine/0 is `75304 - 70652 = 4652 = 1163 x 4`, and engine/53 is `6640 - 5616 = 1024 = 256 x 4`.

## What landed

`pauseMenuDraw` went **19 -> 4** positional diffs on all three lagging versions by deleting two
reconstructed block temps. The token prompt arm measured each line into
`{ s32 tokenLineHeight = tokenBottom - tokenTop; tokenTextY = tokenLineHeight + tokenTextY; }`
before a separate `tokenTextY += K;`. Two of the four advances in the same arm already used the direct
`tokenTextY += (tokenBottom - tokenTop) + K;` form, so all four now use it. The temps had been
introduced to force the `add rX,r0,rX` operand order; the direct form produces that order by itself.
Their real cost was a **live-range split**: with the temps, PAL put the case 1 accumulator in `r26`
and then moved it to `r28` at the first advance, where retail (and EN) keep it in one register for the
whole arm. EN/JP stay byte-identical, and the source is a consistent single idiom.

This is lever 16 of `source_shape_levers.md` (two reconstructed locals where retail had one) firing on
a band the width screen calls hopeless. **A wide flat band does not mean the function is capped — it
means the defect is upstream of allocation.**

## Refutations, per unit

### `pauseMenuDraw` — the remaining 4 diffs

The whole 1163-instruction stream matches; one web is on the wrong home. PAL copies the three `int`
parameters to `r29`/`r27`/`r26` and `player` to `r28`; the `case 2:` `tokenTextY` lands on **paramC's**
register (`r26`) where retail reuses **paramA's** (`r29`). EN is a built-in control: it puts case 1 on
`player`'s register and case 2 on paramC's, and our source reproduces EN exactly.

Refuted (EN held at 0 diffs throughout unless noted):
- One function-scope `tokenTextY` shared by both arms — EN breaks at **every** one of the 20 positions
  (7 or 22 diffs). Retail genuinely has two separate block-scoped locals.
- Two distinctly-named siblings at switch scope, either order — flat at 4.
- Nested block with initializer, in either arm or both — flat at 4.
- `taskTextIds` moved to block scope in either/both arms, either declaration order — **4 -> 7/10**.
  The knob is live in the right region; it only pushes the wrong way.
- Reusing an existing function-scope local (`x`, `stringIndex`, `textY`, `textHeight`, `lineHeight`,
  `panelAlpha`) instead of a dedicated one — EN breaks identically within each group regardless of
  which name, i.e. it is the missing dedicated web that breaks EN, not the spelling.
- Typed recovery `statusTable->tokens[gPauseMenuTokenIndex].alt` for the raw
  `taskTextIds[gPauseMenuTokenIndex * 4]` — EN gains an instruction. `PauseMenuTokenEntry` really is
  8 bytes with an `alt` field, so the index arithmetic is right; the pointer local is load-bearing.
- Inlining the named constants `f32 zero = 0.0f` and `f32 backdropScale = 435.2f` — both EN-breaking,
  so both are load-bearing despite looking like reconstruction litter.
- PAL-only guard-group (`f32 lineSpan;`) position, all 18 destinations — flat at 4.
- Re-spelling the PAL-exclusive `lineSpan` block six ways — 4, or worse (5, 10).
- **Parameter reordering is not available here.** The stream already matches retail, so the three
  parameters' use sites are already correct; permuting them would change the stream, not just the
  homes.
- A 25-minute randomised **joint** search over the product of the declaration order (19 items, 1-3
  random moves per step, plateau walking) and the `lineSpan` block spelling, gating EN at 0 every
  step, logged nothing below 4. Single-knob flatness at width 6 is not an artifact of searching one
  knob at a time.
- **The PAL-only statement knob is live, and its reachable states were enumerated.** `case 4:` carries
  a PAL-exclusive `lineHeight = gGameTextFontMetrics[sLanguageNameTable[getCurLanguage()].fontId].lineHeight;`
  whose spelling EN never sees, so it steers PAL's band for free. Introducing a GPR temp there really
  does re-colour: a `fontId` temp gives **8**, a `language` temp **13**, reusing `textHeight` or `x`
  **8**, reusing `stringIndex` **23**, reusing `textY` **29**, a result-only temp **4**. Chaining 1, 2,
  3 or 4 temps all give **13** — it saturates immediately, exactly as the pressure-counter model says.
  Baseline 4 remains the minimum over every state reachable this way, and none of them is retail's
  colouring. Merging `lineSpan` into `timer` or `zero`, or declaring it unguarded, is inert at 4 —
  consistent with the GPR and FP counters being independent.

### `maketex` / `loadMemCardImages` — root-caused, 2 instructions

The unit has exactly one defect. Every other "differing function" in it is the 8-byte shift cascade
from this one being long: with it fixed the whole object lines up (`saveCardBuildComment` at 0xf58
onward shifts by exactly 8).

Retail keeps the name-table base in `r31` across the call and spends one instruction per use:

```
lis r3,HA(sMemoryCardFileNameString) ; addi r31,r3,LO ; bl saveCardBuildComment ; addi r3,r31,160
```

We rematerialize the address for the **first** use only (`lis; addi; addi r3,r3,160`), then use
`addi r3,r31,K` for the other five. A synthetic probe reproduces this exactly and isolates the
trigger: **a call between the constant-address assignment and its first use.** On EN the `sprintf`
calls in the SJIS arm use `names` before the DVD chain and anchor it; PAL's arm is a bare
`saveCardBuildComment();`, so nothing does.

- Spelling sweeps (`&names[K]`, a temp local, the global written out, `names` declared last, the
  one-element-array idiom `char* names[1]` that the surrounding code uses for scalars) — all still
  rematerialize. Writing the global out breaks **EN** by the same +2, which confirms the anchor
  reading.
- Twelve `-opt` profiles: only `nopropagation` (and `-opt off/level<=1`, `-O0/-O1`) suppress it, and
  every one of those wrecks EN (unit 3216+). The configured `-opt nopeephole,noschedule` is optimal
  for EN.
- All 13 GC compiler versions (1.0 through 2.7) behave identically — not a toolchain difference.
- **Assigning `names` after the call gets 247/247 with 3 diffs**, purely ordering: the `bl` then
  lands *before* the materialization where retail has it after. A redundant second assignment before
  the call is dead-code-eliminated and gives the same 3.

So the residual is one MWCC rematerialization heuristic, and the missing piece is a PAL-only use of
`names` before the call that costs no instruction. Nothing in retail's stream is such a use.

### `wclevelcont_update` — a genuine either/or between code and data

Four diffs, all in the PAL-exclusive message-timer clamp: retail loads the field into `f1` and the
zero into `f0` and stores `f0`; we swap the pair. The first compare in the same block already agrees
with retail, so only the second site inverts.

- Writing the clamp's zero as the plain literal `0.0f` makes the **code** byte-match
  (report `matched_code 8556/8556`) — but MWCC pools a second zero, `.sdata2` grows 0x38 -> 0x3c, and
  the section fails all-or-nothing (`matched_data 504/560`).
- Keeping `gWcLevelContZero` keeps the data exactly right: 0x38 bytes, and its **13 references match
  retail's 13 references to `lbl_803E85D8` one-for-one** — that atom is the unit's own `.sdata2`
  offset 0 (`splits.txt`: `.sdata2 start:0x803E85D8 end:0x803E8610`), so the symbol is the same object
  under a different name.
- Replacing the const with literals *everywhere* keeps `.sdata2` correct (MWCC merges the literals
  into one atom) but breaks 19 functions: the `base + (224.0f + px + gWcLevelContZero[0])` additions
  fold `+ 0.0f` away and lose an `fadds`. So the named atom is genuine and required.
- Declaration forms (`static`, `[1]`, scalar), eight no-op expression wrappers, a subtraction temp,
  reversed compare operands (MWCC canonicalizes — `zero > field` also gives 4), and twelve `-opt`
  profiles: all 4.

### The three rotations, and `GM_MazeWell`

`bossdrakor_update`'s 150 diffs are **one** exchange, not 150 defects: retail is
(`r31`=`obj` param, `r30`=first `obj->extra`, `r29`=second), ours is rotated by one. Applying lever
14's diagnostic (list every definition of each saved register in the target) shows the definition
counts match under the rotation — 1/2/6 against our 2/6/1 — so there is **no** missing local here;
it is a true rotation at band width 7. The named-param-copy lever moves EN by 65 diffs when the copy
is declared first and is inert when declared last. Coalescing-copy edits in the PAL-only
`curveStep`/`advanceStep` block are flat or size-breaking.

`askProgressiveScanMode` looks like a rotation of a 4-wide EN band that is 6 wide on PAL, but lever
14's diagnostic says it is **not** a relabeling: the per-register definition counts are
`[2,2,3,4,4,5]` in retail against `[1,2,2,4,5,6]` in ours, same total (20 definition points, same
stream), different grouping. Retail coalesces the 600-frame counter and `messageY` into one register;
we split them across two. That is a coalescing decision, so the lever is the local set, not the order
— and the PAL-only region is the only place we may change it. An 8-item declaration single-move sweep
found nothing below 25; the PAL-only `messageY`/`shadeReduction` declaration order and branch
assignment order give 31/35/37, so the knob is live and only pushes the wrong way so far. This is the
most promising of the six remaining walls, because the tell is structural rather than an allocator
residue.

`SaveSelectScreen_render` is band width **9** — two loop triples each rotated. In its first loop the
only named local among the three contested values is the PAL-exclusive `bulletY`; the other two are
strength-reduced walking pointers over `taskTexts` (stride 4) and `gSaveSelectInfoTextIds` (stride 1),
and the second loop's three values are *all* compiler temps with no named local behind them. Sweeping
`bulletY` through every function-scope position and every position inside the `OPEN_FILE` case block
where it is actually used (a more plausible scope) is flat at 20, or 23 at the last two case positions.
Dinosaur Planet's `dll_63_draw` — the ancestor — declares `y` before `i` around the same
`y = start; for (i...) { ... y += step; }` shape, which is what we already have.

`GM_MazeWell_update`'s `r28`/`r29` exchange is between `itemIndex` and a **strength-reduced walking
pointer that has no named local behind it**, so no ordering knob reaches it: the block-scoped
`found`/`itemIndex` swap and all 56 EN-safe single moves of the 8 function-scope declarations are flat
at 14.

## The v1.1 frontier is a DIFFERENT set — check it separately

`tools/version_progress.py GSAE01_rev1` does not list the same units as `GSAP01`. EN v1.1's gap is
**+4.393** over 6 real units, and two of them are not on PAL's list at all:

| unit | pts | blocker on EN v1.1 |
| --- | --- | --- |
| `dlls/engine/0/0.c` | +2.620 | `pauseMenuDraw`, same 4 diffs as PAL |
| `main/gametext.c` | +0.791 | `gameTextBuildSystemFontAtlas` — **v1.1 only**, complete on PAL and PAL v1.1 |
| `dlls/objects/653_WCLevelCont` | +0.295 | `wclevelcont_update`, same 4 diffs |
| `dlls/engine/2/maketex.c` | +0.236 | `loadMemCardImages`, plus `saveCardBuildComment` SIZE 108/109 on v1.1 only |
| `dlls/engine/53/53.c` | +0.229 | `SaveSelectScreen_render`, same |
| `dlls/objects/589_BossDrakor` | +0.222 | `bossdrakor_update`, same rotation |

PAL v1.0 has `main/gameloop.c` and `611_GM_MazeWell` instead, and its `gametext` is complete. So the
two lagging families need partly different work, and a fix verified only on PAL can leave v1.1 short.

**One of these was a real mis-guard, now fixed.** `bossdrakor_update`'s `" DRAKOR SPEED %f "` trace was
guarded to the European builds, but EN v1.1 emits it too: its retail `.data` carries the 18-byte string
right after the jump table, and the call sequence is
`lfs f1,0(r30); fmr f30,f1; lis/addi r3,<string>; crset 4*cr1+eq; bl logPrintf`, then the unscaled
`advanceStep` goes straight to `Obj_UpdateRomCurveFollowVelocityIndexed` — v1.1 has **no** 50Hz
rescaling, so it needs its own arm, not the PAL one. That took the unit's `.text` size from 0x1914 to
retail's 0x192c, its **data from 144/496 to 496/496**, and made `bossdrakor_init` match. The lesson is
general: **a size or data mismatch is a mis-guarded version feature and is findable; a same-length
register permutation is not.** Screen for the former before spending probes on the latter.

**Infrastructure note.** While chasing that, two of rev1's BossDrakor functions
(`bossdrakor_spawnAttackObjects`, `bossdrakor_handleActionEvent`) showed diffs that are **not** source
defects: the retail carve at `build/GSAE01_rev1/obj/.../BossDrakor.o` dates from 2026-09-09 while
`config/GSAE01_rev1/symbols.txt` was updated 2026-09-25, so six `bl`s to `s16toFloat` (present in that
config at 0x80080404, the exact branch target) were left unrelocated. Nothing in the ninja graph
produces those objects — they are inputs to `main.elf` — so staleness is invisible to the build. A scan
of all four versions' objects for the frontier units found this in **GSAE01_rev1's BossDrakor only**
(6 raw branches; `gametext` has 3 and `gameloop` 1 on every version, which is normal), so it is not a
systemic understatement. Worth re-carving that version's objects when convenient.

**`gameTextBuildSystemFontAtlas` (v1.1, +0.791)** is a textbook scratch-band signature: 306/306
instructions, everything identical through instruction 236, then a clean **rotation of `r3..r11` by
one** (`r3`→`r4`, … `r10`→`r11`, `r11`→`r3`) across the inlined `gameTextCopySystemFontTile` body.
CLAUDE.md calls that a per-TU flag signature, but no profile reaches it: eight `-opt` profiles and
seven `-inline` settings all leave EN at 0 and v1.1 at 36, except those that break EN too. Reshaping
the caller's tile-copy block is a live knob that moves **EN** (hoisting the texture base gives EN 29
diffs at 275/275 instructions), which only confirms the mechanism — the block is already right for the
four versions that match. Coalescing aliases in v1.1's own `switch (OSGetFontEncode())` arms are
completely inert, because MWCC folds them away without allocating anything.

**Measurement hazard found the hard way:** `gameTextSetLanguage` reads as 17 diffs on PAL from a
stale object and as `MATCH` once rebuilt, and the same unit is reported complete by `report.json`.
Always rebuild immediately before extracting, and prefer the ninja-built object for any unit listed in
that version's `matching_units.txt`.

## Axes that are exhausted across all seven, not just one

These were each run against every unit where they could apply, always with EN gated at 0 diffs. None
produced a match anywhere, so a new idea for this frontier should not start here.

- **TU `-opt` profiles.** Twelve profiles per unit. Every unit's configured profile is already the one
  that keeps EN byte-identical, and every alternative wrecks EN by thousands of diff words. Probed on
  engine/0 (`pauseMenuDraw` 4 under the configured profile, 4 or 200+ under all others), maketex,
  WCLevelCont. BossDrakor already carries `nocse,nopropagation`.
- **Compiler version.** All 13 GC builds (1.0, 1.1, 1.1p1, 1.2.5, 1.2.5n, 1.3, 1.3.2, 1.3.2r, 2.0,
  2.0p1, 2.5, 2.6, 2.7) produce byte-identical results for maketex. Not a toolchain axis.
- **Local type.** `int` / `u32` / `s32` / `long` / `short` / `u16` / `s16` / `u8` on the PAL-exclusive
  locals: `bulletY` in engine/53 is flat at 20 for all eight; `messageY` in gameloop is 25 for the
  32-bit types and 26 for the narrow ones. Allocator-visible in principle, inert here.
- **Coalescing copies as a pressure knob.** `b = a;` before the use emits no instruction (verified:
  the stream stays 242 instructions) and is therefore the one documented way to change the count of
  values the allocator sees without touching the stream. Added in gameloop's loop 1, loop 2, and both,
  at two declaration positions: **all flat at 25**. Consistent with the counter saturating, which also
  means the knob only exists in the decreasing direction, and nothing can be removed from these
  regions without changing the stream.
- **Inlined helper boundaries (lever 6).** A `static inline` helper that MWCC fully inlines keeps the
  instruction count exact and can move homes wholesale — extracting engine/0's `case 2:` body into one
  held `pauseMenuDraw` at 1141/1141 instructions while moving **127** registers. But the movement comes
  from the helper's *own locals* (its four measurement temps), not from the boundary: WCLevelCont's
  PAL-only block extracted into a helper with no new locals, in both a `state` and an `f32*` form, is
  **exactly inert at 4**. So the boundary is only a knob when it also changes the local set, and in all
  seven units EN matches without the helper, which means retail has no helper there to restore.
- **Splitting a web into two source locals.** gameloop's `messageY` has two disjoint webs (loop 1 in
  `r27`, matching; loop 2 in `r29` retail / `r28` ours), which looks exactly like two variables. Giving
  loop 2 its own function-scope local costs **one instruction** at every declaration position, so
  retail's is one variable whose web the allocator splits, and the split is not a source fossil.

## Reference mining — what the projects do and do not give

- `reference_projects/rena-tools/sfadebug` is a decomp of the SFA **PAL debug build** (from
  *Interactive Multi-Game Demo Disc - July 2002*, minimally optimized, so its stack slot order would
  reveal original local sets and declaration order directly — the one input this whole frontier is
  missing). **Its code bytes are not obtainable from this tree.** `orig/GSAP01-DEBUG/` holds only
  `.gitkeep`, and `default.dol.gzf` is a 43 MB Ghidra packed database, not a DOL: a Java-serialized
  header, then one zip entry `FOLDER_ITEM` at offset 52 which raw-inflates (`zlib`, `wbits=-15`) to
  171 MB of database buffers. Those buffers carry the symbol and source-file analysis — searching them
  finds `WClevcontrol`, `maketex.c`, `n_pausemenu` — but **no program bytes**: `blr` (`4e800020`),
  `mflr r0`, `mtlr r0` and `stwu r1,-x(r1)` each occur exactly **once** in 171 MB, where real GC code
  would have thousands. Do not spend time on this container again; getting the debug build's code
  needs the disc. `src/main/` has 11 hand-decompiled files and covers none of the frontier units.
- `sfadebug/notes/srcfiles.csv` is the useful artifact: 11,797 rows of
  `start,end,function,source-file` recovered from the debug build's `__FILE__` strings. It gives the
  **original filenames** — `WClevcontrol.c`, `main/maketex.c`, `n_pausemenu.c`, `frontend_control.c`,
  `savegame.c`, `picmenu.c` — and shows the original object-DLL callback naming: within
  `WClevcontrol.c` the functions are plain `initialise`, `release`, `init`, `update`, `hitDetect`,
  `render`, `free`, `getExtraSize`, `setScale`, not our `wclevelcont_*` prefixes. Useful for naming and
  TU-boundary work; it carries no local-variable information.
- `reference_projects/dinosaur-planet` carries the **real original DLL names** (`66_pausemenu`,
  `63_gameselect`, `18_objfsa`, ...) and per-DLL `syms.txt` with real global names, recovered from the
  prototype's symbol table. Local variable names there are the decomp authors' reconstructions, not
  DWARF, so it gives structure and declaration *order*, never a guaranteed local set.
  `66_pausemenu` is a far earlier ancestor of our pause menu (no tokens, no task text).
  `63_gameselect`'s `dll_63_draw` is the ancestor of `SaveSelectScreen_render` and declares `y` before
  `i` around the same `y = start; for (i...) { ... y += step; }` shape.

## See also

- `docs/source_shape_levers.md` — levers 9, 14 and 16 are the ones this frontier keeps invoking.
- `docs/band_width_worklist.md` — the width screen that correctly called six of these seven flat.
