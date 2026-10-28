// SPDX-License-Identifier: MIT
#include <Uefi.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include "menu_core.h"

static EFI_SYSTEM_TABLE *ST;
static EFI_BOOT_SERVICES *BS;
static EFI_FILE_PROTOCOL *Root, *Log;
static EFI_HII_DATABASE_PROTOCOL *DB;
static EFI_FORM_BROWSER2_PROTOCOL *Browser;
static EFI_GUID LoadedGuid=EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID FsGuid=EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID DbGuid=EFI_HII_DATABASE_PROTOCOL_GUID;
static EFI_GUID BrowserGuid=EFI_FORM_BROWSER2_PROTOCOL_GUID;
static EFI_GUID SetupGuid={0xec87d643,0xeba4,0x4bb5,{0xa1,0xe5,0x3f,0x3e,0x36,0xb2,0x0d,0xa9}};
static EFI_GUID NvLockGuid={0x504af431,0x3025,0x4d32,{0x9f,0xbf,0xe9,0xf3,0x18,0x55,0x55,0x1d}};
static EFI_HII_HANDLE Targets[3];
static UINTN Counts[3],RunNumber;
static const CHAR8 *Names[3]={"AMD CBS","AMD PBS","Setup Advanced"};
#define MAX_BYTES (32U*1024U*1024U)

