# DLL 652 / 656 boundary and constant ownership

The stored-cell distance test belongs at the start of WCPushBlock (656),
following WCBouncyCra (652). Its only source caller is `wcpushblock_update`.
Both numbered slots and generated source paths remain unchanged.

The retail constant pools independently establish the boundary. Bouncy-crate
code consumes nine floats, followed by natural alignment and its conversion
double: 48 bytes at EN `803E6D20..803E6D50`. The cell test's 56-unit margin
starts the next eight-byte-aligned pool at `803E6D50`. Seventeen push-block
parameters, a conversion double, and the final `5.0f` literal complete that
84-byte pool at `803E6DA4`. The previous split put the cell test in 652,
claimed its margin there, and left the push-block parameters unowned.

Moving the helper restores both natural pool starts without padding or
compiler changes. The helper uses the existing `WCLevelContInterface`;
the duplicate partial interface and cross-unit helper declaration are removed.
Named constants are defined in retail order and read through their addresses
to avoid additional anonymous literal copies.

The push-block descriptor retains its proven early position. Its four tile
tables complete the 84-byte `.data` section at EN `8032B004`; the following
four zeros are natural linker alignment before WCLevelCont (653).

| Version | Text boundary | Constant-pool boundary |
| --- | --- | --- |
| GSAE01 | `802242A8` | `803E6D50` |
| GSAE01_rev1 | `802248F8` | `803E79E8` |
| GSAJ01 | `80224398` | `803E6E70` |
| GSAP01 | `80224A08` | `803E8580` |
| GSAP01_rev1 | `80224B40` | `803E8748` |

All five versions preserve all 19 exact functions and all data in both units:
652 has nine functions and 104 data bytes; 656 has ten functions and 168 data
bytes. WCLevelCont also remains exact. `ninja all_source` and the native
strict retail checksum pass. This removes 68 unscored data bytes per version,
leaving 200. Regional projection reproduces both full split blocks in all
four secondary versions. The retail operand audit confirms all 27 named
constants, their initialized bytes, and 47 paired r2-relative references per
secondary version. The generated-path audit passes for both slots.
