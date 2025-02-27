MAX_DIST             =50                           ;max distance between sshapes, must be even value                           ¬
MAX_ROTATIONS_LEVEL  =6                            ;                                                                           |
sizeof.DistanceMap   =1 shl (MAX_ROTATIONS_LEVEL+5);                                                                           |
MAX_ROTATIONS        =1 shl MAX_ROTATIONS_LEVEL    ;if you change this constant you also need to recalc Angle2Degrees variable  \
DIST_TOP             =(MAX_ROTATIONS*0) shr 2      ;                                                                             this constants also redefined in shaders
DIST_RIGHT           =(MAX_ROTATIONS*1) shr 2      ;                                                                            /
DIST_BOTTOM          =(MAX_ROTATIONS*2) shr 2      ;                                                                           |
DIST_LEFT            =(MAX_ROTATIONS*3) shr 2      ;                                                                          _|

MAX_SHEETS           =64 ;maximum count of sheets

COREL_COLORCHANGE    =$B001 ;CorelDraw sends this message to all childs when color scheme is changed
KEY_WOW64_64KEY      =256
E_NOTIMPL            =80004001h
E_FAIL               =80004005h
SelectionChange      =11h
OnPluginCommand      =14h
OnUpdatePluginCommand=15h

macro DLGTEMPLATEEX helpID,exStyle,style,cDlgItems,x,y,cx,cy,_menu,title,pointsize,weight,italic,charset,typeface{
  dd 0xFFFF0001,helpID,exStyle,style
  dw cDlgItems,x,y,cx,cy,_menu,0
  du title,0
  dw pointsize,weight
  db italic,charset
  du typeface,0
}

macro DLGITEMTEMPLATEEX helpID,exStyle,style,x,y,cx,cy,id,class,title{
  align 4
  dd helpID,exStyle,style
  dw x,y,cx,cy
  dd id
  if class eq 'BUTTON'
    dw 0FFFFh,80h
  else if class eq 'EDIT'
    dw 0FFFFh,81h
  else if class eq 'STATIC'
    dw 0FFFFh,82h
  else if class eq 'LISTBOX'
    dw 0FFFFh,83h
  else if class eq 'SCROLLBAR'
    dw 0FFFFh,84h
  else if class eq 'COMBOBOX'
    dw 0FFFFh,85h
  else
    du class,0
  end if
  du title,0
  dw 0
}

struct TNestingParams
  SheetWidth     rd 1
  HeightMapWidth rd 1
  Rotations      rd 1
  reserved       rd 1
ends

sizeof.TSheet.Heightmap=(10000+MAX_DIST)*4

struct TSheet
  FBO       rd 1
  Texture   rd 1
  Empty     rd 1
  Area      rd 1
  HeightMap rd 10000+MAX_DIST
ends

pfd                     PIXELFORMATDESCRIPTOR sizeof.PIXELFORMATDESCRIPTOR,1,PFD_DRAW_TO_WINDOW+PFD_SUPPORT_OPENGL+PFD_DOUBLEBUFFER,PFD_TYPE_RGBA,32
GPUNestParamsVariant:
  dq VT_I8
GPUNestParams:
 .SheetWidth        dw 400
 .SheetHeight       dw 400
 .FitToBiggest      db 0
 .NestingAlgo       db 2
 .RotationsLevel    db 2
 .MinDist           db 4
                    rq 1
align 16
ClearColor:
  .r  dd 1.0
  .g  dd 1.0
  .b  dd 1.0
  .a  dd 1.0
Preview:
      .x      rd 1
      .y      rd 1
      .Width  rd 1
      .Height rd 1
      .matrix dd 1.0,0,0,0,\
                 0,1.0,0,0,\
                 0,0,1.0,0,\
                 0,0,0,1.0
      .scale  dd 1.0
      .Shader rd 1
      .DC     rq 1
IID_ICUIApplication     db 0x0a,0x00,0xee,0x9c,0xa0,0x42,0x80,0x59,0x43,0xa3,0x7a,0xa7,0x14,0x61,0x48,0x2c
flt_rcp_255             dd 0.0039215686274509803921568627451,0.0039215686274509803921568627451,0.0039215686274509803921568627451,0.0039215686274509803921568627451 ;1/255
dbl_05:                 dq 0.5,0.5
flt_1                   dd 1.0,1.0,1.0,1.0
flt_05:                 dd 0.5,0.5,0.5,0.5
flt_sign_mask           dd 80000000h,80000000h,80000000h,80000000h
flt_exp_0_0_1_2         dd 0,0,1 shl 23,2 shl 23
Angle2Degrees           dq 5.625 ;360.0/MAX_ROTATIONS
MaxScale                dd 1000.0
MinScale                dd 0.01
flt_exp_9               dd 9 shl 23
MainDlg:                DLGTEMPLATEEX 0,0,DS_SETFONT+DS_MODALFRAME+DS_CENTER+WS_MAXIMIZEBOX+WS_POPUP+WS_VISIBLE+WS_CLIPCHILDREN+WS_CAPTION+WS_SYSMENU+WS_THICKFRAME,1,0,0,460,320,0,'',9,FW_NORMAL,0,0,''
                        DLGITEMTEMPLATEEX 0,0,SS_LEFT+WS_CHILD+WS_VISIBLE ,2 ,2 ,30 ,40,0,'STATIC',''

