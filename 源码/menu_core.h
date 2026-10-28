// SPDX-License-Identifier: MIT
#ifndef MEOW_MENU_CORE_H
#define MEOW_MENU_CORE_H
#include <Uefi.h>
typedef char MenuAssertUintn64[(sizeof(UINTN)==8)?1:-1];
typedef char MenuAssertChar16[(sizeof(CHAR16)==2)?1:-1];
typedef char MenuAssertGuid[(sizeof(EFI_GUID)==16)?1:-1];
typedef struct { UINTN formsets[3]; UINTN roots[3]; } MENU_SCAN;
extern EFI_GUID MenuGuids[3];
extern UINT16 MenuRoots[3];
/* Pure parser. Reads only within [buffer, buffer+size); never modifies input. */
EFI_STATUS MenuScan(const UINT8 *buffer, UINTN size, MENU_SCAN *result);
EFI_STATUS MenuCopyForms(const UINT8 *buffer, UINTN size, UINT8 *out, UINTN *outSize);
EFI_STATUS MenuPatchSetup(UINT8 *buffer, UINTN size, UINTN *matches);
#define MENU_MODE_MENU 0
#define MENU_MODE_ENABLE 1
#define MENU_MODE_DISABLE 2
EFI_STATUS MenuCommandMode(const CHAR16 *options, UINT32 bytes, UINT8 *mode);
#endif
