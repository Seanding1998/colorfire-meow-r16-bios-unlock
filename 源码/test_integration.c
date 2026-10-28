// SPDX-License-Identifier: MIT
/* Host-only protocol simulation. Not linked into MeowMenu.efi. */
#include "meowmenu.c"
static EFI_SYSTEM_TABLE mockST;
static EFI_BOOT_SERVICES mockBS;
static EFI_SIMPLE_TEXT_INPUT_PROTOCOL mockIn;
static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL mockOut;
static EFI_LOADED_IMAGE_PROTOCOL mockLoaded;
static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL mockFS;
static EFI_FILE_PROTOCOL mockRoot,mockLog,mockSnapshot;
static EFI_HII_DATABASE_PROTOCOL mockDB;
static EFI_FORM_BROWSER2_PROTOCOL mockBrowser;
static EFI_RUNTIME_SERVICES mockRT;
static UINT8 storedSetup[255];
static UINTN variableWrites,variableReads,mailboxWrites,nativeLock;
static UINT8 pool[4*1024*1024];
static UINTN used,keyIndex,calls,writes,testCase;
static const UINT8 *fixture;
static UINTN fixtureSize;
static UINT8 currentForms[1024*1024];
static UINTN currentSize,updates;
static BOOLEAN equal_guid(const EFI_GUID *a,const EFI_GUID *b){UINTN i;for(i=0;i<16;i++)if(((const UINT8*)a)[i]!=((const UINT8*)b)[i])return FALSE;return TRUE;}
static EFI_STATUS EFIAPI output(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *t,CHAR16 *s){(VOID)t;(VOID)s;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI key(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *t,EFI_INPUT_KEY *k){(VOID)t;k->ScanCode=0;k->UnicodeChar=keyIndex++?'q':testCase>=50?'6':testCase>=30?'5':testCase>=10?'4':'1';return EFI_SUCCESS;}
static EFI_STATUS EFIAPI alloc(EFI_MEMORY_TYPE type,UINTN n,VOID **p){(VOID)type;n=(n+15)&~(UINTN)15;if(n>sizeof(pool)-used)return EFI_OUT_OF_RESOURCES;*p=pool+used;used+=n;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI release(VOID *p){(VOID)p;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI close_file(EFI_FILE_PROTOCOL *p){(VOID)p;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI flush_file(EFI_FILE_PROTOCOL *p){return ((testCase==14||testCase==36||testCase==61||testCase==83)&&p==&mockSnapshot)||(testCase==20&&updates==1&&p==&mockLog)||(testCase==55&&nativeLock==0&&p==&mockLog)?EFI_DEVICE_ERROR:EFI_SUCCESS;}
static EFI_STATUS EFIAPI write_file(EFI_FILE_PROTOCOL *p,UINTN *n,VOID *data){(VOID)p;(VOID)n;(VOID)data;writes++;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI open_file(EFI_FILE_PROTOCOL *p,EFI_FILE_PROTOCOL **f,CHAR16 *name,UINT64 mode,UINT64 attrs){(VOID)p;if(testCase==6)return EFI_ACCESS_DENIED;if(attrs&EFI_FILE_DIRECTORY){*f=&mockRoot;return EFI_SUCCESS;}if(name[0]=='\\')return EFI_INVALID_PARAMETER;if(mode==EFI_FILE_MODE_READ)return EFI_NOT_FOUND;*f=writes?&mockSnapshot:&mockLog;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI open_volume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *p,EFI_FILE_PROTOCOL **f){(VOID)p;*f=&mockRoot;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI handle(EFI_HANDLE h,EFI_GUID *g,VOID **p){(VOID)h;if(equal_guid(g,&LoadedGuid))*p=&mockLoaded;else if(equal_guid(g,&FsGuid))*p=&mockFS;else return EFI_NOT_FOUND;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI locate(EFI_GUID *g,VOID *registration,VOID **p){(VOID)registration;if(equal_guid(g,&DbGuid)){if(testCase==2)return EFI_NOT_FOUND;*p=&mockDB;}else if(equal_guid(g,&BrowserGuid)){if(testCase==1)return EFI_NOT_FOUND;*p=&mockBrowser;}else return EFI_NOT_FOUND;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI list_packages(CONST EFI_HII_DATABASE_PROTOCOL *t,UINT8 type,CONST EFI_GUID *g,UINTN *n,EFI_HII_HANDLE *h){UINTN need=testCase==3?2*sizeof(*h):sizeof(*h);(VOID)t;(VOID)g;if(type!=EFI_HII_PACKAGE_FORMS)return EFI_INVALID_PARAMETER;if(!h||*n<need){*n=need;return EFI_BUFFER_TOO_SMALL;}h[0]=(EFI_HII_HANDLE)1;if(testCase==3)h[1]=(EFI_HII_HANDLE)2;*n=need;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI export_packages(CONST EFI_HII_DATABASE_PROTOCOL *t,EFI_HII_HANDLE h,UINTN *n,EFI_HII_PACKAGE_LIST_HEADER *b){UINTN size=currentSize?currentSize:fixtureSize;const VOID *data=currentSize?currentForms:fixture;(VOID)t;(VOID)h;if(!b||*n<size){*n=size;return EFI_BUFFER_TOO_SMALL;}memcpy(b,data,size);*n=size;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI update_packages(CONST EFI_HII_DATABASE_PROTOCOL *t,EFI_HII_HANDLE h,CONST EFI_HII_PACKAGE_LIST_HEADER *b){UINTN size=b->PackageLength;(VOID)t;(VOID)h;updates++;if(testCase==13&&updates==2)return EFI_DEVICE_ERROR;if(!(testCase==19&&updates==2)){if(size>sizeof(currentForms))return EFI_OUT_OF_RESOURCES;memcpy(currentForms,b,size);currentSize=size;}if(testCase==12&&updates==1)currentForms[40]^=1;return testCase==11&&updates==1?EFI_DEVICE_ERROR:EFI_SUCCESS;}
static EFI_STATUS EFIAPI get_variable(CHAR16 *name,EFI_GUID *guid,UINT32 *attrs,UINTN *size,VOID *data){(VOID)name;variableReads++;if(!equal_guid(guid,&SetupGuid))return EFI_NOT_FOUND;if(testCase==38)return EFI_NOT_FOUND;if((testCase==39&&variableReads>=3)||((testCase==60||testCase==86)&&variableReads>=5))return EFI_DEVICE_ERROR;if(*size<255){*size=255;return EFI_BUFFER_TOO_SMALL;}memcpy(data,storedSetup,255);if(testCase==56&&variableReads==4)((UINT8*)data)[1]^=1;*size=testCase==35||testCase==92?254:255;*attrs=testCase==34||testCase==91?0x27:testCase==82?3:7;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI set_variable(CHAR16 *name,EFI_GUID *guid,UINT32 attrs,UINTN size,VOID *data){UINTN i;UINT8 *b=data;(VOID)name;
 if(equal_guid(guid,&NvLockGuid)){
  mailboxWrites++;if(size!=6||attrs!=2||b[0]!='A'||b[1]!='N'||b[2]!='V'||b[3]!='$'||b[5]!=0||b[4]>1)return EFI_INVALID_PARAMETER;
  if((testCase==51&&mailboxWrites==1)||((testCase==54||testCase==87)&&mailboxWrites==3))return EFI_DEVICE_ERROR;
  nativeLock=b[4];return (testCase==52||testCase==84)&&mailboxWrites==2?EFI_DEVICE_ERROR:EFI_SUCCESS;
 }
 variableWrites++;if(!equal_guid(guid,&SetupGuid)||size!=255||attrs!=(testCase==82?3U:7U))return EFI_INVALID_PARAMETER;for(i=0;i<255;i++)if(i!=0x8a&&b[i]!=storedSetup[i])return EFI_INVALID_PARAMETER;
 if(testCase==57)return EFI_DEVICE_ERROR;
 if(testCase==31||testCase==53||testCase==85||(testCase>=50&&nativeLock!=0))return EFI_SECURITY_VIOLATION;
 if(testCase!=32&&testCase!=89)memcpy(storedSetup,data,255);if(testCase==33||testCase==88)storedSetup[1]^=1;return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI send_form(CONST EFI_FORM_BROWSER2_PROTOCOL *t,EFI_HII_HANDLE *h,UINTN count,EFI_GUID *g,EFI_FORM_ID id,CONST EFI_SCREEN_DESCRIPTOR *screen,EFI_BROWSER_ACTION_REQUEST *action){UINTN target=testCase>=10?2:0;EFI_STATUS s;static UINT8 probe[1024*1024];UINTN matches=0;(VOID)t;(VOID)screen;if(!h||count!=1||!equal_guid(g,&MenuGuids[target])||id!=MenuRoots[target])return EFI_INVALID_PARAMETER;if(target==2){if(!currentSize)return EFI_NOT_READY;memcpy(probe,currentForms,currentSize);s=MenuPatchSetup(probe,currentSize,&matches);if(s!=EFI_UNSUPPORTED||matches!=0)return EFI_COMPROMISED_DATA;}calls++;*action=EFI_BROWSER_ACTION_REQUEST_NONE;return testCase==4||testCase==15?EFI_UNSUPPORTED:EFI_SUCCESS;}
__declspec(dllexport) EFI_STATUS RunCase(UINTN which,const UINT8 *data,UINTN size,UINTN *results){EFI_STATUS s;UINTN i;
 memset(&mockST,0,sizeof(mockST));memset(&mockBS,0,sizeof(mockBS));memset(&mockIn,0,sizeof(mockIn));memset(&mockOut,0,sizeof(mockOut));memset(&mockLoaded,0,sizeof(mockLoaded));memset(&mockFS,0,sizeof(mockFS));memset(&mockRoot,0,sizeof(mockRoot));memset(&mockLog,0,sizeof(mockLog));memset(&mockDB,0,sizeof(mockDB));memset(&mockBrowser,0,sizeof(mockBrowser));
 for(i=0;i<3;i++){Counts[i]=0;Targets[i]=NULL;}
 Root=NULL;Log=NULL;Browser=NULL;DB=NULL;used=keyIndex=calls=writes=currentSize=updates=variableWrites=variableReads=mailboxWrites=0;nativeLock=which==59||which==82||which==89?0:1;testCase=which;fixture=data;fixtureSize=size;
 for(i=0;i<255;i++)storedSetup[i]=(UINT8)(i^0x5a);storedSetup[0x8a]=which==37||which==62||which==70||which>=80?1:which==40?2:0;if(which>=50&&which!=58&&which!=90)storedSetup[0x55]=1;
 if(which==81||which==93)storedSetup[0x8a]=0;if(which==94)storedSetup[0x8a]=2;
 if(which==70||which==71){mockLoaded.LoadOptions=L"FS4:\\MEOW\\MeowMenu.efi --enable-rd";mockLoaded.LoadOptionsSize=sizeof(L"FS4:\\MEOW\\MeowMenu.efi --enable-rd");}
 if(which==72){mockLoaded.LoadOptions=L"--enable-rd --oops";mockLoaded.LoadOptionsSize=sizeof(L"--enable-rd --oops");}
 if(which>=80){mockLoaded.LoadOptions=L"FS4:\\MEOW\\MeowMenu.efi --disable-rd";mockLoaded.LoadOptionsSize=sizeof(L"FS4:\\MEOW\\MeowMenu.efi --disable-rd");}
 memset(&mockRT,0,sizeof(mockRT));mockRT.GetVariable=get_variable;mockRT.SetVariable=set_variable;if(which>=30)mockST.RuntimeServices=&mockRT;
 mockST.BootServices=&mockBS;mockST.ConIn=&mockIn;mockST.ConOut=&mockOut;mockOut.OutputString=output;mockIn.ReadKeyStroke=key;
 mockBS.HandleProtocol=handle;mockBS.LocateProtocol=locate;mockBS.AllocatePool=alloc;mockBS.FreePool=release;
 mockFS.OpenVolume=open_volume;mockRoot.Open=open_file;mockRoot.Close=close_file;
 mockLog.Write=write_file;mockLog.Flush=flush_file;mockLog.Close=close_file;mockSnapshot=mockLog;
 mockDB.ListPackageLists=list_packages;mockDB.ExportPackageLists=export_packages;mockDB.UpdatePackageList=testCase==18?NULL:update_packages;mockBrowser.SendForm=send_form;
 s=EfiMain((EFI_HANDLE)1,&mockST);results[0]=calls;results[1]=writes;results[2]=updates;results[3]=0;results[4]=variableWrites;results[5]=storedSetup[0x8a];results[6]=mailboxWrites;results[7]=nativeLock;results[8]=keyIndex;
 if(currentSize){static UINT8 check[1024*1024];UINTN n=sizeof(check);if(!EFI_ERROR(MenuCopyForms(data,size,check,&n))&&n==currentSize&&bytes_equal(check,currentForms,n))results[3]=1;}
 return s;
}
