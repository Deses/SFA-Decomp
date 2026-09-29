# PAL prompt matching

`askProgressiveScanMode` now matches both PAL revisions. All 34 functions and
all data in `gameloop.c` match in all five versions. The original investigation
below describes the previous 25-instruction register mismatch.

## Resolution

The whole TU uses its existing GC/1.3 compiler and optimization profile with
`-opt nolifetimes` added. This disables the earlier lifetime-splitting pass
identified below. It is one common profile for all five versions, with no
function pragmas or artificial TU splits. The other 33 functions remain exact.

The PAL prompt reuses two integer locals across phases: its initial frame
counter later holds the confirmation Y coordinate, and its retrace counter
later holds the displayed message ID. The confirmation shade has a separate
local. With lifetime splitting enabled, the reused values become fresh
temporaries and get different registers. Disabling splitting while retaining
one shade local also fails: it leaves the confirmation shade in r26 where
retail uses r0. The combination of source lifetimes and the TU profile is
necessary for this reconstruction.

PAL and EN/JP now have separate implementations within the same TU. Retail
establishes different behavior: PAL selects 50/60 Hz, positions text around
DVD errors, and scales the display copy; EN/JP select interlaced/progressive
scan and use the corresponding copy filter. The existing EN/JP implementation
is retained. The explicit byte conversion when PAL saves text alignment also
preserves its retail allocation. There is no change to the public text API.

This is a verified matching reconstruction, not proof of the original source
spelling or original build command. The flag alone does not match PAL, and a
source-only variable-renaming sweep cannot reproduce the required lifetimes.

Validation covers all five versions: `ninja all_source`, the strict DOL
checksum target, and independent links of all retail objects followed by a
link substituting `gameloop.c` from source. All ten independent links reproduce
their verified originals byte for byte. Formatting preserves all five raw
source-object hashes. Both PAL matching manifests now include `main/gameloop.c`.
After promotion, all five complete source builds also reproduce retail DOLs.
Reports made with the corrected metadata comparison from
[the objdiff tooling](objdiff_metadata.md) show 100% matched and completed code
and data: 9,498 functions in EN and JP, 9,501 in EN rev1, and 9,506 in each PAL
revision. No expected checksum or report denominator changed.

## Previous investigation

## Find the pass that creates a temporary

`tools/tricky_backend_trace.py --temporary-name` now works on native Windows
as well as macOS. First configure a PAL matching build, then run:

```sh
python tools/tricky_backend_trace.py --unit main/main/gameloop \
  --function askProgressiveScanMode --graph --temporary-name '@196' \
  --output build/pal_prompt_births
```

Names depend on the source revision. At source revision `b1b9531fb5`, `@196`
is virtual register 43, holding the confirmation phase's `messageY`. Its
factory caller returns to `0x45D2FF`, immediately after the call at `0x45D2FA`
to the object factory `0x4F4200`. The containing routine starts at `0x45D290`.
Its diagnostic strings include `Splitting range for variable: %d` and
`IROUseDef.c`; it creates a same-type object and rewrites selected definitions
and uses to that object. This establishes an earlier live-range split rather
than a register-coloring decision.

The first phase still uses the named `messageY` register 33. Likewise, a
scratch experiment sharing `i` between the retrace loop and displayed text ID,
and `counter` with confirmation Y, creates separate objects before the first
backend optimization snapshot. Both replacement objects originate at the same
factory call. Sharing C variable names does not preserve a shared allocation
node in these examples.

## A separate constraint in the wait loop

In the baseline graph, wait counter register 37 interferes only with physical
registers 0 and 3 through 12, and text ID register 38. Once the first phase has
enabled saved registers 28 through 31, retail's text ID assignment to 28 leaves
29 free. The observed allocator selects the lowest enabled free register,
so it cannot select retail's counter register 30 at that point. Correcting
the first phase's register rotation alone therefore does not fix the wait.

This constrains this graph and worklist, not all possible source forms. A
hypothetical union of same-color values can yield retail's allocation, but
the tested C rewrites do not produce that graph. It is not a matching result.

## Capture validation

The native Windows hook runs only in a private child compiler, verifies the
compiler hash and factory return instruction, and emulates the four-byte RET
without changing EAX or flags. Graph joins require object address, name and
type agreement; reused arena addresses do not establish identity. Missing
requested births fail the capture.

The baseline instrumented and ordinary objects have the same SHA-256:
`da560ca56fc59e822d67cb4653495f50ce8ce832c37604e28e23d10fd06b9c09`.
The factory hook does not alter compiler files, source, flags, or game objects.
