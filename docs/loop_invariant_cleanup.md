# Loop-invariant optimization cleanup (2026-09-27)

No production TU disables loop-invariant optimization. The final two users,
`dlls/engine/2/2.c` and `main/pi_dolphin.c`, now use the corresponding common
profiles without `noloopinvariants`; its three unused profiles are removed.
Compiler versions, TU boundaries and matching status are unchanged. Historical
reports and general compiler-probe options still name the flag.

`seqDoSubCmd0B` passed its object through pointer-to-integer casts to the two
broadcast message APIs. Every caller of these APIs supplies a pointer or null,
so their payload parameter is now `void*`. Conversion to the queue's existing
32-bit storage happens inside the API. The three other non-null callers pass
their object directly. The single-recipient message API retains its mixed
integer/pointer payload contract.

`defragMemory` retains the resource base as an integer address and casts each
derived table cursor at its use. The existing pointer-arithmetic step to the
biased base stays explicit: folding it into integer addition changes the
generated addressing sequence.

`mapUnload` keeps an integer address view for indexed resource slots alongside
the typed view used for merge buffers. Its slot offsets use `offsetof` instead
of pointer-to-integer conversions on invariant member addresses. The ROM-list
release now uses a pointer to the actual slot, removing the old biased integer
address and compensating negative displacement.

Validation:

- EN objdiff: all 70 engine-2 functions and all 57 `pi_dolphin` functions,
  plus their complete data sections, remain 100% exact.
- Complete object comparison covers those two TUs, `objlib`, and all seven
  broadcast callers in EN and all four secondary targets. Section contents,
  symbol offsets and relocations are unchanged; only anonymous literal-symbol
  numbering changes in some objects.
- Every input DOL was hash-verified. Both modified TUs also pass isolated
  source-substitution links in EN rev1 and JP. PAL and PAL rev1 stop in the
  all-retail baseline link with duplicate `lbl_803DCC20` / `lbl_803DCC98`
  definitions, before source substitution. Their object comparisons still
  pass; no regional matching-manifest entries are changed.
- `ninja all_source` and the strict EN retail checksum target pass.
- Formatting is a separate, object-preserving change.
