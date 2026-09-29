# PAL text-data extraction and source-link verification

The PAL all-retail control link stalled in GC/1.3.2's optimization stage,
including with a 120-second timeout. Replacing just `main/gametext_data.o`
with its compiled source object made PAL v1.0 link in approximately three
seconds and reproduce the original DOL. This isolated an extraction problem
that the normal matching build concealed by already using that source object.

## Recover the string objects

Twenty entries in `sMapDirectoryNameTable` point to small string literals in
`gametext_data`'s `.sdata`. Both PAL symbol configs described these as zero-size
labels. DTK consequently emitted one 160-byte `pad_09_*_sdata` object spanning
the strings, with the labels embedded inside it. These are actual strings,
not padding. EN, EN rev1 and JP already describe their equivalents as objects.

The verified retail pointer table supplies each start address; the first NUL
supplies its size. All five versions have the same strings at the same table
indices. In both PAL releases they occupy eight-byte slots, starting at
`803DCA50` (v1.0) or `803DCC10` (rev1):

| Offset | String | Size including NUL |
| --- | --- | --- |
| `00` | Arwing | 7 |
| `08` | Boot | 5 |
| `10` | CRFort | 7 |
| `18` | DFPTop | 7 |
| `20` | Desert | 7 |
| `28` | LINKG | 6 |
| `30` | Link | 5 |
| `38` | LinkB | 6 |
| `40` | LinkC | 6 |
| `48` | LinkD | 6 |
| `50` | LinkE | 6 |
| `58` | LinkF | 6 |
| `60` | LinkH | 6 |
| `68` | LinkJ | 6 |
| `70` | MMPass | 7 |
| `78` | NWastes | 8 |
| `80` | Shop | 5 |
| `88` | SwapHol | 8 |
| `90` | Volcano | 8 |
| `98` | Warlock | 8 |

The records now use `type:object`, their actual sizes, `data:string`, and
`scope:local`. The source emits precisely these twenty local string objects;
their pointer-table references are in the same TU. Local linkage also resolves
PAL rev1's pre-existing duplicate names: string labels `lbl_803DCC20` and
`lbl_803DCC98` collided with unrelated source-named BSS globals in
`objprint_dolphin` and `pi_dolphin`. Those globals retain their names and linkage.
The bytes between each terminating NUL and the next string remain padding.

## Native verification

`tools/verify_source_link.py` now invokes the Windows linker directly and uses
`dtk.exe` on Windows. Other hosts retain the wibo invocation. Executable paths
are absolute, avoiding Windows subprocess failures on relative POSIX paths.
The all-retail baseline, exact DOL comparison, configured input hash check, and
30-second subprocess limits remain in place.

After configuring and building each version, run:

```sh
python tools/verify_source_link.py GSAP01 main/gametext_data.c musyx/runtime/sal_volume.c MSL_C/PPCEABI/bare/H/trigf.c
```

Repeat for `GSAE01`, `GSAE01_rev1`, `GSAJ01`, and `GSAP01_rev1`.
The selected MusyX and MSL units cover the remaining 24 uncredited metadata
bytes in the ordinary progress report: eight exception-table bytes, twelve
exception-index bytes, and one four-byte constructor entry. Exact source links
check their final retained bytes and relocations without adding manual compiler
metadata or changing the report denominator.

The PAL prompt remains a separate source mismatch. Substituting
`main/gameloop.c` is a negative control: the retail baseline must succeed and
the source link must fail the exact comparison. A successful normal matching
build still uses its retail object and does not establish a full PAL source link.

Validation after the correction:

- All five `all_source` builds and strict retail checksum targets pass.
- All five all-retail controls and three-unit source substitutions reproduce
  their verified originals byte-for-byte.
- All fifteen selected source objects retain their original SHA-256 hashes.
- Full regional regeneration preserves all forty corrected string records.
- Both PAL negative controls pass the retail baseline and reject the game-loop
  source substitution. Their function counts remain 9,505 / 9,506.
- `gametext_data` remains fully data-exact; the normal metadata scores are
  unchanged. No source, split, compiler setting, or expected checksum changes.
