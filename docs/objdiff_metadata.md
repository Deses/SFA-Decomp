# Comparing compiler-generated metadata

The optional `tools/objdiff_metadata.py` builder applies a local patch to upstream
objdiff v3.8.0, pinned at `784a16740a8a67b835b752ab867e76347d8a9691`. It fixes
comparison of anonymous constructor sections and CodeWarrior exception records.
It changes reporting only; it does not rewrite objects, alter matching status,
exclude metadata, or change the game's compiler and linker settings.

## Use

Install Git and a Rust toolchain satisfying upstream's `Cargo.toml`, then run:

```sh
python tools/objdiff_metadata.py --test
python tools/objdiff_metadata.py --print-path
```

Pass the printed binary path to the existing configure option, retaining any
other local tool paths:

```sh
python configure.py -v GSAE01 --matching --objdiff <printed-path>
ninja all_source
ninja
```

The builder checks out upstream beneath `build/tools/`, applies
`tools/patches/objdiff-metadata.patch`, verifies the exact revision and patch,
and builds with Cargo's locked dependency versions. The cache name includes the
revision and patch hash. A modified cached checkout causes an error; use another
`--build-dir` to preserve an experiment. No upstream or project commits are
created. Ordinary configure defaults remain unchanged; omit `--objdiff` to
return to the default tool, or pass your previous binary explicitly.

## What was wrong

`MSL_C/PPCEABI/bare/H/trigf.c` emits an anonymous four-byte `.ctors` section with
a relocation to its static initializer. The extracted reference has an ordinary
symbol covering those bytes. Upstream derives comparison extents from ordinary
symbols and consequently treats the anonymous source section as empty. The
patch compares its actual section bytes and relocations, while preserving
explicit symbol exclusions. It also prevents empty symbol sets from receiving
a vacuous 100% score and checks extra source relocations in the raw-byte fallback.

`musyx/runtime/sal_volume.c` emits exception metadata for three functions, of
which one survives the retail link. Its eight-byte `extab` record and twelve-byte
`extabindex` record therefore have different names and object offsets in the
source and extracted reference. The patch derives comparison identities from
the exception decoder's function-to-record relationships. Ambiguous shared
tables retain their original identities. The original bytes, lengths, symbols,
and relocations remain intact.

Exception-index relocations must also agree on symbol identity and addend.
Equal object-local offsets cannot establish the right function or table after
dead stripping. Regression tests specifically reject pointers to unrelated
symbols at the same offset, as well as wrong bytes, lengths, addends, missing
relocations, and extra relocations. Fixtures contain only synthetic PowerPC
functions and exception records, with assembly sources included for regeneration.

## Validation on staging

All 64 objdiff core tests passed, including 14 new metadata tests. The existing
exception snapshot changes only by adding six normalized comparison names.
Full reports from stock v3.8.0 and the previous local tool had identical unit
measures before the patch. With the patch, exactly two units gain data credit
in each version: `sal_volume` gains 20 bytes and `trigf` gains four. All code
measures, data totals, completion flags, and linked measures remain unchanged.

| Version | Matched data with the patch | Matched functions |
| --- | ---: | ---: |
| EN v1.0 | 1,209,820 / 1,209,820 | 9,498 / 9,498 |
| EN v1.1 | 1,211,086 / 1,211,086 | 9,501 / 9,501 |
| JP | 1,209,844 / 1,209,844 | 9,498 / 9,498 |
| PAL v1.0 | 1,213,902 / 1,213,902 | 9,505 / 9,506 |
| PAL rev1 | 1,214,026 / 1,214,026 | 9,505 / 9,506 |

Every version passed `ninja all_source` and its strict retail checksum target.
`tools/verify_source_link.py` first reproduced each hash-verified original from
retail objects, then reproduced it again with both metadata-owning source
objects substituted. All ten source-object hashes were unchanged. Thus these
24 bytes were already correct at final link; this fixes their comparison rather
than claiming new source recovery. PAL's `askProgressiveScanMode` remains
nonmatching and is not hidden by the tool change.
