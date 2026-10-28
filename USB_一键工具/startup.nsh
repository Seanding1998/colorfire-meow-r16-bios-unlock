@echo -off
map -r
set -v meow_usb NONE
set -v meow_dups NO
if exist fs0:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs0
  else
    set -v meow_dups YES
  endif
endif
if exist fs1:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs1
  else
    set -v meow_dups YES
  endif
endif
if exist fs2:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs2
  else
    set -v meow_dups YES
  endif
endif
if exist fs3:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs3
  else
    set -v meow_dups YES
  endif
endif
if exist fs4:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs4
  else
    set -v meow_dups YES
  endif
endif
if exist fs5:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs5
  else
    set -v meow_dups YES
  endif
endif
if exist fs6:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs6
  else
    set -v meow_dups YES
  endif
endif
if exist fs7:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs7
  else
    set -v meow_dups YES
  endif
endif
if exist fs8:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs8
  else
    set -v meow_dups YES
  endif
endif
if exist fs9:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs9
  else
    set -v meow_dups YES
  endif
endif
if exist fs10:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs10
  else
    set -v meow_dups YES
  endif
endif
if exist fs11:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs11
  else
    set -v meow_dups YES
  endif
endif
if exist fs12:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs12
  else
    set -v meow_dups YES
  endif
endif
if exist fs13:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs13
  else
    set -v meow_dups YES
  endif
endif
if exist fs14:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs14
  else
    set -v meow_dups YES
  endif
endif
if exist fs15:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs15
  else
    set -v meow_dups YES
  endif
endif
if exist fs16:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs16
  else
    set -v meow_dups YES
  endif
endif
if exist fs17:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs17
  else
    set -v meow_dups YES
  endif
endif
if exist fs18:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs18
  else
    set -v meow_dups YES
  endif
endif
if exist fs19:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs19
  else
    set -v meow_dups YES
  endif
endif
if exist fs20:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs20
  else
    set -v meow_dups YES
  endif
endif
if exist fs21:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs21
  else
    set -v meow_dups YES
  endif
endif
if exist fs22:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs22
  else
    set -v meow_dups YES
  endif
endif
if exist fs23:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs23
  else
    set -v meow_dups YES
  endif
endif
if exist fs24:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs24
  else
    set -v meow_dups YES
  endif
endif
if exist fs25:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs25
  else
    set -v meow_dups YES
  endif
endif
if exist fs26:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs26
  else
    set -v meow_dups YES
  endif
endif
if exist fs27:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs27
  else
    set -v meow_dups YES
  endif
endif
if exist fs28:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs28
  else
    set -v meow_dups YES
  endif
endif
if exist fs29:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs29
  else
    set -v meow_dups YES
  endif
endif
if exist fs30:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs30
  else
    set -v meow_dups YES
  endif
endif
if exist fs31:\MEOW\USB.TAG then
  if %meow_usb% == NONE then
    set -v meow_usb fs31
  else
    set -v meow_dups YES
  endif
endif
if %meow_dups% == YES then
  echo ERROR: Multiple MEOW USB volumes found. No action performed.
  exit /b 1
endif
if %meow_usb% == NONE then
  echo ERROR: MEOW USB volume not found on fs0 through fs31.
  exit /b 1
endif
if not exist %meow_usb%:\MEOW\MeowMenu.efi then
  echo ERROR: MEOW program missing. No action performed.
  exit /b 1
endif
%meow_usb%:\MEOW\MeowMenu.efi --menu
set -v meow_status %lasterror%
echo EFI return status: %meow_status%
echo Logs and backups are in MEOW\DATA on the USB.
echo To restart, type reset after this script finishes.
exit /b %meow_status%
