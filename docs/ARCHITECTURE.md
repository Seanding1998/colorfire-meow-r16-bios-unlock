# Architecture and operational boundaries

## Native form targets

| FormSet | GUID | Root form |
|---|---|---|
| AMD CBS | `c03209ed-5f4b-4631-bd03-8ed627cd9daf` | `0x7000` |
| AMD PBS | `b863b959-0ec6-4033-99c1-8fd89f040222` | `0x000B` |
| Setup Advanced | `7b59104a-c00d-4158-87ff-f04d6396a915` | `0x2712` |

These identifiers describe the analyzed target firmware, not universal AMI layouts. The parser matches actual FormSet contents, not the outer PackageList GUID or an FFS file name. Missing or ambiguous targets are refused.

## RD MODE identity

`Setup`, GUID `ec87d643-eba4-4bb5-a1e5-3f3e36b20da9`, size 255. The analyzed question is `0xA6`, VarStore `0xF101`, offset `0x8A`, uint8 range 0–1. The application requires the matching HII definitions before attempting a persistent write.

Persistence copies the current entire variable, changes only the RD byte, retains the original attributes and verifies the entire result. Restore is the same operation with target zero; it does not replay a historical full-variable backup.

## Native write-window protocol

The analyzed firmware recognizes `NvLockMailbox`, GUID `504AF431-3025-4D32-9FBF-E9F31855551D`, with boot-services attributes and a six-byte `ANV$ + level + 0` command. This is a consumed SMM command, not a readable persistent mailbox variable.

The application only considers this mechanism after a rejected direct write, a second unchanged-data check and the expected `Setup[0x55]` policy condition. It establishes the analyzed DXE default level 1, temporarily uses level 0, performs the bounded RD write and attempts level 1 again on normal error paths. Final re-lock failure makes the whole operation fail and exits the application.

There is no readback interface for the private lock. This is an explicit default-policy reset, not restoration of an observed previous value. A firmware update can change the semantics while leaving some HII identifiers intact, so structural checks do not certify a new BIOS version.

## Temporary HII changes

Only the target Setup FormSet is patched, and the exact count must be 28. The original six-byte `EQ_ID_VAL` expression retains its opcode and size; only its constant changes from 0 to FFFF. No broad SuppressIf removal is performed.

`UpdatePackageList` replaces package types, so the application collects all Forms packages for the existing handle, excludes other package types and retains the handle. It arms restoration before the update because an error does not imply an unchanged database. Readback is checked before SendForm; restoration is attempted after browser return or error.

Firmware notification callbacks and browser-internal behavior remain outside the host simulator. Restoring Forms never claims to undo configuration changes saved by the user.
