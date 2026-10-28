# SPDX-License-Identifier: MIT
from pathlib import Path
import ctypes as C, struct, json, random, hashlib

base=Path(__file__).resolve().parent
class Scan(C.Structure):
    _fields_=[('formsets',C.c_uint64*3),('roots',C.c_uint64*3)]
lib=C.CDLL(str(base/'menu_core_test.dll'))
fn=lib.MenuScan
fn.argtypes=[C.c_void_p,C.c_uint64,C.POINTER(Scan)]
fn.restype=C.c_uint64
def scan(b):
    buf=C.create_string_buffer(b); result=Scan()
    status=fn(buf,len(b),C.byref(result))
    return status,list(result.formsets),list(result.roots)

names=('Setup','CbsSetupDxeRPL','AmdPbsSetupDxe')
fixtures={}
checks=[]
for name in names:
    fixture_path=base/'fixtures'/(name+'.fixture.hii')
    b=fixture_path.read_bytes()
    length=len(b)
    status,forms,roots=scan(b)
    idx={'CbsSetupDxeRPL':0,'AmdPbsSetupDxe':1,'Setup':2}[name]
    assert status==0 and forms[idx]==1 and roots[idx]==1,(name,hex(status),forms,roots)
    (base/(name+'.fixture.hii')).write_bytes(b)
    fixtures[name]=b
    checks.append({'fixture':name,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b),'formsets':forms,'roots':roots})

b=fixtures['CbsSetupDxeRPL']
assert scan(b'')[0]!=0
assert scan(b[:19])[0]!=0
# Prefix truncations with the declared list length updated must still be rejected.
for n in range(20,min(len(b),1000)):
    broken=bytearray(b[:n]);struct.pack_into('<I',broken,16,n)
    assert scan(bytes(broken))[0]!=0
# Walk to forms payload, test zero-length opcode and a malformed FormSet length.
p=20
while b[p+3]!=2:p+=struct.unpack_from('<I',b,p)[0]&0xffffff
q=p+4
broken=bytearray(b);broken[q+1]=0x80
assert scan(bytes(broken))[0]!=0
broken=bytearray(b);broken[q+22]=3
assert scan(bytes(broken))[0]!=0
# Duplicated full forms packages must report duplicates, allowing caller to refuse.
length=struct.unpack_from('<I',b,p)[0]&0xffffff
dup=bytearray(b[:p]+b[p:p+length]+b[p:]);struct.pack_into('<I',dup,16,len(dup))
status,forms,roots=scan(bytes(dup));assert status==0 and forms[0]==2 and roots[0]==2
# Changing only the PackageList GUID must not affect matching of the FormSet GUID.
changed=bytearray(b);changed[:16]=bytes(range(16))
assert scan(bytes(changed))==scan(b)
# Unknown FormSet GUID must not masquerade as CBS.
changed=bytearray(b);changed[q+2:q+18]=bytes(16)
assert scan(bytes(changed))[1][0]==0
# Deterministic malformed/random corpus exercises boundary checks of production C.
rng=random.Random(9850)
for i in range(12000):
    n=rng.randrange(0,2048);bad=bytearray(rng.randbytes(n))
    if n>=20:struct.pack_into('<I',bad,16,n)
    scan(bytes(bad))
report={'synthetic_fixtures':checks,'checks':['three generated synthetic HII lists accepted; GUID+root selected',str(max(0,min(len(fixtures['CbsSetupDxeRPL']),1000)-20))+' truncated-list cases rejected','zero-length opcode rejected','inconsistent FormSet class GUID count rejected','duplicate target counted twice','PackageList GUID not confused with FormSet GUID','unknown FormSet not selected','12000 deterministic malformed buffers completed'], 'limitations':['host C parser tests only, not firmware execution','no BIOS settings or NVRAM were accessed','AMI SendForm must be tested on target hardware']}
(base/'test-results.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
