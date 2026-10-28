// SPDX-License-Identifier: MIT
#include "menu_core.h"
EFI_GUID MenuGuids[3] = {
 {0xc03209ed,0x5f4b,0x4631,{0xbd,0x03,0x8e,0xd6,0x27,0xcd,0x9d,0xaf}},
 {0xb863b959,0x0ec6,0x4033,{0x99,0xc1,0x8f,0xd8,0x9f,0x04,0x02,0x22}},
 {0x7b59104a,0xc00d,0x4158,{0x87,0xff,0xf0,0x4d,0x63,0x96,0xa9,0x15}}
};
UINT16 MenuRoots[3] = {0x7000,0x000b,0x2712};
static UINT32 rd32(const UINT8 *p) {
 return (UINT32)p[0] | ((UINT32)p[1]<<8) | ((UINT32)p[2]<<16) | ((UINT32)p[3]<<24);
}
static BOOLEAN same(const UINT8 *a,const UINT8 *b,UINTN n) {
 UINTN i; for(i=0;i<n;i++) if(a[i]!=b[i]) return FALSE; return TRUE;
}
EFI_STATUS MenuScan(const UINT8 *b,UINTN n,MENU_SCAN *r) {
 UINTN p=20,end,i; BOOLEAN finished=FALSE;
 if(!r) return EFI_INVALID_PARAMETER;
 for(i=0;i<3;i++) {r->formsets[i]=0; r->roots[i]=0;}
 if(!b || n<24 || rd32(b+16)!=n) return EFI_COMPROMISED_DATA;
 while(p<n) {
  UINT32 h; UINTN length; UINT8 type;
  if(n-p<4) return EFI_COMPROMISED_DATA;
  h=rd32(b+p); length=h&0xffffff; type=(UINT8)(h>>24);
  if(length<4 || length>n-p) return EFI_COMPROMISED_DATA;
  end=p+length;
  if(type==0xdf) { if(length!=4 || end!=n) return EFI_COMPROMISED_DATA; finished=TRUE; break; }
  if(type==2) {
   UINTN q=p+4,depth=0,fsDepth=0; INTN target=-1;
   while(q<end) {
    UINT8 op,scope; UINTN len;
    if(end-q<2) return EFI_COMPROMISED_DATA;
    op=b[q]; scope=b[q+1]>>7; len=b[q+1]&127;
    if(len<2 || len>end-q) return EFI_COMPROMISED_DATA;
    if(op==0x0e) {
     if(len<23 || !scope || fsDepth) return EFI_COMPROMISED_DATA;
     if((b[q+22]&0xfc) || len!=23+16*(b[q+22]&3)) return EFI_COMPROMISED_DATA;
     target=-1;
     for(i=0;i<3;i++) if(same(b+q+2,(const UINT8*)&MenuGuids[i],16)) {target=(INTN)i; r->formsets[i]++;}
     fsDepth=depth+1;
    }
    if(op==0x01) {
     UINT16 form;
     if(len<6 || !fsDepth || !scope) return EFI_COMPROMISED_DATA;
     form=(UINT16)(b[q+2]|((UINT16)b[q+3]<<8));
     if(target>=0 && form==MenuRoots[target]) r->roots[target]++;
    }
    if(op==0x29) {
     if(scope || len!=2 || depth==0) return EFI_COMPROMISED_DATA;
     if(depth==fsDepth) {fsDepth=0;target=-1;}
     depth--;
    } else if(scope) {
     if(depth>=4096) return EFI_COMPROMISED_DATA;
     depth++;
    }
    q+=len;
   }
   if(depth || fsDepth) return EFI_COMPROMISED_DATA;
  }
  p=end;
 }
 return finished ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
}
static void wr32(UINT8 *p,UINT32 v){p[0]=(UINT8)v;p[1]=(UINT8)(v>>8);p[2]=(UINT8)(v>>16);p[3]=(UINT8)(v>>24);}
EFI_STATUS MenuCopyForms(const UINT8 *b,UINTN n,UINT8 *out,UINTN *outSize){
 MENU_SCAN scan;EFI_STATUS s;UINTN p,total=24,k=20,i;
 if(!outSize)return EFI_INVALID_PARAMETER;
 s=MenuScan(b,n,&scan);if(EFI_ERROR(s))return s;
 for(p=20;p<n;){UINT32 h=rd32(b+p);UINTN len=h&0xffffff;if((h>>24)==2)total+=len;p+=len;}
 if(!out||*outSize<total){*outSize=total;return EFI_BUFFER_TOO_SMALL;}
 for(i=0;i<20;i++)out[i]=b[i];
 for(p=20;p<n;){UINT32 h=rd32(b+p);UINTN len=h&0xffffff;if((h>>24)==2){for(i=0;i<len;i++)out[k+i]=b[p+i];k+=len;}p+=len;}
 wr32(out+k,0xdf000004);wr32(out+16,(UINT32)total);*outSize=total;return EFI_SUCCESS;
}
EFI_STATUS MenuPatchSetup(UINT8 *b,UINTN n,UINTN *matches){
 MENU_SCAN scan;EFI_STATUS s;UINTN p,offsets[28],count=0,vars=0,rd=0,i;
 static const UINT8 setupVarGuid[16]={0x43,0xd6,0x87,0xec,0xa4,0xeb,0xb5,0x4b,0xa1,0xe5,0x3f,0x3e,0x36,0xb2,0x0d,0xa9};
 static const UINT8 name[6]={'S','e','t','u','p',0};
 if(!matches)return EFI_INVALID_PARAMETER;*matches=0;
 s=MenuScan(b,n,&scan);if(EFI_ERROR(s))return s;
 if(scan.formsets[2]!=1||scan.roots[2]!=1)return EFI_UNSUPPORTED;
 for(p=20;p<n;){
  UINT32 h=rd32(b+p);UINTN plen=h&0xffffff;
  if((h>>24)==2){
   UINTN q=p+4,end=p+plen,depth=0,fsDepth=0,prev=q;BOOLEAN target=FALSE;
   while(q<end){
    UINT8 op=b[q],scope=b[q+1]>>7;UINTN len=b[q+1]&127;
    if(op==0x0e){target=same(b+q+2,(const UINT8*)&MenuGuids[2],16);fsDepth=depth+1;}
    if(target){
     if(op==0x24&&len==28&&same(b+q+2,setupVarGuid,16)&&b[q+18]==1&&b[q+19]==0xf1&&b[q+20]==0xff&&b[q+21]==0&&same(b+q+22,name,6))vars++;
     if(op==0x05&&len==17&&scope&&b[q+6]==0xa6&&b[q+7]==0&&b[q+8]==1&&b[q+9]==0xf1&&b[q+10]==0x8a&&b[q+11]==0&&(b[q+13]&3)==0&&b[q+14]==0&&b[q+15]==1&&b[q+16]==0)rd++;
     if(op==0x12&&len==6&&!scope&&b[q+2]==0xa6&&b[q+3]==0&&b[q+4]==0&&b[q+5]==0&&q-prev==2&&b[prev]==0x0a&&b[prev+1]==0x82){
      count++;if(count>28){*matches=count;return EFI_UNSUPPORTED;}offsets[count-1]=q;
     }
    }
    if(op==0x29){if(depth==fsDepth){target=FALSE;fsDepth=0;}depth--;}
    else if(scope)depth++;
    prev=q;q+=len;
   }
  }
  p+=plen;
 }
 *matches=count;
 if(count!=28||vars!=1||rd!=1)return EFI_UNSUPPORTED;
 /* All checks finish before the first mutation. Keep opcode and length intact. */
 for(i=0;i<count;i++){b[offsets[i]+4]=0xff;b[offsets[i]+5]=0xff;}
 return EFI_SUCCESS;
}
static BOOLEAN token_equal(const CHAR16 *a,const CHAR16 *b){UINTN i=0;while(a[i]&&b[i]){if(a[i]!=b[i])return FALSE;i++;}return a[i]==b[i];}
EFI_STATUS MenuCommandMode(const CHAR16 *options,UINT32 bytes,UINT8 *mode){
 UINTN n,i=0,tokens=0;BOOLEAN seen=FALSE,terminated=FALSE;
 if(!mode)return EFI_INVALID_PARAMETER;*mode=MENU_MODE_MENU;
 if(!bytes)return EFI_SUCCESS;
 if(!options||bytes%2||bytes>8192)return EFI_INVALID_PARAMETER;
 n=bytes/2;
 {UINTN k;for(k=0;k<n;k++)if(options[k]==0){n=k;terminated=TRUE;break;}}
 if(!terminated)return EFI_INVALID_PARAMETER;
 while(i<n){
  CHAR16 token[260];UINTN k=0;BOOLEAN quote=FALSE;
  while(i<n&&(options[i]==' '||options[i]=='\t'))i++;
  if(i==n)break;
  while(i<n){CHAR16 c=options[i++];if(c=='"'){quote=!quote;continue;}if(!quote&&(c==' '||c=='\t'))break;if(k>=259)return EFI_INVALID_PARAMETER;token[k++]=c;}
  if(quote||!k)return EFI_INVALID_PARAMETER;token[k]=0;
  if(token_equal(token,L"--enable-rd")||token_equal(token,L"--disable-rd")||token_equal(token,L"--menu")){
   if(seen)return EFI_INVALID_PARAMETER;seen=TRUE;
   *mode=token_equal(token,L"--enable-rd")?MENU_MODE_ENABLE:token_equal(token,L"--disable-rd")?MENU_MODE_DISABLE:MENU_MODE_MENU;
  }else{
   if(tokens||seen||k<4||token[k-4]!='.'||(token[k-3]!='e'&&token[k-3]!='E')||(token[k-2]!='f'&&token[k-2]!='F')||(token[k-1]!='i'&&token[k-1]!='I'))return EFI_INVALID_PARAMETER;
  }
  tokens++;
 }
 return EFI_SUCCESS;
}
