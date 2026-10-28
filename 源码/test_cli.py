# SPDX-License-Identifier: MIT
from pathlib import Path
import ctypes as C,json
base=Path(__file__).resolve().parent
lib=C.CDLL(str(base/'menu_core_test.dll'))
fn=lib.MenuCommandMode;fn.argtypes=[C.c_void_p,C.c_uint32,C.POINTER(C.c_ubyte)];fn.restype=C.c_uint64
cases=[('',False),('--enable-rd',True),('--menu',False),('FS4:\\MEOW\\MeowMenu.efi --enable-rd',True),('"FS4:\\folder with spaces\\MeowMenu.EFI" --enable-rd',True),('MeowMenu.efi',False),('--enable-rd --menu',None),('--enable-rd --enable-rd',None),('--oops',None),('abc --enable-rd',None),('"bad.efi',None),('x'*260+'.efi --enable-rd',None)]
cases += [('--disable-rd',2),('FS4:\\MEOW\\MeowMenu.efi --disable-rd',2),('"FS4:\\folder with spaces\\MeowMenu.efi" --disable-rd',2),('--enable-rd --disable-rd',None),('--disable-rd --enable-rd',None),('--disable-rd --menu',None),('--disable-rd --disable-rd',None)]
for text,expected in cases:
    data=text.encode('utf-16-le')+b'\x00\x00';buf=C.create_string_buffer(data);mode=C.c_ubyte()
    status=fn(buf,len(data),C.byref(mode))
    assert bool(status>>63)==(expected is None),(text,hex(status))
    if expected is not None:assert mode.value==expected
for data in [b'x',b'--\x00x',b'x\x00']:
    buf=C.create_string_buffer(data);mode=C.c_ubyte();assert fn(buf,len(data),C.byref(mode))>>63
mode=C.c_ubyte(1);assert fn(None,0,C.byref(mode))==0 and mode.value==0
(base/'cli-results.json').write_text(json.dumps({'valid_and_invalid_argument_cases':len(cases)+4,'default_mode':'menu','enable_flag':'--enable-rd','restore_flag':'--disable-rd','invalid_arguments':'rejected without variable write'},indent=2),encoding='utf-8')
print('PASS CLI:',len(cases)+4,'argument/default/bounds cases')
