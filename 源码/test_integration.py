# SPDX-License-Identifier: MIT
from pathlib import Path
import ctypes as C,json
base=Path(__file__).resolve().parent
lib=C.CDLL(str(base/'integration_test.dll'))
fn=lib.RunCase
fn.argtypes=[C.c_uint64,C.c_void_p,C.c_uint64,C.POINTER(C.c_uint64)]
fn.restype=C.c_uint64
cbs=(base/'CbsSetupDxeRPL.fixture.hii').read_bytes()
pbs=(base/'AmdPbsSetupDxe.fixture.hii').read_bytes()
setup=(base/'Setup.fixture.hii').read_bytes()
rows=[]
for case,label,data,expected_calls,error in [
 (0,'native CBS target selected',cbs,1,False),
 (1,'native browser missing',cbs,0,True),
 (2,'HII database missing',cbs,0,True),
 (3,'duplicate CBS handles refused',cbs,0,False),
 (4,'browser returns unsupported; application returns to menu',cbs,1,False),
 (5,'malformed package refused',b'bad package',0,True),
 (6,'USB log cannot be created; application stops',cbs,0,True),
 (7,'CBS not registered; selecting CBS refused',pbs,0,False),
]:
    b=C.create_string_buffer(data); results=(C.c_uint64*9)()
    status=fn(case,b,len(data),results)
    assert results[0]==expected_calls,(label,status,list(results))
    assert bool(status>>63)==error,(label,hex(status))
    rows.append({'case':label,'application_status':hex(status),'SendForm_calls':results[0],'mock_file_writes':results[1]})
for case,label,expected_calls,expected_updates,restored,error in [
 (10,'patch verified before browser, then original restored',1,2,True,False),
 (11,'update mutates database then returns error; original restored',0,2,True,True),
 (12,'readback mismatch; browser not called; original restored',0,2,True,True),
 (13,'restoration update fails; reported error',1,2,False,True),
 (14,'snapshot flush fails; no update performed',0,0,False,True),
 (15,'browser returns unsupported; original restored',1,2,True,True),
 (18,'UpdatePackageList missing; no update performed',0,0,False,True),
 (19,'restoration returns success but readback still patched; error',1,2,False,True),
 (20,'log flush fails after update; browser skipped and original restored',0,2,True,True),
]:
    b=C.create_string_buffer(setup);results=(C.c_uint64*9)();status=fn(case,b,len(setup),results)
    assert results[0]==expected_calls and results[2]==expected_updates,(label,hex(status),list(results))
    assert bool(results[3])==restored and bool(status>>63)==error,(label,hex(status),list(results))
    rows.append({'case':label,'application_status':hex(status),'SendForm_calls':results[0],'UpdatePackageList_calls':results[2],'restored_original_forms':bool(results[3])})
for case,label,setcalls,value,error in [
 (30,'RD direct write and all-byte readback succeed',1,1,False),
 (31,'RD direct write rejected; error not called success',1,0,True),
 (32,'SetVariable returns success but value unchanged; refused',1,0,True),
 (33,'other Setup byte differs after write; integrity error',1,1,True),
 (34,'authenticated attributes refused',0,0,True),
 (35,'unexpected Setup variable length refused',0,0,True),
 (36,'RD backup flush fails; no variable write',0,0,True),
 (37,'RD already enabled; no variable write',0,1,False),
 (38,'Setup variable missing; no variable write',0,0,True),
 (39,'post-write readback fails; not claimed successful',1,1,True),
 (40,'unexpected RD value refused',0,2,True),
]:
    b=C.create_string_buffer(setup);results=(C.c_uint64*9)();status=fn(case,b,len(setup),results)
    assert results[4]==setcalls and results[5]==value and bool(status>>63)==error,(label,hex(status),list(results))
    rows.append({'case':label,'application_status':hex(status),'SetVariable_calls':results[4],'stored_RD_value':results[5]})
