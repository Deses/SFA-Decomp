# Memory-card literal pool and unit boundary

The EN v1.1 `saveCardBuildComment` holdout is exact with the existing GC/1.3
profile. It needs compiler-generated string pooling, not a different compiler
or instruction-scheduling setting. The recovered card code and the adjacent
utility unit match all code and assigned data in all five retail versions.

## Evidence

The reconstructed `maketex.c` loaded `sMemoryCardFileNameString`, defined in
`intersect_memcard.c`, and added offsets as large as `0x138` to that 20-byte
array. Those offsets reached an explicit byte array in the next unit, crossing
three intervening switch tables. This reproduced linked addresses but concealed
the compiler's shared `.data` pool.

In EN v1.1, retail materializes the pool address in `r31` before calling
`getCurLanguage`. An ordinary local pointer produced either a later address
load or an extra copy through `r0`. Replacing the comment formats with string
literals makes MWCC insert the correct pool-base setup automatically. Combining
the card functions with the preceding card code then reproduces the complete
pool: the filename literal at offset zero, the three generated jump tables,
the asset paths, and the regional title/subtitle strings. The v1.1 comment
function's 432 bytes become exact without special flags or casts.

The boundary now falls before `arrayRemoveUnordered`, after
`cardSetStatusNoCard2`. This keeps the save callbacks, card operations, image
loader and comment builder together, and leaves the contiguous array, pair-table,
random and timer helpers in their own unit. Existing source paths are retained;
this does not establish original filenames.

Merging the utilities as well would lose a separate zero constant: their
16-byte `.sdata2` pool contains a float zero, alignment and the generated signed
integer-to-double bias. The card unit independently owns its existing 16-byte
pool. Both pools, the following ObjSeq pool, and their retail addresses remain
unchanged. The original split between the card callbacks instead cut through
the shared literal pool.

## Data ownership

`sMemoryCardFileName` now initializes from `"Star Fox Adventures"`. The image
loader uses the six literal asset paths, and the comment builder uses literal
format strings. The artificial `gMemoryCardBannerAssetNames` definition,
cross-array offset macros and obsolete external string declaration are removed.
The symbol configs identify the individual literals with local descriptive
names; these names do not introduce source definitions.

The compiler's final pool extent excludes two trailing alignment bytes in EN
v1.0 and JP and four in both PAL versions. EN v1.1 ends with the subtitle and
needs no correction. These zero bytes remain linker padding outside the unit.
The previous byte arrays had included them in source-data accounting. No retail
bytes are removed, and no padding arrays are introduced.

| Version | Card code | Card data | Utility code | Utility data |
| --- | ---: | ---: | ---: | ---: |
| EN v1.0 | 10,132 | 390 | 996 | 16 |
| EN v1.1 | 10,404 | 392 | 996 | 16 |
| JP | 10,132 | 390 | 996 | 16 |
| PAL v1.0 | 10,524 | 372 | 996 | 16 |
| PAL v1.1 | 10,524 | 372 | 996 | 16 |

All entries above are fully matching, including the generated jump-table
relocations. Every input DOL was checked against its configured hash. All five
`all_source` builds and native `--matching` checksum builds pass with the card
source linked. Compiler profiles, expected hashes and the following engine/2
unit are unchanged.

The regional projector was rerun after the EN split change. Its generic
projection cannot preserve every regional literal-pool endpoint here and also
revises unrelated, previously verified regional boundaries. Those proposed
split, symbol and fallback-mapping changes were not retained. The four regional
configs instead keep the directly audited ranges above; the regenerated EN
v1.1 manifest adds the now-exact utility unit. The other manifests are unchanged.

The earlier compiler-impossibility claims about `saveCardBuildComment` in
`version_parity_frontier.md` are superseded by this recovery. Those experiments
tested the manually reconstructed address expression, not the original pool
generation mechanism.
