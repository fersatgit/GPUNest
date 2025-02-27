format PE GUI 4.0 DLL as 'cpg'
entry DllEntryPoint

macro GLExt name{
  label name dword
  db `name,0
}

include 'encoding\win1251.inc'
include 'win32w.inc'
include '..\OpenGL.inc'
include '..\Data.inc'
include 'Import.inc'

IPlugin             dd IPluginVMT
IPluginVMT          dd QueryInterface,\
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
  dwDrawStage rd 1
  hdc         rd 1
  rc          RECT
  dwItemSpec  rd 1
  uItemState  rd 1
  lItemlParam rd 1
ends

struct THeightMap
  Width            rd 1
  Data             rd 1  ;array[0..MAX_ROTATIONS-1] of packed record
                         ;                             Len:       dword;  //это значимая длина HeighMap
                         ;                             HeightMap: array[0..Width-2] of dword;
                         ;                             end;
ends

struct TNestNode
  Shape               rd 1 ;IVGShape
  BitmapHandle        rd 1
  DC                  rd 1
  texture             rd 1
  x                   rq 1
  y                   rq 1
  Width               rq 1
  Height              rq 1
  DistanceMap         rd MAX_ROTATIONS*8   ;array[0..MAX_ROTATIONS*2-1] of packed record x,y,z,w: integer; end;
  cx                  rd 1
  cy                  rd 1
  angle               rd 1
  HeightMap           THeightMap
  TexWidth            rd 1
  TexHeight           rd 1
  Raster              rd 1
ends

include 'CorelDraw.inc'
include 'Nesting.inc'
include 'GUI.inc'

align 16
DllEntryPoint: ;hinstDLL,fdwReason,lpvReserved
  mov eax,TRUE
ret 12

AttachPlugin: ;ppIPlugin: IVGAppPlugin
  mov eax,[esp+4]
  mov dword[eax],IPlugin
  mov eax,256
ret 4

QueryInterface:   ;(const self:IVGAppPlugin; const IID: TGUID; out Obj): HResult; stdcall;
  mov eax,[esp+12]
  mov dword[eax],IPlugin
  xor eax,eax
ret 12
AddRef:           ;(const self:IVGAppPlugin):Integer; stdcall;
Release:          ;(const self:IVGAppPlugin):Integer; stdcall;
  xor eax,eax
ret 4
GetTypeInfoCount: ;(const self:IVGAppPlugin; out Count: Integer): HResult; stdcall;
  mov eax,E_NOTIMPL
ret 8
GetTypeInfo:      ;(const self:IVGAppPlugin; Index, LocaleID: Integer; out TypeInfo): HResult; stdcall;
  mov eax,E_NOTIMPL
ret 12
GetIDsOfNames:    ;(const self:IVGAppPlugin; const IID: TGUID; Names: Pointer;NameCount, LocaleID: Integer; DispIDs: Pointer): HResult; stdcall;
  mov eax,E_NOTIMPL
ret 24

Invoke:           ;(const self:IVGAppPlugin; DispID: Integer; const IID: TGUID; LocaleID: Integer;Flags: Word; var Params; VarResult, ExcepInfo, ArgErr: Pointer): HResult; stdcall;
  mov eax,[esp+8]
  cmp eax,SelectionChange
  je .SelectionChange
  cmp eax,OnPluginCommand
  je .OnPluginCommand
  cmp eax,OnUpdatePluginCommand
  je .OnUpdatePluginCommand
  xor eax,eax
  ret 36
        .SelectionChange:cominvk CorelApp,Get_ActiveSelectionRange,Selection
                         test    eax,eax
                         jne @f
                           cominvk Selection,Get_Count,Enabled
                           cominvk Selection,Release
                         @@:
                         xor     eax,eax
                         ret 36
        .OnPluginCommand:mov    eax,[esp+24]
                         mov    eax,[eax+DISPPARAMS.rgvarg]
                         invoke lstrcmpW,dword[eax+VARIANT.data],strGPUNest
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
                           mov     [CorelWndHandle],eax
                           mov     [SheetsCount],0
                           invoke  DialogBoxIndirectParamW,0,MainDlg,eax,DialogFunc,0
                           cominvk Selection,Release
                           cominvk CorelDoc,EndCommandGroup
                           cominvk CorelDoc,Release
                           cominvk CorelApp,Refresh
                         @@:
                         xor     eax,eax
                         ret 36
  .OnUpdatePluginCommand:xchg   ebx,[esp+24]
                         mov    ebx,[ebx+DISPPARAMS.rgvarg]
                         invoke lstrcmpW,dword[ebx+sizeof.VARIANT*2+VARIANT.data],strGPUNest
                         test   eax,eax
                         jne    @f
                           mov eax,dword[ebx+sizeof.VARIANT*1+VARIANT.data]
                           mov edx,[Enabled]
                           mov [eax],dx
                         @@:
                         mov    ebx,[esp+24]
                         xor    eax,eax
                         ret 36

OnLoad:           ;(const self:IVGAppPlugin; const _Application: IVGApplication):LongInt;stdcall;
  xchg    ebx,[esp+8]
  mov     [CorelApp],ebx
  comcall ebx,IVGApplication,AddRef
  comcall ebx,IVGApplication,Get_VersionMinor,CorelVersion
  comcall ebx,IVGApplication,Get_VersionMajor,CorelVersion+1
  mov     ebx,[esp+8]
ret 8

StartSession:     ;(const self:IVGAppPlugin):LongInt;stdcall;
  cmp     byte[CorelVersion+1],17
  mov     eax,errVersionNotSupported
  jnae    .error
  mov     eax,1
  cpuid
  test    ecx,1 shl 19 ;SSE 4.1
  mov     eax,errCPUNotSupported
  je .error
    push    ebx
    mov     ebx,[CorelApp]
    comcall ebx,IVGApplication,AddPluginCommand,strGPUNest,strButtonCaption,strButtonCaption,buf
    comcall ebx,IVGApplication,AdviseEvents,IPlugin,EventsCookie
    comcall ebx,IVGApplication,Get_CommandBars,CommandBars
    cominvk CommandBars,Get_Item,VT_BSTR,0,strStandard,0,CommandBar
    cominvk CommandBar,Get_Controls,Controls
    cominvk Controls,Get_Count,buf
    mov     ebx,dword[buf]
    @@:cominvk Controls,Get_Item,ebx,Control
       push    eax
       cominvk Control,Get_Caption,esp
       invoke  lstrcmpW,strButtonCaption
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
    lea     eax,[buf+eax-sizeof.strGPUNest-2]
    invoke  lstrcpyW,eax,strGPUNest
    cmp     [CorelVersion],1104h ;17.4
    push    buf+4
    jb .legacy
      mov     dword[eax+sizeof.strGPUNest-2],69002Eh ;'.i'
      mov     dword[eax+sizeof.strGPUNest+2],6F0063h ;'co'
      invoke  CreateFileW,buf+4,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0
      push    eax
      invoke  WriteFile,eax,ICOData,sizeof.ICOData,BkColor,0
      invoke  CloseHandle
      cominvk Control,SetIcon2
      jmp @f
    .legacy:
      mov     dword[eax+sizeof.strGPUNest-2],62002Eh ;'.b'
      mov     dword[eax+sizeof.strGPUNest+2],70006Dh ;'mp'
      invoke  CreateFileW,buf+4,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0
      push    eax
      invoke  WriteFile,eax,BMPData,sizeof.BMPData,BkColor,0
      invoke  CloseHandle
      cominvk Control,SetCustomIcon
    @@:
    cominvk Control,Release
    cominvk Controls,Release
    cominvk CommandBar,Release
    cominvk CommandBars,Release
    pop     ebx
    xor     eax,eax
    ret 4
  .error:
  invoke MessageBoxW,[CorelWndHandle],eax,strGPUNest,MB_TASKMODAL
  mov    eax,E_FAIL
ret 4

StopSession:      ;(const self:IVGAppPlugin):LongInt;stdcall;
  cominvk CorelApp,UnadviseEvents,[EventsCookie]
  xor     eax,eax
ret 4

OnUnload:         ;(const self:IVGAppPlugin)LongInt;stdcall;
  cominvk CorelApp,Release
  xor     eax,eax
ret 4

EventTrack:
  .cbSize      dd 16
  .dwFlags     dd 3 ;TME_HOVER+TME_LEAVE
  .hwndTrack   rd 1
  .dwHoverTime dd 1
bmi                   BITMAPINFOHEADER sizeof.BITMAPINFOHEADER,0,0,1,8
bmiColors             rd 256
align 16
buf                   rb 2048
bufsize=$-buf
delta                 rd 4
tmpVariant            VARIANT
SelectionRect:
  .x      rq 1
  .y      rq 1
  .width  rq 1
  .height rq 1
BkColor               rd 1
EditColor             rd 1
TextColor             rd 1
MinHeight             rd 1
BkBrush               rd 1
EditBrush             rd 1
BorderPen             rd 1
ContourPen            rd 1
CorelCLSID            rq 2
HighlightPen          rd 1
HighlightBrush        rd 1
CorelWndHandle        rd 1
RC                    rd 1
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
DefEditProc           rd 1
CurveInfo             rd 1
NestNodeCount         rd 1
NestNodes             rd 1
NestingOrder          rd 1
NFP                   rd 1
FrameBuffer           rd 1
PlacementMap          rd 1
Textures              rd 1
tmpHeightMap          rd 1
FrameBufferLen        rd 1
PlacementMapLen       rd 1
NFPLen                rd 1
TargetSheet           rd 1
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
wnds:
  DialogWindow          rd 1
  FitToBiggestCheckBox  rd 1
  WidthEdit             rd 1
  HeightEdit            rd 1
  ByCoordsRadio         rd 1
  ByHeightMapRadio      rd 1
  ByHeightRadio         rd 1
  RotationsTrackBar     rd 1
  MinDistTrackBar       rd 1
  RotationsValue        rd 1
  MinDistValue          rd 1
  ApplyButton           rd 1
  OkButton              rd 1
  PreviewWindow         rd 1
  ControlPanel          rd 1
Sheets                rb MAX_SHEETS*sizeof.TSheet
NestingParams         TNestingParams
EventsCookie          rd 1
Enabled               rd 1
CorelVersion          rd 1