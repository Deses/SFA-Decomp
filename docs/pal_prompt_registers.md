# PAL prompt register investigation

`askProgressiveScanMode` remains nonmatching on PAL. Its 242 instructions have
the retail opcode sequence, but 25 instructions use different registers. This
note records compiler observations, not recovered original source structure.

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
