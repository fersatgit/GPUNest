format PE64 GUI 4.0 DLL as 'cpg'
entry DllEntryPoint

macro GLExt name{
  label name qword
  db `name,0
}

include 'encoding\win1251.inc'
include 'win64w.inc'
include '..\OpenGL.inc'
include '..\Data.inc'
include 'Import.inc'

prologue@proc equ static_rsp_prologue
epilogue@proc equ static_rsp_epilogue
close@proc equ static_rsp_close

IPlugin             dq IPluginVMT
IPluginVMT          dq QueryInterface,\
                       AddRef,\
                       Release,\
                       GetTypeInfoCount,\
                       GetTypeInfo,\
                       GetIDsOfNames,\
                       Invoke,\
                       OnLoad,\
                       StartSession,\
                       StopSession,\
                       OnUnload

struct NMCUSTOMDRAW
  hdr         NMHDR
  dwDrawStage rd 2
  hdc         rq 1
  rc          RECT
  dwItemSpec  rq 1
  uItemState  rd 2
  lItemlParam rq 1
ends

struct THeightMap
  Width            rd 1
  Data             rq 1  ;array[0..MAX_ROTATIONS-1] of packed record
                         ;                             Len:       dword;  //это значимая длина HeighMap
                         ;                             HeightMap: array[0..Width-2] of dword;
                         ;                             end;
ends

struct TNestNode
  Shape               rq 1 ;IVGShape
  BitmapHandle        rq 1
  DC                  rq 1
  Raster              rq 1
  x                   rq 1
  y                   rq 1
  Width               rq 1
  Height              rq 1
  DistanceMap         rd MAX_ROTATIONS*8   ;array[0..MAX_ROTATIONS*2-1] of packed record x,y,z,w: integer; end;
  cx                  rd 1
  cy                  rd 1
  angle               rd 1
  texture             rd 1
  _align1             rd 1
  HeightMap           THeightMap
  TexWidth            rd 1
  TexHeight           rd 1
  _align2             rd 2
ends

include 'CorelDraw.inc'
include 'Nesting.inc'
include 'GUI.inc'

align 16
DllEntryPoint: ;hinstDLL,fdwReason,lpvReserved
  mov eax,TRUE
ret

AttachPlugin: ;ppIPlugin: IVGAppPlugin
  mov qword[rcx],IPlugin
  mov eax,256
ret

QueryInterface:   ;(const self:IVGAppPlugin; const IID: TGUID; out Obj): HResult; stdcall;
  mov qword[r8],IPlugin
AddRef:           ;(const self:IVGAppPlugin):Integer; stdcall;
Release:          ;(const self:IVGAppPlugin):Integer; stdcall;
  xor eax,eax
ret
GetTypeInfoCount: ;(const self:IVGAppPlugin; out Count: Integer): HResult; stdcall;
GetTypeInfo:      ;(const self:IVGAppPlugin; Index, LocaleID: Integer; out TypeInfo): HResult; stdcall;
GetIDsOfNames:    ; this,IID,Names,NameCount,LocaleID,DispIDs
  mov eax,E_NOTIMPL
ret

proc Invoke this,DispID,IID,LocaleID,Flags,Params,VarResult,ExcepInfo,ArgErr
  cmp edx,SelectionChange
  je .SelectionChange
  cmp edx,OnPluginCommand
  je .OnPluginCommand
  cmp edx,OnUpdatePluginCommand
  je .OnUpdatePluginCommand
  xor eax,eax
  ret
        .SelectionChange:cominvk CorelApp,Get_ActiveSelectionRange,Selection
                         test    eax,eax
                         jne @f
                           cominvk Selection,Get_Count,Enabled
                           cominvk Selection,Release
                         @@:
                         xor     eax,eax
                         ret
        .OnPluginCommand:mov    rax,[Params]
                         mov    rax,[rax+DISPPARAMS.rgvarg]
                         invoke lstrcmpW,[rax+VARIANT.data],strGPUNest
                         test   eax,eax
                         jne    @f
                           cominvk CorelApp,Get_ActiveSelectionRange,Selection
                           cominvk CorelApp,Get_ActiveDocument,CorelDoc
                           cominvk CorelDoc,BeginCommandGroup,0
                           cominvk CorelDoc,Set_Unit,cdrMillimeter
                           cominvk CorelDoc,Set_ReferencePoint,cdrCenter
                           cominvk CorelApp,Get_ActiveWindow,CorelWnd
                           cominvk CorelWnd,Get_Handle,CorelWndHandle
                           cominvk CorelWnd,Release
                           invoke  GetParent,[CorelWndHandle]
                           mov     [CorelWndHandle],rax
                           mov     [SheetsCount],0
                           invoke  DialogBoxIndirectParamW,0,MainDlg,rax,DialogFunc,0
                           cominvk Selection,Release
                           cominvk CorelDoc,EndCommandGroup
                           cominvk CorelDoc,Release
                           cominvk CorelApp,Refresh
                         @@:
                         xor     eax,eax
                         ret
  .OnUpdatePluginCommand:xchg   rbx,[Params]
                         mov    rbx,[rbx+DISPPARAMS.rgvarg]
                         invoke lstrcmpW,[rbx+sizeof.VARIANT*2+VARIANT.data],strGPUNest
                         test   eax,eax
                         jne    @f
                           mov rax,[rbx+sizeof.VARIANT*1+VARIANT.data]
                           mov edx,[Enabled]
                           mov [rax],dx
                         @@:
                         mov    rbx,[Params]
                         xor    eax,eax
                         ret
endp

proc OnLoad uses rbx ;(const self:IVGAppPlugin; const _Application: IVGApplication):LongInt;stdcall;
  mov     rbx,rdx
  mov     [CorelApp],rdx
  comcall rbx,IVGApplication,AddRef
  comcall rbx,IVGApplication,Get_VersionMinor,CorelVersion
  comcall rbx,IVGApplication,Get_VersionMajor,CorelVersion+1
ret
endp

proc StartSession uses rbx     ;(const self:IVGAppPlugin):LongInt;stdcall;
  cmp   byte[CorelVersion+1],17
  mov   rax,errVersionNotSupported
  jnae  .error
  mov   eax,1
  cpuid
  test  ecx,1 shl 19 ;SSE 4.1
  mov   rax,errCPUNotSupported
  je    .error
    mov     rbx,[CorelApp]
    comcall rbx,IVGApplication,AddPluginCommand,strGPUNest,strButtonCaption,strButtonCaption,buf
    comcall rbx,IVGApplication,AdviseEvents,IPlugin,EventsCookie
    comcall rbx,IVGApplication,Get_CommandBars,CommandBars
    cominvk CommandBars,Get_Item,varStrStandard,CommandBar
    cominvk CommandBar,Get_Controls,Controls
    cominvk Controls,Get_Count,buf
    mov     ebx,dword[buf]
    @@:cominvk Controls,Get_Item,rbx,Control
       cominvk Control,Get_Caption,buf
       invoke  lstrcmpW,qword[buf],strButtonCaption
       test    eax,eax
       je @f
       cominvk Control,Release
       dec     ebx
    jne @b
    cominvk Controls,AddCustomButton,cdrCmdCategoryPlugins,strGPUNest,1,0,Control
    @@:
    invoke  GetTempPathW,bufsize/2,buf+4
    lea     eax,[eax*2+sizeof.strGPUNest+6]
    mov     dword[buf],eax
    lea     rcx,[buf+rax-sizeof.strGPUNest-2]
    invoke  lstrcpyW,rcx,strGPUNest
    cmp    [CorelVersion],1104h ;17.4
    jb .legacy
      mov     rdx,6F00630069002Eh                 ;'.ico'
      mov     [rax+sizeof.strGPUNest-2],rdx
      invoke  CreateFileW,buf+4,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0
      mov     rbx,rax
      invoke  WriteFile,rax,ICOData,sizeof.ICOData,BkColor,0
      invoke  CloseHandle,rbx
      cominvk Control,SetIcon2,buf+4
      jmp @f
    .legacy:
      mov     rdx,70006D0062002Eh                 ;'.bmp'
      mov     [rax+sizeof.strGPUNest-2],rdx
      invoke  CreateFileW,buf+4,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0
      mov     rbx,rax
      invoke  WriteFile,rax,BMPData,sizeof.BMPData,BkColor,0
      invoke  CloseHandle,rbx
      cominvk Control,SetCustomIcon,buf+4
    @@:
    cominvk Control,Release
    cominvk Controls,Release
    cominvk CommandBar,Release
    cominvk CommandBars,Release
    xor     eax,eax
    ret
  .error:
  invoke MessageBoxW,[CorelWndHandle],rax,strGPUNest,MB_TASKMODAL
  mov    eax,E_FAIL
ret
endp

proc StopSession      ;(const self:IVGAppPlugin):LongInt;stdcall;
  cominvk CorelApp,UnadviseEvents,[EventsCookie]
  xor     eax,eax
ret
endp

proc OnUnload         ;(const self:IVGAppPlugin)LongInt;stdcall;
  cominvk CorelApp,Release
  xor     eax,eax
ret
endp

varStrStandard        VARIANT VT_BSTR,strStandard
EventTrack:
  .cbSize      dd 24
  .dwFlags     dd 3 ;TME_HOVER+TME_LEAVE
  .hwndTrack   rq 1
  .dwHoverTime dq 1
bmi                   BITMAPINFOHEADER sizeof.BITMAPINFOHEADER,0,0,1,8
bmiColors             rd 256
align 16
buf                   rb 2048
bufsize=$-buf
delta                 rd 4
tmpVariant            VARIANT
DefEditProc           rq 1
SelectionRect:
  .x      rq 1
  .y      rq 1
  .width  rq 1
  .height rq 1
BkColor               rd 1
EditColor             rd 1
TextColor             rd 1
MinHeight             rd 1
EditBrush             rq 1
BkBrush               rq 1
BorderPen             rq 1
ContourPen            rq 1
CorelCLSID            rq 2
HighlightBrush        rq 1
HighlightPen          rq 1
CorelWndHandle        rq 1
RC                    rq 1
Mouse                 POINT
CalcArea              rd 1
Put                   rd 1
NotPlacedOfs          rq 1
CorelApp              IVGApplication
CorelCUIApp           ICUIApplication
CorelDataContext      ICUIDataContext
CustomizationDS       ICUIDataSourceProxy
CorelDoc              IVGDocument
CommandBars           ICUICommandBars
CommandBar            ICUICommandBar
Controls              ICUIControls
Control               ICUIControl
CorelWnd              IVGWindow
Shape                 IVGShape
Selection             IVGShapeRange
Curve                 IVGCurve
Bitmap                IVGBitmap
UserData              IVGProperties
wnds:
  DialogWindow          rq 1
  FitToBiggestCheckBox  rq 1
  WidthEdit             rq 1
  HeightEdit            rq 1
  ByCoordsRadio         rq 1
  ByHeightMapRadio      rq 1
  ByHeightRadio         rq 1
  RotationsTrackBar     rq 1
  MinDistTrackBar       rq 1
  RotationsValue        rq 1
  MinDistValue          rq 1
  ApplyButton           rq 1
  OkButton              rq 1
  PreviewWindow         rq 1
  ControlPanel          rq 1

TargetSheet           rq 1
CurveInfo             rq 1
NestNodes             rq 1
NestingOrder          rq 1
NFP                   rq 1
FrameBuffer           rq 1
PlacementMap          rq 1
Textures              rq 1
tmpHeightMap          rq 1
NestNodeCount         rd 1
FrameBufferLen        rd 1
PlacementMapLen       rd 1
NFPLen                rd 1
CurSheet              rd 1
OptimalPlace          rd 1
SSBO                  rd 3
VAO                   rd 1
VBO                   rd 1
FBO                   rd 1
Texture               rd 1
tmpHeightMapSize      rd 1
HeightMapSize         rd 1
SheetsCount           rd 1
StopNesting           rd 1
MaxDiameter           rd 1
NestingThreadId       rd 1
Place                 rd 1
GetBounds             rd 1
Sheets                rb MAX_SHEETS*sizeof.TSheet
NestingParams         TNestingParams
EventsCookie          rd 1
Enabled               rd 1
CorelVersion          rd 1