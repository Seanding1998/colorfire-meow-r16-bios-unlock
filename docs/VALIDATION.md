# Validation record

## Hardware observations

The development machine is a COLORFIRE MEOW R16 Ultra / N16QF1 with AMI BIOS 1.09. The owner confirmed:

- Independent CBS and PBS pages can be opened through the installed native browser.
- Temporary Setup form changes expose RD-controlled menu entries, followed by successful Forms restoration and readback.
- The v0.3 persistent enable operation changes RD from 0 to 1 through the controlled native window, verifies the full variable and receives a successful final lock command result.
- Hidden settings remain visible after rebooting into ordinary BIOS.

This does not establish support for every model or later BIOS. The v0.5 disable direction and subsequent wrapper changes have not been separately exercised as a hardware write. No claim is made that all hidden BIOS parameters are safe, writable or persistent.

## Publicly reproducible tests

`源码/generate_test_fixtures.py` constructs synthetic HII structures using documented field encodings and target identifiers. It does not extract a BIOS or embed vendor firmware. These are parser test vectors, not packages for firmware installation or browser rendering.

The Windows host build tests the actual C parser and application against mocked Boot Services, Runtime Services, filesystem operations and FormBrowser2:

- 59 protocol scenarios cover direct writes, denied writes, controlled windows, restore-to-zero, no-op repetition, backup failures, partial-update errors, mismatched readback, log failures and failed re-locking.
- 23 CLI cases cover default menu, exact enable/disable modes, quoting, conflicting or unknown arguments and bounded UTF-16 input.
- Parser tests cover package bounds, duplicate targets, malformed class GUID counts, declared-length truncations and 12,000 deterministic random inputs.
- Patch tests require exactly 28 original conditions, exactly 56 changed bytes and no mutation on identity/count failure.

Local development also used real extracted and runtime HII data; those private files are deliberately not part of this repository or its public test prerequisites. Host simulation is not firmware execution and cannot certify a vendor callback implementation.