ControlPanelDlg:        DLGTEMPLATEEX 0,0,DS_SETFONT+WS_CHILD+WS_VISIBLE,13,0,0,132,60,0,'',10,FW_NORMAL,0,0,'LUCIDA CONSOLE'
                        ;encoding macro don`t work here. I don`t know why. So I define unicode-string by hand.
                        CAPTIONS equ 32,32,1056,1072,1079,1084,1077,1089,1090,1080,1090,1100,32,1074,1085,1091,1090,1088,1080,32,32,32,32,32,32,1085,1072,1080,1073,1086,1083,1100,1096,1077,1081,32,1092,1080,1075,1091,1088,1099,13,10,13,10,1064,1080,1088,1080,1085,1072,32,1083,1080,1089,1090,1072,13,10,13,10,1042,1099,1089,1086,1090,1072,32,1083,1080,1089,1090,1072,13,10,13,10,32,9484,9472,9472,9472,32,1057,1090,1088,1072,1090,1077,1075,1080,1103,32,9472,9472,9472,9488,13,10,32,9474,32,32,32,1055,1086,32,1082,1086,1086,1088,1076,1080,1085,1072,1090,1072,1084,9474,13,10,32,9474,32,32,32,1055,1086,32,1082,1072,1088,1090,1077,32,1074,1099,1089,1086,1090,9474,13,10,32,9474,32,32,32,1055,1086,32,1074,1099,1089,1086,1090,1077,32,32,32,32,32,9474,13,10,32,9492,9472,9472,9472,9472,9472,9472,9472,9472,9472,9472,9472,9472,9472,9472,9472,9472,9472,9496,13,10,13,10,1050,1086,1083,1080,1095,1077,1089,1090,1074,1086,32,1074,1088,1072,1097,1077,1085,1080,1081,13,10,13,10,13,10,13,10,13,10,1052,1080,1085,1080,1084,1072,1083,1100,1085,1086,1077,32,1088,1072,1089,1089,1090,1086,1103,1085,1080,1077
                        DLGITEMTEMPLATEEX 0,0,SS_LEFT+WS_CHILD+WS_VISIBLE                               ,5 ,7 ,90 ,160,0,'STATIC',CAPTIONS
                        DLGITEMTEMPLATEEX 0,0,BS_AUTOCHECKBOX+WS_CHILD+WS_VISIBLE+WS_TABSTOP            ,3 ,9 ,8 ,12,1,'BUTTON',''
                        DLGITEMTEMPLATEEX 0,0,ES_LEFT+ES_NUMBER+WS_CHILD+WS_VISIBLE+WS_BORDER+WS_TABSTOP,60,30,20,12,2,'EDIT',''
                        DLGITEMTEMPLATEEX 0,0,ES_LEFT+ES_NUMBER+WS_CHILD+WS_VISIBLE+WS_BORDER+WS_TABSTOP,60,45,20,12,3,'EDIT',''
                        DLGITEMTEMPLATEEX 0,0,BS_AUTORADIOBUTTON+WS_CHILD+WS_VISIBLE+WS_TABSTOP         ,14,72,6,6,4,'BUTTON',''
                        DLGITEMTEMPLATEEX 0,0,BS_AUTORADIOBUTTON+WS_CHILD+WS_VISIBLE+WS_TABSTOP         ,14,80,6,6,5,'BUTTON',''
                        DLGITEMTEMPLATEEX 0,0,BS_AUTORADIOBUTTON+WS_CHILD+WS_VISIBLE+WS_TABSTOP         ,14,88,6,6,6,'BUTTON',''
                        DLGITEMTEMPLATEEX 0,0,TBS_HORZ+WS_CHILD+WS_VISIBLE+WS_TABSTOP                   ,0,120,96,18,7,'msctls_trackbar32',''
                        DLGITEMTEMPLATEEX 0,0,TBS_HORZ+WS_CHILD+WS_VISIBLE+WS_TABSTOP                   ,0,160,96,18,8,'msctls_trackbar32',''
                        DLGITEMTEMPLATEEX 0,0,SS_CENTER+WS_CHILD+WS_VISIBLE+WS_GROUP                    ,44,138,10,10,9,'STATIC',''
                        DLGITEMTEMPLATEEX 0,0,SS_CENTER+WS_CHILD+WS_VISIBLE+WS_GROUP                    ,44,178,10,10,10,'STATIC',''
                        DLGITEMTEMPLATEEX 0,0,WS_CHILD+WS_VISIBLE+WS_TABSTOP                            ,6,210,50,14,11,'BUTTON','Применить'
                        DLGITEMTEMPLATEEX 0,0,WS_CHILD+WS_VISIBLE+WS_TABSTOP                            ,59,210,30,14,12,'BUTTON','OK'

                        dd sizeof.strGPUNest-2