for case,label,setcalls,value,mailcalls,lock,error in [
 (50,'native window writes RD and re-establishes lock 1',2,1,3,1,False),
 (51,'baseline lock command fails; no Setup retry; finally locks 1',1,0,2,1,True),
 (52,'open-window command mutates lock then fails; finally locks 1',1,0,3,1,True),
 (53,'Setup remains denied inside window; finally locks 1',2,0,3,1,True),
 (54,'final lock command fails; complete operation reported failed',2,1,3,0,True),
 (55,'log flush fails after opening; no Setup retry; finally locks 1',1,0,3,1,True),
 (56,'pre-write variable changed; no Setup retry; finally locks 1',1,0,3,1,True),
 (57,'direct write error other than security; no native window',1,0,0,1,True),
 (58,'DXE default policy condition mismatches; no native window',1,0,0,1,True),
 (59,'direct write succeeds; native window unnecessary',1,1,0,0,False),
 (60,'final variable read fails; still re-establishes lock 1',2,1,3,1,True),
 (61,'backup fails; no Setup or mailbox write',0,0,0,1,True),
 (62,'already enabled; no Setup or mailbox write',0,1,0,1,False),
]:
    b=C.create_string_buffer(setup);results=(C.c_uint64*9)();status=fn(case,b,len(setup),results)
    assert (results[4],results[5],results[6],results[7],bool(status>>63))==(setcalls,value,mailcalls,lock,error),(label,hex(status),list(results))
    rows.append({'case':label,'application_status':hex(status),'Setup_SetVariable_calls':results[4],'stored_RD_value':results[5],'mailbox_calls':results[6],'final_mock_lock':results[7]})
for case,label,writes,value,mail,error in [
 (70,'one-click already enabled; no prompt or variable or lock write',0,1,0,False),
 (71,'one-click enables RD through native window without prompt',2,1,3,False),
 (72,'invalid one-click arguments refused before any variable write',0,0,0,True),
]:
    b=C.create_string_buffer(setup);results=(C.c_uint64*9)();status=fn(case,b,len(setup),results)
    assert results[4]==writes and results[5]==value and results[6]==mail and results[8]==0 and bool(status>>63)==error,(label,hex(status),list(results))
    rows.append({'case':label,'status':hex(status),'Setup_SetVariable_calls':results[4],'mailbox_calls':results[6],'keyboard_reads':results[8]})
for case,label,setcalls,value,mail,lock,error in [
 (80,'restore through native window, only RD cleared and lock 1',2,0,3,1,False),
 (81,'already disabled restore is no-op',0,0,0,1,False),
 (82,'restore direct write succeeds without mailbox',1,0,0,0,False),
 (83,'restore backup flush fails; no write',0,1,0,1,True),
 (84,'restore window open partially changes lock then errors; finally 1',1,1,3,1,True),
 (85,'restore still denied inside window; finally 1',2,1,3,1,True),
 (86,'restore final readback fails; operation not claimed successful',2,0,3,1,True),
 (87,'restore re-lock fails; operation fails',2,0,3,0,True),
 (88,'restore unrelated byte changes detected',2,0,3,1,True),
 (89,'restore reports write success but value stays enabled; detected',1,1,0,0,True),
 (90,'restore rejects mismatched default-lock policy',1,1,0,1,True),
 (91,'restore rejects authenticated variable attributes',0,1,0,1,True),
 (92,'restore rejects unexpected variable length',0,1,0,1,True),
 (93,'repeat restore makes no variable or mailbox writes',0,0,0,1,False),
 (94,'restore rejects unexpected RD byte',0,2,0,1,True),
]:
    b=C.create_string_buffer(setup);results=(C.c_uint64*9)();status=fn(case,b,len(setup),results)
    assert (results[4],results[5],results[6],results[7],results[8],bool(status>>63))==(setcalls,value,mail,lock,0,error),(label,hex(status),list(results))
    rows.append({'case':label,'status':hex(status),'Setup_SetVariable_calls':results[4],'stored_RD_value':results[5],'mailbox_calls':results[6],'final_mock_lock':results[7],'keyboard_reads':results[8]})
(base/'integration-results.json').write_text(json.dumps({'host_protocol_simulation':rows,'limitations':['Mock Boot Services and FormBrowser2 only; not target firmware or VM execution.','No real firmware APIs or BIOS variables accessed.']},indent=2),encoding='utf-8')
print(json.dumps(rows,indent=2))