/* Freestanding support: no CRT, no Windows imports. */
void *memcpy(void *d,const void *s,UINTN n);
void *memset(void *d,int c,UINTN n);
#pragma function(memcpy, memset)
void *memcpy(void *d,const void *s,UINTN n) {
 UINT8 *a=d; const UINT8 *b=s; UINTN i; for(i=0;i<n;i++) a[i]=b[i]; return d;
}
void *memset(void *d,int c,UINTN n) {
 volatile UINT8 *a=d; UINTN i; for(i=0;i<n;i++) a[i]=(UINT8)c; return d;
}
static void emit(const CHAR8 *s) {
 CHAR16 u[160]; UINTN n=0,k=0;
 while(s[n]) n++;
 if(Log) {
  UINTN written=n; EFI_STATUS status=Log->Write(Log,&written,(VOID*)s);
  if(EFI_ERROR(status) || written!=n) {Log->Close(Log);Log=NULL;ST->ConOut->OutputString(ST->ConOut,L"Log write failed. Photograph the screen.\r\n");}
  else {
   status=Log->Flush(Log);
   if(EFI_ERROR(status)){Log->Close(Log);Log=NULL;ST->ConOut->OutputString(ST->ConOut,L"Log flush failed. Photograph the screen.\r\n");}
  }
 }
 while(*s) {
  u[k++]=(CHAR16)(UINT8)*s++;
  if(k==159 || !*s) {u[k]=0; ST->ConOut->OutputString(ST->ConOut,u); k=0;}
 }
}
static void hex(UINT64 value) {
 CHAR8 s[19]; UINTN i; s[0]='0';s[1]='x';
 for(i=0;i<16;i++) s[2+i]="0123456789ABCDEF"[(value>>(60-4*i))&15];
 s[18]=0;emit(s);
}
static void status_line(const CHAR8 *label,EFI_STATUS status) {
 emit(label);emit(": ");hex(status);emit("\r\n");
}
static void file_name(CHAR16 *dest,const CHAR16 *prefix,UINTN number,const CHAR16 *suffix) {
 UINTN i=0,k=0;while(prefix[k])dest[i++]=prefix[k++];
 dest[i++]=(CHAR16)('0'+number/100);dest[i++]=(CHAR16)('0'+number/10%10);dest[i++]=(CHAR16)('0'+number%10);
 k=0;while(suffix[k])dest[i++]=suffix[k++];dest[i]=0;
}
static EFI_STATUS open_log(EFI_HANDLE image) {
 EFI_LOADED_IMAGE_PROTOCOL *loaded=NULL;EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs=NULL;
 EFI_STATUS s;UINTN i;CHAR16 name[40];EFI_FILE_PROTOCOL *check=NULL,*dataDir=NULL;
 s=BS->HandleProtocol(image,&LoadedGuid,(VOID**)&loaded);if(EFI_ERROR(s)||!loaded)return s;
 s=BS->HandleProtocol(loaded->DeviceHandle,&FsGuid,(VOID**)&fs);if(EFI_ERROR(s)||!fs)return s;
 s=fs->OpenVolume(fs,&Root);if(EFI_ERROR(s)||!Root)return s;
 s=Root->Open(Root,&dataDir,L"\\MEOW\\DATA",EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,EFI_FILE_DIRECTORY);
 if(EFI_ERROR(s)||!dataDir)return EFI_ERROR(s)?s:EFI_DEVICE_ERROR;
 Root->Close(Root);Root=dataDir;
 for(i=0;i<1000;i++) {
  file_name(name,L"MeowMenu",i,L".log");
  s=Root->Open(Root,&check,name,EFI_FILE_MODE_READ,0);
  if(!EFI_ERROR(s)) {check->Close(check);continue;}
  if(s!=EFI_NOT_FOUND)return s;
  s=Root->Open(Root,&Log,name,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,0);
  if(!EFI_ERROR(s)) {RunNumber=i;ST->ConOut->OutputString(ST->ConOut,L"Log file: \\MEOW\\DATA\\");ST->ConOut->OutputString(ST->ConOut,name);ST->ConOut->OutputString(ST->ConOut,L"\r\n");}
  return s;
 }
 return EFI_OUT_OF_RESOURCES;
}
static void snapshot(UINTN target,VOID *data,UINTN size) {
 CHAR16 name[40];EFI_FILE_PROTOCOL *f=NULL;EFI_STATUS s;UINTN written=size;
 const CHAR16 *prefix[3]={L"MeowCBS",L"MeowPBS",L"MeowSetup"};
 if(!Root)return;
 file_name(name,prefix[target],RunNumber,L".hii");
 /* Do not truncate or replace an existing snapshot. */
 s=Root->Open(Root,&f,name,EFI_FILE_MODE_READ,0);
 if(!EFI_ERROR(s)) {f->Close(f);status_line("Snapshot already exists",EFI_ALREADY_STARTED);return;}
 if(s!=EFI_NOT_FOUND){status_line("Snapshot lookup",s);return;}
 s=Root->Open(Root,&f,name,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,0);
 if(EFI_ERROR(s)){status_line("Snapshot open",s);return;}
 s=f->Write(f,&written,data);
 if(!EFI_ERROR(s))s=f->Flush(f);
 f->Close(f);
 if(!EFI_ERROR(s)&&written!=size)s=EFI_DEVICE_ERROR;
 status_line("HII snapshot write",s);
}
static EFI_STATUS export_list(EFI_HII_HANDLE handle,VOID **buffer,UINTN *size) {
 EFI_STATUS s;VOID *p=NULL;UINTN capacity=0;
 *buffer=NULL;*size=0;
 s=DB->ExportPackageLists(DB,handle,&capacity,NULL);
 if(s!=EFI_BUFFER_TOO_SMALL)return EFI_ERROR(s)?s:EFI_COMPROMISED_DATA;
 if(capacity<24 || capacity>MAX_BYTES)return EFI_BAD_BUFFER_SIZE;
 s=BS->AllocatePool(EfiBootServicesData,capacity,&p);if(EFI_ERROR(s))return s;
 *size=capacity;s=DB->ExportPackageLists(DB,handle,size,p);
 if(EFI_ERROR(s)||*size>capacity){BS->FreePool(p);return EFI_ERROR(s)?s:EFI_COMPROMISED_DATA;}
 *buffer=p;return EFI_SUCCESS;
}
static EFI_STATUS discover(void) {
 EFI_STATUS s;EFI_HII_HANDLE *handles=NULL;UINTN size=0,capacity,count,i,j,valid=0;
 s=DB->ListPackageLists(DB,EFI_HII_PACKAGE_FORMS,NULL,&size,NULL);
 if(s!=EFI_BUFFER_TOO_SMALL)return EFI_ERROR(s)?s:EFI_NOT_FOUND;
 if(size==0||size>MAX_BYTES||size%sizeof(EFI_HII_HANDLE))return EFI_BAD_BUFFER_SIZE;
 capacity=size;s=BS->AllocatePool(EfiBootServicesData,capacity,(VOID**)&handles);if(EFI_ERROR(s))return s;
 s=DB->ListPackageLists(DB,EFI_HII_PACKAGE_FORMS,NULL,&size,handles);
 if(EFI_ERROR(s)||size>capacity||size%sizeof(EFI_HII_HANDLE)){BS->FreePool(handles);return EFI_ERROR(s)?s:EFI_COMPROMISED_DATA;}
 count=size/sizeof(EFI_HII_HANDLE);emit("Forms package lists: ");hex(count);emit("\r\n");
 for(i=0;i<count;i++) {
  VOID *data=NULL;UINTN length=0;MENU_SCAN found;
  s=export_list(handles[i],&data,&length);
  if(EFI_ERROR(s)){status_line("HII export skipped",s);continue;}
  s=MenuScan(data,length,&found);
  if(EFI_ERROR(s)){status_line("Malformed package list skipped",s);BS->FreePool(data);continue;}
  valid++;
  for(j=0;j<3;j++)if(found.formsets[j]) {
   emit("Found ");emit(Names[j]);emit("; FormSets=");hex(found.formsets[j]);emit("; root forms=");hex(found.roots[j]);emit("\r\n");
   if(found.formsets[j]==1&&found.roots[j]==1&&Counts[j]==0)Targets[j]=handles[i];
   /* Any duplicate or missing root makes this target unavailable. */
   if(found.formsets[j]!=1||found.roots[j]!=1)Counts[j]+=2;else Counts[j]++;
   if(Counts[j]==1)snapshot(j,data,length);
  }
  BS->FreePool(data);
 }
 BS->FreePool(handles);emit("Valid package lists: ");hex(valid);emit("\r\n");
 return valid?EFI_SUCCESS:EFI_NOT_FOUND;
}
static EFI_STATUS read_key(EFI_INPUT_KEY *key) {
 EFI_STATUS s;UINTN index;
 for(;;) {
  s=ST->ConIn->ReadKeyStroke(ST->ConIn,key);if(s!=EFI_NOT_READY)return s;
  s=BS->WaitForEvent(1,&ST->ConIn->WaitForKey,&index);if(EFI_ERROR(s))return s;
 }
}
static BOOLEAN bytes_equal(const UINT8 *a,const UINT8 *b,UINTN n){UINTN i;for(i=0;i<n;i++)if(a[i]!=b[i])return FALSE;return TRUE;}
static EFI_STATUS forms_for_handle(EFI_HII_HANDLE handle,VOID **forms,UINTN *size){
 VOID *full=NULL,*copy=NULL;UINTN n=0,capacity;EFI_STATUS s;
 *forms=NULL;*size=0;s=export_list(handle,&full,&n);if(EFI_ERROR(s))return s;
 capacity=n;s=BS->AllocatePool(EfiBootServicesData,capacity,&copy);
 if(!EFI_ERROR(s))s=MenuCopyForms(full,n,copy,&capacity);
 BS->FreePool(full);
 if(EFI_ERROR(s)){if(copy)BS->FreePool(copy);return s;}
 *forms=copy;*size=capacity;return EFI_SUCCESS;
}
static EFI_STATUS verify_forms(EFI_HII_HANDLE handle,VOID *expected,UINTN size){
 VOID *actual=NULL;UINTN n=0;EFI_STATUS s=forms_for_handle(handle,&actual,&n);
 if(EFI_ERROR(s))return s;
 if(n!=size||!bytes_equal(expected,actual,size))s=EFI_COMPROMISED_DATA;
 BS->FreePool(actual);return s;
}
static EFI_STATUS save_data(const CHAR16 *prefix,const CHAR16 *suffix,VOID *data,UINTN size){
 CHAR16 name[64];EFI_FILE_PROTOCOL *file=NULL;EFI_STATUS s;UINTN n=size;
 file_name(name,prefix,RunNumber,suffix);
 s=Root->Open(Root,&file,name,EFI_FILE_MODE_READ,0);
 if(!EFI_ERROR(s)){file->Close(file);return EFI_ALREADY_STARTED;}
 if(s!=EFI_NOT_FOUND)return s;
 s=Root->Open(Root,&file,name,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,0);if(EFI_ERROR(s))return s;
 s=file->Write(file,&n,data);if(!EFI_ERROR(s)&&n!=size)s=EFI_DEVICE_ERROR;
 if(!EFI_ERROR(s))s=file->Flush(file);file->Close(file);return s;
}
static EFI_STATUS save_blob(const CHAR16 *prefix,VOID *data,UINTN size){return save_data(prefix,L".hii",data,size);}
static EFI_STATUS read_setup(UINT8 *data,UINT32 *attrs){
 UINTN n=255;EFI_STATUS s;
 if(!ST->RuntimeServices||!ST->RuntimeServices->GetVariable)return EFI_UNSUPPORTED;
 s=ST->RuntimeServices->GetVariable(L"Setup",&SetupGuid,attrs,&n,data);
 if(!EFI_ERROR(s)&&n!=255)s=EFI_BAD_BUFFER_SIZE;
 return s;
}
static void report_rd(const CHAR8 *label){UINT8 data[255];UINT32 attrs=0;EFI_STATUS s=read_setup(data,&attrs);status_line(label,s);if(!EFI_ERROR(s)){emit("Setup[0x8A] persistent value: ");hex(data[0x8a]);emit("; attributes: ");hex(attrs);emit("\r\n");}}
static EFI_STATUS set_native_lock(UINT8 level){
 UINT8 command[6]={'A','N','V','$',0,0};
 if(level>1)return EFI_INVALID_PARAMETER;
 command[4]=level;
 return ST->RuntimeServices->SetVariable(L"NvLockMailbox",&NvLockGuid,EFI_VARIABLE_BOOTSERVICE_ACCESS,sizeof(command),command);
}
static EFI_STATUS gated_rd_write(UINT8 *original,UINT8 *desired,UINT32 attrs){
 EFI_STATUS s,lockStatus;UINT8 current[255];UINT32 currentAttrs=0;
 /* This is a deliberate default-policy reset, NOT a claim to read the old lock. */
 if(original[0x55]!=1)return EFI_UNSUPPORTED;
 emit("Establishing native DXE default lock policy 1. Original private lock is not readable.\r\n");
 if(!Log)return EFI_DEVICE_ERROR;
 s=set_native_lock(1);status_line("Establish native lock 1",s);if(EFI_ERROR(s))goto relock;
 if(!Log){s=EFI_DEVICE_ERROR;goto relock;}
 /* From here on, every normal error path attempts re-locking, even if open fails. */
 s=set_native_lock(0);status_line("Open temporary native write window",s);
 if(!EFI_ERROR(s)){
  if(!Log)s=EFI_DEVICE_ERROR;
  else {
   s=read_setup(current,&currentAttrs);status_line("Write window preflight read",s);
   if(!EFI_ERROR(s)&&(currentAttrs!=attrs||!bytes_equal(current,original,sizeof(current))))s=EFI_COMPROMISED_DATA;
   if(!EFI_ERROR(s)&&!Log)s=EFI_DEVICE_ERROR;
   if(!EFI_ERROR(s)){s=ST->RuntimeServices->SetVariable(L"Setup",&SetupGuid,attrs,255,desired);status_line("Setup write inside native window",s);}
  }
 }
relock:
 lockStatus=set_native_lock(1);status_line("Re-establish native lock 1",lockStatus);
 if(EFI_ERROR(lockStatus)){emit("Native re-lock not confirmed. Restart before any further firmware operations.\r\n");return EFI_DEVICE_ERROR;}
 return s;
}
static EFI_STATUS persist_rd(BOOLEAN nativeWindow,UINT8 target){
 UINT8 original[255],desired[255],actual[255];UINT32 attrs=0,afterAttrs=0;UINTN matches=0,n=0;VOID *forms=NULL;EFI_STATUS s,readback;
 if(target>1)return EFI_INVALID_PARAMETER;
 if(!ST->RuntimeServices||!ST->RuntimeServices->SetVariable||!Log||Counts[2]!=1)return EFI_NOT_READY;
 s=forms_for_handle(Targets[2],&forms,&n);if(EFI_ERROR(s))return s;
 s=MenuPatchSetup(forms,n,&matches);BS->FreePool(forms);status_line("RD persistence platform identity",s);if(EFI_ERROR(s))return s;
 s=read_setup(original,&attrs);status_line("Read persistent Setup before write",s);if(EFI_ERROR(s))return s;
 emit("Current Setup[0x8A]: ");hex(original[0x8a]);emit("; attributes: ");hex(attrs);emit("\r\n");
 if(attrs!=3&&attrs!=7)return EFI_UNSUPPORTED;
 if(original[0x8a]>1)return EFI_UNSUPPORTED;
 if(original[0x8a]==target){emit(target?"RD MODE is already enabled. No variable or lock write performed.\r\n":"RD MODE is already disabled. No variable or lock write performed.\r\n");return EFI_SUCCESS;}
 s=save_data(target?L"MeowSetupBeforeRD":L"MeowSetupBeforeRestore",L".bin",original,sizeof(original));status_line("Save original Setup variable",s);if(EFI_ERROR(s)||!Log)return EFI_ERROR(s)?s:EFI_DEVICE_ERROR;
 memcpy(desired,original,sizeof(original));desired[0x8a]=target;
 emit("Writing only Setup[0x8A] to ");hex(target);emit(", preserving all other bytes and attributes.\r\n");
 if(!Log)return EFI_DEVICE_ERROR;
 s=ST->RuntimeServices->SetVariable(L"Setup",&SetupGuid,attrs,sizeof(desired),desired);
 status_line("Direct Setup SetVariable",s);
 if(s==EFI_SECURITY_VIOLATION&&nativeWindow){
  readback=read_setup(actual,&afterAttrs);status_line("Read after rejected direct write",readback);
  if(EFI_ERROR(readback))s=readback;
  else if(afterAttrs!=attrs||!bytes_equal(original,actual,sizeof(actual)))s=EFI_COMPROMISED_DATA;
  else if(!Log)s=EFI_DEVICE_ERROR;
  else s=gated_rd_write(original,desired,attrs);
 }
 readback=read_setup(actual,&afterAttrs);status_line("Persistent Setup readback",readback);
 if(!EFI_ERROR(readback)){
  emit("Readback Setup[0x8A]: ");hex(actual[0x8a]);emit("\r\n");
  if(!EFI_ERROR(s)&&(afterAttrs!=attrs||!bytes_equal(desired,actual,sizeof(actual))))s=EFI_COMPROMISED_DATA;
  if(EFI_ERROR(s)&&!bytes_equal(original,actual,sizeof(actual)))emit("Variable changed despite an error. Inspect backup and log before retrying.\r\n");
 }else if(!EFI_ERROR(s))s=readback;
 if(!EFI_ERROR(s))emit(target?"RD MODE enable verified in NVRAM. Restart and check ordinary F2 without USB boot.\r\n":"RD MODE restore verified in NVRAM. Only the hidden-menu switch was disabled.\r\n");
 else emit("RD MODE persistence NOT verified. No success claim.\r\n");
 return s;
}
static EFI_STATUS unlock_setup(void){
 VOID *original=NULL,*patched=NULL;UINTN n=0,matches=0;MENU_SCAN scan;
 EFI_STATUS s,restore,check;EFI_BROWSER_ACTION_REQUEST action=EFI_BROWSER_ACTION_REQUEST_NONE;
 BOOLEAN armed=FALSE;
 if(Counts[2]!=1||!Targets[2]||!DB->UpdatePackageList||!Log)return EFI_NOT_READY;
 s=forms_for_handle(Targets[2],&original,&n);if(EFI_ERROR(s))goto cleanup;
 s=MenuScan(original,n,&scan);if(EFI_ERROR(s)||scan.formsets[2]!=1||scan.roots[2]!=1){s=EFI_UNSUPPORTED;goto cleanup;}
 s=BS->AllocatePool(EfiBootServicesData,n,&patched);if(EFI_ERROR(s))goto cleanup;
 memcpy(patched,original,n);s=MenuPatchSetup(patched,n,&matches);
 emit("Setup RD MODE condition matches: ");hex(matches);emit("; required 28\r\n");
 status_line("Prepare Setup patch",s);if(EFI_ERROR(s))goto cleanup;
 s=save_blob(L"MeowOriginalSetup",original,n);status_line("Save original Forms backup",s);if(EFI_ERROR(s))goto cleanup;
 s=save_blob(L"MeowPatchedSetup",patched,n);status_line("Save patched Forms snapshot",s);if(EFI_ERROR(s))goto cleanup;
 s=verify_forms(Targets[2],original,n);status_line("Original Forms preflight",s);if(EFI_ERROR(s)||!Log){if(!EFI_ERROR(s))s=EFI_DEVICE_ERROR;goto cleanup;}
 emit("Applying temporary HII Forms update. No Setup variable write.\r\n");
 if(!Log){s=EFI_DEVICE_ERROR;goto cleanup;}
 /* UpdatePackageList is not transactional: arm restoration BEFORE calling it. */
 armed=TRUE;s=DB->UpdatePackageList(DB,Targets[2],patched);
 status_line("UpdatePackageList patched Forms",s);if(EFI_ERROR(s))goto cleanup;
 s=verify_forms(Targets[2],patched,n);status_line("Patched Forms readback",s);if(EFI_ERROR(s))goto cleanup;
 emit("SendForm begin: Setup Advanced with 28 RD MODE conditions bypassed\r\n");
 if(!Log){s=EFI_DEVICE_ERROR;goto cleanup;}
 s=Browser->SendForm(Browser,&Targets[2],1,&MenuGuids[2],MenuRoots[2],NULL,&action);
 status_line("Unlocked Setup SendForm returned",s);emit("Browser ActionRequest: ");hex(action);emit("\r\n");
 check=verify_forms(Targets[2],patched,n);status_line("Forms after browser session",check);
 report_rd("Read Setup after native browser");
cleanup:
 if(armed){
  restore=DB->UpdatePackageList(DB,Targets[2],original);status_line("Restore original Forms",restore);
  check=verify_forms(Targets[2],original,n);status_line("Restored Forms readback",check);
  if(EFI_ERROR(restore)||EFI_ERROR(check)){emit("Forms restoration could not be verified. Restart before further setup use.\r\n");s=EFI_DEVICE_ERROR;}
 }
 if(patched)BS->FreePool(patched);if(original)BS->FreePool(original);
 status_line("Temporary Setup unlock result",s);return s;
}
static EFI_STATUS menu(void) {
 EFI_STATUS s;EFI_INPUT_KEY key;EFI_BROWSER_ACTION_REQUEST action;UINTN target,i;
 for(;;) {
  emit("\r\nN16QF1 MEOW Menu 0.5\r\n");
  for(i=0;i<3;i++) {CHAR8 digit[2]={(CHAR8)('1'+i),0};emit(digit);emit("  ");emit(Names[i]);emit(Counts[i]==1&&Targets[i]?" [available]\r\n":" [unavailable or ambiguous]\r\n");}
  emit("4  Temporarily unlock Setup hidden menus\r\n5  Write RD MODE directly, no write-window change\r\n6  Enable RD MODE with controlled native write window\r\n7  Restore hidden-menu switch to disabled\r\nQ  Return to Shell\r\n");
  emit("Options 1 to 3 open original pages. Option 4 updates RAM Forms only.\r\n");
  emit("Saving inside the firmware browser may change BIOS settings.\r\n");
  emit("Option 5 writes one Setup setting and verifies the stored variable.\r\n");
  emit("Option 6 may briefly lower the native lock, then forces default lock 1.\r\n");
  emit("Option 7 restores only RD MODE, preserving all other current settings.\r\n");
  emit("Choose 1, 2, 3, 4, 5, 6, 7 or Q: ");s=read_key(&key);emit("\r\n");
  if(EFI_ERROR(s)){status_line("Keyboard",s);return s;}
  if(key.ScanCode==0x17||key.UnicodeChar=='q'||key.UnicodeChar=='Q')return EFI_SUCCESS;
  if(key.UnicodeChar=='4')return unlock_setup();
  if(key.UnicodeChar=='5')return persist_rd(FALSE,1);
  if(key.UnicodeChar=='6')return persist_rd(TRUE,1);
  if(key.UnicodeChar=='7')return persist_rd(TRUE,0);
  if(key.UnicodeChar<'1'||key.UnicodeChar>'3')continue;
  target=(UINTN)(key.UnicodeChar-'1');
  if(Counts[target]!=1||!Targets[target]){emit("Target not uniquely registered; no action taken.\r\n");continue;}
  /* Re-export immediately before use and verify the same target still exists. */
  {
   VOID *data=NULL;UINTN length=0;MENU_SCAN found;
   s=export_list(Targets[target],&data,&length);
   if(!EFI_ERROR(s)){s=MenuScan(data,length,&found);BS->FreePool(data);if(!EFI_ERROR(s)&&(found.formsets[target]!=1||found.roots[target]!=1))s=EFI_COMPROMISED_DATA;}
   if(EFI_ERROR(s)){status_line("Preflight refused",s);continue;}
  }
  action=EFI_BROWSER_ACTION_REQUEST_NONE;
  emit("SendForm begin: ");emit(Names[target]);emit(" root=");hex(MenuRoots[target]);emit("\r\n");
  s=Browser->SendForm(Browser,&Targets[target],1,&MenuGuids[target],MenuRoots[target],NULL,&action);
  status_line("SendForm returned",s);emit("Browser ActionRequest: ");hex(action);emit("\r\n");
  emit("No reset is performed by this application.\r\n");
 }
}
EFI_STATUS EFIAPI EfiMain(EFI_HANDLE image,EFI_SYSTEM_TABLE *system) {
 EFI_STATUS s;EFI_LOADED_IMAGE_PROTOCOL *loaded=NULL;UINT8 mode=MENU_MODE_MENU;
 ST=system;if(!ST||!ST->BootServices||!ST->ConOut||!ST->ConIn)return EFI_INVALID_PARAMETER;
 BS=ST->BootServices;
 s=BS->HandleProtocol(image,&LoadedGuid,(VOID**)&loaded);
 if(EFI_ERROR(s)||!loaded)return EFI_ERROR(s)?s:EFI_NOT_FOUND;
 s=MenuCommandMode(loaded->LoadOptions,loaded->LoadOptionsSize,&mode);
 if(EFI_ERROR(s)){ST->ConOut->OutputString(ST->ConOut,L"Invalid command line. Use --enable-rd, --disable-rd or --menu. No write performed.\r\n");return s;}
 s=open_log(image);
 if(EFI_ERROR(s)||!Log){status_line("Cannot create USB log; stopping",EFI_ERROR(s)?s:EFI_DEVICE_ERROR);if(Root)Root->Close(Root);return EFI_ERROR(s)?s:EFI_DEVICE_ERROR;}
 emit("MEOW Menu 0.5, x64 EFI application\r\n");
 emit("No SREP, no image patch, no NVRAM startup entry.\r\n");
 s=BS->LocateProtocol(&DbGuid,NULL,(VOID**)&DB);status_line("Locate HII Database",s);
 if(EFI_ERROR(s)||!DB){if(!EFI_ERROR(s))s=EFI_NOT_FOUND;goto done;}
 s=discover();status_line("HII discovery",s);if(EFI_ERROR(s))goto done;
 if(mode!=MENU_MODE_MENU){emit(mode==MENU_MODE_ENABLE?"One-click RD MODE enable.\r\n":"One-click restore of hidden-menu switch only.\r\n");report_rd("Read Setup at one-click start");s=persist_rd(TRUE,mode==MENU_MODE_ENABLE?1:0);goto done;}
 s=BS->LocateProtocol(&BrowserGuid,NULL,(VOID**)&Browser);status_line("Locate native FormBrowser2",s);
 if(EFI_ERROR(s)||!Browser||!Browser->SendForm){if(!EFI_ERROR(s))s=EFI_UNSUPPORTED;goto done;}
 report_rd("Read Setup at application start");
 s=menu();
done:
 status_line("Application exit",s);
 if(Log){Log->Flush(Log);Log->Close(Log);Log=NULL;}
 if(Root){Root->Close(Root);Root=NULL;}
 return s;
}