strGPUNest              du 'GPUNest',0
sizeof.strGPUNest=$-strGPUNest
                        dd 26
strGPUNestParams        du 'GPUNestParams',0
                        dd 24
strPageMatColor         du 'PageMatColor',0
                        dd 30
strCustomizationDS      du 'CustomizationDS',0
                        dd 16
strStandard             du 'Standard',0
                        dd 20
strButtonCaption        du 'Компоновка',0
WindowSchemeRegPath     du 'Software\Corel\CorelDRAW\%i.0\Draw\Application Preferences\WindowScheme',0
strTen                  du '10',0
strCancel               du 'Прервать',0
strApply                du 'Применить',0
errCompileShader        db 'Ошибка компиляции шейдера %s',0
errVideoNotSupported    db 'Функция %s не поддерживается драйвером видеокарты.',0
align 2
errCPUNotSupported      du 'Процессор не поддерживается. Требуется SSE 4.1.',0
errVersionNotSupported  du 'Эта версия CorelDraw не поддерживается. Требуется версия 17 или выше.',0
errTDR                  du 'Превышен лимит времени на выполнение шейдера. Работа будет прекращена.',13,10,13,10,'Чтобы отключить лимитирование создайте ключ реестра с именем TdrLevel типом REG_DWORD и значением 0 в разделе "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers" и перезагрузите компьютер.',0
errRAM                  du 'Не достаточно оперативной памяти.',0
errVideoRAM             du 'Не достаточно видеопамяти.',0
errSheets               du 'Превышено максимальное количество листов.',0
fmtInt2Str              du '%i',0

ICOData                 file 'icon.ico'
sizeof.ICOData=$-ICOData
BMPData                 file 'icon.bmp'
sizeof.BMPData=$-BMPData

ShaderNames:
  .VertexShader db 'VertexShader',0
  .CS_CalcArea  db 'CalcArea.cs',0
  .GS_GetBounds db 'GetBounds.gs',0
  .FS_GetBounds db 'GetBounds.fs',0
  .VS_Place     db 'Place.vs',0
  .GS_Place     db 'Place.gs',0
  .FS_Place     db 'Place.fs',0
  .GS_Put       db 'Put.gs',0
  .FS_Put       db 'Put.fs',0
  .GS_Preview   db 'Preview.gs',0
  .FS_Preview   db 'Preview.fs',0

VertexShader file 'Shaders\Minified\VertexShader.vs'
             db 0
GS_GetBounds file 'Shaders\Minified\GetBounds.gs'
             db 0
FS_GetBounds file 'Shaders\Minified\GetBounds.fs'
             db 0
VS_Place     file 'Shaders\Minified\Place.vs'
             db 0
GS_Place     file 'Shaders\Minified\Place.gs'
             db 0
FS_Place     file 'Shaders\Minified\Place.fs'
             db 0
GS_Put       file 'Shaders\Minified\Put.gs'
             db 0
FS_Put       file 'Shaders\Minified\Put.fs'
             db 0
GS_Preview   file 'Shaders\Minified\Preview.gs'
             db 0
FS_Preview   file 'Shaders\Minified\Preview.fs'
             db 0
CS_CalcArea  file 'Shaders\Minified\CalcArea.cs'
             db 0

GLExtFunctions:
  GLExt glCreateShader
  GLExt glShaderSource
  GLExt glCompileShader
  GLExt glGetShaderiv
  GLExt glGetShaderInfoLog
  GLExt glAttachShader
  GLExt glDeleteShader
  GLExt glGenBuffers
  GLExt glBindBuffer
  GLExt glBindBufferBase
  GLExt glBufferData
  GLExt glDeleteBuffers
  GLExt glGenFramebuffers
  GLExt glBindFramebuffer
  GLExt glDeleteFramebuffers
  GLExt glFramebufferTexture2D
  GLExt glCreateProgram
  GLExt glLinkProgram
  GLExt glUseProgram
  GLExt glDeleteProgram
  GLExt glUniform1i
  GLExt glUniform4f
  GLExt glUniform4fv
  GLExt glUniformMatrix4fv
  GLExt glNamedBufferSubData
  GLExt glGetNamedBufferSubData
  GLExt glGenVertexArrays
  GLExt glBindVertexArray
  GLExt glVertexAttribPointer
  GLExt glEnableVertexAttribArray
  GLExt glDeleteVertexArrays
  GLExt glDrawArrays
  GLExt glDispatchCompute
  GLExt glActiveTexture

data export
    export 0,\
           AttachPlugin,'AttachPlugin',\
           EventTrack.dwHoverTime,'NvOptimusEnablement',\                   ;EventTrack.dwHoverTime consist 1
           EventTrack.dwHoverTime,'AmdPowerXpressRequestHighPerformance'
end data

data fixups
end data