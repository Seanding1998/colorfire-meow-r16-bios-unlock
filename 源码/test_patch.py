# SPDX-License-Identifier: MIT
from pathlib import Path
import ctypes as C,json,struct
base=Path(__file__).resolve().parent
lib=C.CDLL(str(base/'menu_core_test.dll'))
patch=lib.MenuPatchSetup;patch.argtypes=[C.c_void_p,C.c_uint64,C.POINTER(C.c_uint64)];patch.restype=C.c_uint64
copy=lib.MenuCopyForms;copy.argtypes=[C.c_void_p,C.c_uint64,C.c_void_p,C.POINTER(C.c_uint64)];copy.restype=C.c_uint64
original=(base/'Setup.fixture.hii').read_bytes()
def run(b):
    buf=C.create_string_buffer(b);matches=C.c_uint64();s=patch(buf,len(b),C.byref(matches));return s,matches.value,buf.raw[:len(b)]
s,n,patched=run(original)
assert s==0 and n==28
diff=[i for i,(a,b) in enumerate(zip(original,patched)) if a!=b]
assert len(diff)==56 and all(original[i]==0 and patched[i]==255 for i in diff)
def refuse(b,expected=None):
    s,n,out=run(b);assert s>>63 and out==b
    if expected is not None:assert n==expected
refuse(patched,0)
broken=bytearray(original);broken[diff[0]]=255;broken[diff[1]]=255;refuse(bytes(broken),27)
var=original.index(bytes.fromhex('43D687ECA4EBB54BA1E53F3E36B20DA9'))
broken=bytearray(original);broken[var]^=1;refuse(bytes(broken))
rd=original.index(bytes.fromhex('0591A603A703A60001F18A001010000100'))
broken=bytearray(original);broken[rd+10]^=1;refuse(bytes(broken))
for name in ['CbsSetupDxeRPL','AmdPbsSetupDxe']:refuse((base/(name+'.fixture.hii')).read_bytes())
p=20
while original[p+3]!=2:p+=struct.unpack_from('<I',original,p)[0]&0xffffff
plen=struct.unpack_from('<I',original,p)[0]&0xffffff
insert=p+plen-2
extra=bytes.fromhex('0A821206A60000002902')
broken=bytearray(original[:insert]+extra+original[insert:]);struct.pack_into('<I',broken,16,len(broken));struct.pack_into('<I',broken,p,(2<<24)|(plen+len(extra)))
refuse(bytes(broken),29)
buf=C.create_string_buffer(original);size=C.c_uint64(0)
assert copy(buf,len(original),None,C.byref(size))>>63
output=C.create_string_buffer(size.value);assert copy(buf,len(original),output,C.byref(size))==0
forms=output.raw[:size.value];pos=20;types=[]
while pos<len(forms):
    h=struct.unpack_from('<I',forms,pos)[0];types.append(h>>24);pos+=h&0xffffff
assert types==[2,223] and forms[:16]==original[:16]
s,n,newforms=run(forms);assert s==0 and n==28
report={'checks':['exactly 28 conditions patched; exactly 56 bytes 00 to FF; all other bytes preserved','already patched, mixed 27 and extra 29 matches refused with no mutation','wrong VarStore GUID and RD MODE offset refused','CBS and PBS lists refused','Forms-only list preserves PackageList GUID and excludes Strings','Forms-only patch accepted'],'fixture_size':len(original),'forms_only_size':len(forms),'hardware_tested':False}
(base/'patch-results.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
