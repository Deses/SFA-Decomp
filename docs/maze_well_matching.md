# Maze-well version parity

The complete slot-611 unit now matches all five retail versions with its
existing GC/1.3 compiler profile. PAL's remaining fourteen register differences
were a source-lifetime problem, not evidence for a reserved register.

The activation scan is a private inline helper. The outer update retains the
watched-bit scan and the sequence/input actions; the helper grants rewards and
queues the dialogue for the used item. Removing the redundant integer/pointer
conversions of the object preserves the four-register saved set. Both scans
then receive the retail registers in every version.

One further detail matters: the outer scan uses its walking `s16*` to test each
bit, then reads the selected value through the complete table view. The helper
also retains a complete table view. Collapsing those views into one pointer
leaves an extra `addi r0; mr r31,r0` at the table-address initialization.
Keeping both meaningful views lets the compiler common their address loads and
emit retail's direct `addi r31`. No new instructions, globals, compiler flags,
or regional source exceptions are needed.

## Packed data

`GmMazeWellQuestTables` replaces the flat 44-halfword definition. Its 88 bytes
contain the following arrays, with explicit two-byte gaps after the first two:

| Offset | Element type | Count | Use |
| --- | --- | ---: | --- |
| `0x00` | `s16` | 9 | Watched quest bits |
| `0x14` | `s16` | 9 | Reward bits |
| `0x28` | `s16` | 8 | Follow-up bits |
| `0x38` | `s32` | 8 | Dialogue IDs |

The canonical header asserts each offset and the total size. The legacy
`gGmMazeWellQuestBits` symbol retains its name, offset, and 88-byte extent;
the descriptor still follows it and ends the TU.

The loop still admits the unused ninth row. If activated, that row reads the
first dialogue's high halfword as its follow-up bit and the following
descriptor's first word as its dialogue. Explicit byte-derived pointers retain
those unchecked accesses without inventing ninth entries in either array.

## Validation

All six functions and all 148 assigned data bytes match in each version.
Code totals are 964 bytes for EN/EN v1.1/JP and 848 bytes for both PAL versions.
`ninja all_source` and the native `--matching` strict retail checksum target
pass for all five, including the rebuilt canonical-header consumer
`modelEngine.c`. The slot-611 generated-path audit also passes.

Both PAL matching manifests now include this source object. The only remaining
code holdout is `askProgressiveScanMode` in `main/gameloop.c`.
