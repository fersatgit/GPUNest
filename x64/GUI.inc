proc ButtonProc wnd,msg,wParam,lParam
local Rect:DQWORD
local _rbx:QWORD, _rdi:QWORD, _rsi:QWORD
  cmp edx,WM_PAINT
  je .WM_PAINT
  cmp edx,WM_LBUTTONUP
  je .WM_LBUTTONUP
  cmp edx,WM_MOUSEHOVER
  je .WM_MOUSEHOVER
  cmp edx,WM_MOUSELEAVE
  je .WM_MOUSELEAVE
  cmp edx,WM_MOUSEMOVE
  je .WM_MOUSEMOVE
  add rsp,58h
  jmp [DefWindowProcW]
  .WM_MOUSEHOVER:mov r8,1
  .WM_MOUSELEAVE:
       .WM_PAINT:mov    [_rsi],rsi
                 mov    [_rdi],rdi
                 mov    [_rbx],rbx
                 mov    rsi,rcx
                 cmp    r8,1
                 mov    rdi,BkBrush
                 mov    rax,HighlightBrush
                 cmove  rdi,rax
                 invoke GetDC,rcx
                 mov    rbx,rax
                 invoke GetClientRect,rsi,addr Rect
                 invoke SetBkMode,rbx,OPAQUE
                 invoke SelectObject,rbx,qword[rdi]
                 invoke SelectObject,rbx,qword[rdi+8]
                 invoke Rectangle,rbx,dword[Rect],dword[Rect+4],dword[Rect+8],dword[Rect+12]
                 invoke SetTextColor,rbx,[TextColor]
                 invoke SetBkMode,rbx,TRANSPARENT
                 invoke SendMessageW,rsi,WM_GETTEXT,bufsize shr 1,buf
                 invoke DrawTextW,rbx,buf,-1,addr Rect,DT_CENTER+DT_VCENTER+DT_SINGLELINE
                 invoke ValidateRect,rsi,0
                 invoke ReleaseDC,rsi,rbx
                 mov    rsi,[_rsi]
                 mov    rdi,[_rdi]
                 mov    rbx,[_rbx]
                 ret
   .WM_LBUTTONUP:xor    r8,r8
                 cmp    rcx,[OkButton]
                 sete   r8l
                 add    r8,11
                 invoke SendMessageW,[ControlPanel],WM_COMMAND,r8,0
                 ret
   .WM_MOUSEMOVE:mov    [EventTrack.hwndTrack],rcx
                 invoke TrackMouseEvent,EventTrack
                 ret
endp

proc EditProc uses rbx rdi, wnd,msg,wParam,lParam
  mov rdi,rcx
  mov [rsp+32],r9
  mov rbx,r8
  mov [msg],rdx
  cmp edx,WM_NCPAINT
  je .WM_NCPAINT
  cmp edx,WM_KILLFOCUS
  je .WM_KILLFOCUS
  cmp edx,WM_PASTE
  jne .DEFAULT
    ret
    .WM_NCPAINT:invoke GetDC,rcx
                mov    rbx,rax
                invoke SelectObject,rbx,[BorderPen]
                invoke SelectObject,rbx,[EditBrush]
                invoke Rectangle,rbx,-1,-3,38,17
                invoke ReleaseDC,rdi,rbx
                ret
  .WM_KILLFOCUS:invoke SendMessageW,rcx,WM_GETTEXT,bufsize,buf
                call   BufStr2IntW
                cmp    eax,10
                jae @f
                  invoke SendMessageW,rdi,WM_SETTEXT,0,strTen
                @@:
       .DEFAULT:invoke CallWindowProcW,[DefEditProc],rdi,[msg],rbx
                ret
endp

Rescale:
  movss    xmm0,[Preview.scale]
  movq     xmm1,qword[Preview.x]
  addss    xmm0,xmm0
  cvtpi2ps xmm2,qword[Preview.Width]
  shufps   xmm0,xmm0,0
  divps    xmm0,xmm2
  movaps   xmm2,xmm0
  mulps    xmm0,xmm1
  subps    xmm0,dqword[flt_1]
  movss    [Preview.matrix],xmm2
  psrlq    xmm2,32
  movq     qword[Preview.matrix+12*4],xmm0
  movss    [Preview.matrix+5*4],xmm2
ret

proc PreviewProc uses rbx rdi rsi rbp, wnd,msg,wParam,lParam
local tmpDQWORD:DQWORD
  cmp edx,WM_PAINT
  je .WM_PAINT
  cmp edx,WM_ERASEBKGND
  je .WM_ERASEBKGND
  cmp edx,WM_MOUSEMOVE
  je .WM_MOUSEMOVE
  cmp edx,WM_LBUTTONDOWN
  je .WM_LBUTTONDOWN
  cmp edx,WM_LBUTTONUP
  je .WM_LBUTTONUP
  stdcall [DefWindowProcW],rcx,rdx,r8,r9
  ret
   .WM_LBUTTONDOWN:movsx  eax,r9w
                   shr    r9,16
                   mov    [Mouse.x],eax
                   mov    [Mouse.y],r9d
                   invoke SetCapture,rcx
                   jmp .WM_PAINT
     .WM_LBUTTONUP:invoke ReleaseCapture
    .WM_ERASEBKGND:ret
     .WM_MOUSEMOVE:movd      xmm4,r9d
                   punpcklwd xmm4,xmm4
                   psrad     xmm4,16
                   test      r8,MK_LBUTTON
                   je @f
                     movaps   xmm1,dqword[Preview.x]
                     movq     xmm2,[Mouse]
                     movsldup xmm3,dqword[Preview.scale]
                     psubd    xmm2,xmm4
                     cvtdq2ps xmm2,xmm2
                     divps    xmm2,xmm3
                     addsubps xmm1,xmm2
                     movq     qword[Preview.x],xmm1
                     call     Rescale
                   @@:
                   movq      [Mouse],xmm4
         .WM_PAINT:invoke wglMakeCurrent,[Preview.DC],[RC]
                   invoke glBindFramebuffer,GL_FRAMEBUFFER,0
                   invoke glViewport,0,0,[Preview.Width],[Preview.Height]
                   invoke glClearColor,float [ClearColor.r],float [ClearColor.g],float [ClearColor.b],float [flt_1]
                   invoke glClear,GL_COLOR_BUFFER_BIT

;Draw shapes
                   invoke glEnable,GL_BLEND
                   invoke glLinkProgram,[Preview.Shader]
                   invoke glUseProgram,[Preview.Shader]
                   invoke glUniformMatrix4fv,1,1,GL_FALSE,Preview.matrix
                   invoke glActiveTexture,GL_TEXTURE0
                   mov    ebx,[NestNodeCount]
                   mov    rbp,[NestNodes]
                   @@:invoke   glBindTexture,GL_TEXTURE_2D,[rbp+TNestNode.texture]
                      cvtpi2ps xmm1,qword[Preview.Width]
                      cvtpi2ps xmm0,qword[rbp+TNestNode.TexWidth]
                      mulps    xmm1,[flt_05]
                      movlhps  xmm0,xmm1
                      movups   [tmpDQWORD],xmm0
                      invoke   glUniform4fv,0,1,addr tmpDQWORD
                      invoke   glBegin,GL_POINTS
                      cvtdq2ps xmm0,dqword[rbp+TNestNode.cx]
                      movups   [tmpDQWORD],xmm0
                      invoke   glVertex4fv,addr tmpDQWORD
                      invoke   glEnd
                      add      rbp,sizeof.TNestNode
                      dec      ebx
                   jne @b
                   invoke glDisable,GL_BLEND

;Draw sheet frames
                   invoke glUseProgram,0
                   invoke glLoadMatrixf,Preview.matrix
                   mov    ebx,[SheetsCount]
                   test   ebx,ebx
                   je @f
                     xor    esi,esi
                     movzx  edi,[GPUNestParams.SheetHeight]
                     movzx  ebp,[GPUNestParams.SheetWidth]
                     .DrawFrame:
                        invoke glBegin,GL_LINE_LOOP
                        xorps  xmm1,xmm1
                        xorps  xmm2,xmm2
                        invoke glColor4f,float [flt_1],xmm1,xmm2,float [flt_1]
                        invoke glVertex2i,esi,0
                        invoke glVertex2i,esi,edi
                        add    esi,ebp
                        invoke glVertex2i,esi,edi
                        invoke glVertex2i,esi,0
                        invoke glEnd
                        add    esi,MAX_DIST
                        dec    ebx
                     jne .DrawFrame
                   @@:

                   invoke glFinish
                   invoke SwapBuffers,[Preview.DC]
                   invoke ValidateRect,[PreviewWindow],0
                   ret
endp

BufStr2IntW:
  mov rdx,buf
  xor eax,eax
  jmp .start
      @@:movzx ecx,byte[rdx]
         lea   eax,[eax*4+eax]
         lea   eax,[eax*2+ecx-'0']
         add   rdx,2
  .start:cmp   word[rdx],0
  jne @b
ret

proc GetParams
  stdcall EditProc,[WidthEdit],WM_KILLFOCUS,0,0
  stdcall EditProc,[HeightEdit],WM_KILLFOCUS,0,0
  invoke  SendMessageW,[FitToBiggestCheckBox],BM_GETCHECK,0,0
  and     eax,BST_CHECKED
  mov     [GPUNestParams.FitToBiggest],al
  invoke  SendMessageW,[ByHeightRadio],BM_GETCHECK,0,0
  mov     ebx,eax
  invoke  SendMessageW,[ByHeightMapRadio],BM_GETCHECK,0,0
  lea     ebx,[ebx*2+eax]
  invoke  SendMessageW,[ByCoordsRadio],BM_GETCHECK,0,0
  lea     ebx,[ebx*2+eax]
  bsf     eax,ebx
  mov     [GPUNestParams.NestingAlgo],al
  invoke  SendMessageW,[RotationsTrackBar],TBM_GETPOS,0,0
  mov     [GPUNestParams.RotationsLevel],al
  invoke  SendMessageW,[MinDistTrackBar],TBM_GETPOS,0,0
  add     eax,eax
  mov     [GPUNestParams.MinDist],al
  invoke  SendMessageW,[WidthEdit],WM_GETTEXT,8,buf
  call    BufStr2IntW
  mov     [GPUNestParams.SheetWidth],ax
  invoke  SendMessageW,[HeightEdit],WM_GETTEXT,8,buf
  call    BufStr2IntW
  mov     [GPUNestParams.SheetHeight],ax
  ret
endp

proc ControlPanelFunc uses rbx rdi rsi rbp, wnd,msg,wParam,lParam
  cmp edx,WM_HSCROLL
  je .WM_HSCROLL
  cmp edx,WM_NOTIFY
  je .WM_NOTIFY
  cmp edx,WM_CTLCOLORSTATIC
  je .WM_CTLCOLORSTATIC
  cmp edx,WM_CTLCOLORDLG
  je .WM_CTLCOLORDLG
  cmp edx,WM_CTLCOLOREDIT
  je .WM_CTLCOLOREDIT
  cmp edx,WM_COMMAND
  je .WM_COMMAND
  cmp edx,WM_INITDIALOG
  je .WM_INITDIALOG
    xor eax,eax
  ret
         .WM_HSCROLL:cmp r9,[RotationsTrackBar]
                     jne @f
                       invoke SendMessageW,r9,TBM_GETPOS,0,0
                       mov    [GPUNestParams.RotationsLevel],al
                       mov    ecx,eax
                       mov    r8,1
                       shl    r8,cl
                       invoke wsprintfW,buf,fmtInt2Str,r8
                       mov    [buf+rax*2],0
                       invoke SendMessageW,[RotationsValue],WM_SETTEXT,0,buf
                       ret
                     @@:
                     invoke SendMessageW,r9,TBM_GETPOS,0,0
                     lea    ebx,[eax+eax]
                     movzx  edx,[GPUNestParams.MinDist]
                     mov    [GPUNestParams.MinDist],bl
                     mov    edi,ebx
                     sub    edi,edx
                     je @f
                       sar    edi,1
                       invoke wsprintfW,buf,fmtInt2Str,ebx
                       mov    [buf+eax*2],0
                       invoke SendMessageW,[MinDistValue],WM_SETTEXT,0,buf
                       invoke DeleteObject,[ContourPen]
                       invoke CreatePen,PS_SOLID,ebx,255
                       mov    [ContourPen],rax

                       mov    esi,[NestNodeCount]
                       mov    rbp,[NestNodes]
                       .UpdateContour:
                         invoke RestoreDC,[rbp+TNestNode.DC],-1
                         invoke BitBlt,[rbp+TNestNode.DC],0,0,[rbp+TNestNode.TexWidth],[rbp+TNestNode.TexHeight],[rbp+TNestNode.DC],0,[rbp+TNestNode.TexHeight],SRCCOPY
                         invoke SaveDC,[rbp+TNestNode.DC]
                         test   ebx,ebx
                         je .skip
                           invoke SelectObject,[rbp+TNestNode.DC],[ContourPen]
                           invoke StrokePath,[rbp+TNestNode.DC]
                         .skip:
                         invoke glBindTexture,GL_TEXTURE_2D,[rbp+TNestNode.texture]
                         invoke glTexSubImage2D,GL_TEXTURE_2D,0,0,0,[rbp+TNestNode.TexWidth],[rbp+TNestNode.TexHeight],GL_RED,GL_UNSIGNED_BYTE,[rbp+TNestNode.Raster]
                         mov    ecx,MAX_ROTATIONS*32
                         .UpdateDistanceMap:
                           add [rbp+TNestNode.DistanceMap+rcx-12],edi
                           sub ecx,16
                         jne .UpdateDistanceMap
                         add    rbp,sizeof.TNestNode
                         dec    esi
                       jne .UpdateContour
                       invoke PostMessageW,[PreviewWindow],WM_PAINT,0,0
                       mov    r8,1
                       jmp    .WM_COMMAND
                     @@:
                     ret
         .WM_COMMAND:cmp r8,1
                     je .FitToBiggestCheckBox
                     cmp r8,11
                     je .ApplyButton
                     cmp r8,12
                     je .OkButton
                     ret
                     .FitToBiggestCheckBox:invoke SendMessageW,[FitToBiggestCheckBox],BM_GETCHECK,0,0
                                           and    eax,BST_CHECKED
                                           xor    eax,BST_CHECKED
                                           mov    ebx,eax
                                           jne @f
                                             mov      eax,[NestNodeCount]
                                             shl      eax,3
                                             add      rax,[NestingOrder]
                                             mov      rax,[rax-8]
                                             movapd   xmm0,dqword[rax+TNestNode.Width]
                                             roundpd  xmm0,xmm0,2
                                             cvtpd2dq xmm0,xmm0
                                             movd     r8d,xmm0
                                             pextrd   edi,xmm0,1
                                             movzx    eax,[GPUNestParams.MinDist]
                                             add      r8d,eax
                                             add      edi,eax
                                             invoke   wsprintfW,buf,fmtInt2Str,r8
                                             mov      [buf+rax*2],0
                                             invoke   SendMessageW,[WidthEdit],WM_SETTEXT,0,buf
                                             invoke   wsprintfW,buf,fmtInt2Str,rdi
                                             mov      [buf+rax*2],0
                                             invoke   SendMessageW,[HeightEdit],WM_SETTEXT,0,buf
                                           @@:
                                           invoke EnableWindow,[WidthEdit],ebx
                                           invoke EnableWindow,[HeightEdit],ebx
                                           ret
                              .ApplyButton:cmp   [NestingThreadId],0
                                           setne byte[StopNesting]
                                           jne @f
                                             call   GetParams
                                             movzx  eax,[GPUNestParams.FitToBiggest]
                                             cmp    eax,[NestNodeCount]
                                             je @f
                                               invoke wglMakeCurrent,0,0
                                               invoke SendMessageW,[ApplyButton],WM_SETTEXT,0,strCancel
                                               invoke InvalidateRect,[ApplyButton],0,0
                                               invoke EnableWindow,[OkButton],0
                                               invoke EnableWindow,[RotationsTrackBar],0
                                               invoke EnableWindow,[MinDistTrackBar],0
                                               invoke CreateThread,0,0,Nesting,0,0,NestingThreadId
                                           @@:
                                           ret
                                 .OkButton:cominvk CorelApp,Set_Optimization,1
                                           mov     ebx,[NestNodeCount]
                                           mov     rbp,[NestNodes]
                                           @@:cvtpi2pd xmm2,qword[rbp+TNestNode.cx]
                                              addpd    xmm2,dqword[SelectionRect.x]
                                              movhlps  xmm3,xmm2
                                              comcall  [rbp+TNestNode.Shape],IVGShape,SetPositionEx,cdrCenter,xmm2,xmm3
                                              cvtsi2sd xmm1,[rbp+TNestNode.angle]
                                              mulsd    xmm1,[Angle2Degrees]
                                              comcall  [rbp+TNestNode.Shape],IVGShape,Rotate,xmm1
                                              add      rbp,sizeof.TNestNode
                                              dec      ebx
                                           jne @b
                                           cominvk CorelApp,Set_Optimization,0
                                           invoke  PostMessageW,[DialogWindow],WM_CLOSE,r8,r9
                                           ret
          .WM_NOTIFY:xor  eax,eax
                     cmp  [r9+NMCUSTOMDRAW.hdr.idFrom],7
                     je @f
                     cmp  [r9+NMCUSTOMDRAW.hdr.idFrom],8
                     je @f
                       ret
                     @@:
                     cmp  [r9+NMCUSTOMDRAW.hdr.code],NM_CUSTOMDRAW
                     jne @f
                       cmp  [r9+NMCUSTOMDRAW.dwDrawStage],CDDS_PREPAINT
                       mov  rbx,r9
                       mov  rdi,rcx
                       mov  r8,CDRF_NOTIFYITEMDRAW+256 ;CDRF_SKIPPOSTPAINT
                       je @f
                         invoke SelectObject,[rbx+NMCUSTOMDRAW.hdc],[BkBrush]
                         invoke SelectObject,[rbx+NMCUSTOMDRAW.hdc],[BorderPen]
                         invoke Rectangle,[rbx+NMCUSTOMDRAW.hdc],[rbx+NMCUSTOMDRAW.rc.left],[rbx+NMCUSTOMDRAW.rc.top],[rbx+NMCUSTOMDRAW.rc.right],[rbx+NMCUSTOMDRAW.rc.bottom]
                         invoke ReleaseDC,[rbx+NMCUSTOMDRAW.hdr.hwndFrom],[rbx+NMCUSTOMDRAW.hdc]
                         mov    r8,CDRF_SKIPDEFAULT
                     @@:
                     invoke SetWindowLongPtrW,rdi,DWLP_MSGRESULT,r8
                     mov    eax,1
                     ret
  .WM_CTLCOLORSTATIC:
     .WM_CTLCOLORDLG:mov    rdi,r8
                     invoke SetBkColor,rdi,[BkColor]
                     invoke SetTextColor,rdi,[TextColor]
                     mov    rax,[BkBrush]
                     ret
    .WM_CTLCOLOREDIT:mov    rdi,r8
                     invoke SetBkColor,rdi,[EditColor]
                     invoke SetTextColor,rdi,[TextColor]
                     mov    rax,[EditBrush]
                     ret
      .WM_INITDIALOG:mov  ebx,12
                     mov  rdi,rcx
                     @@:invoke GetDlgItem,rdi,ebx
                        mov    [wnds+rbx*8],rax
                        dec    ebx
                     jne @b
                     invoke SendMessageW,[RotationsTrackBar],TBM_SETRANGE,0,MAX_ROTATIONS_LEVEL shl 16
                     invoke SendMessageW,[MinDistTrackBar],TBM_SETRANGE,0,(MAX_DIST shr 1) shl 16
                     invoke PostMessageW,[WidthEdit],EM_LIMITTEXT,4,0
                     invoke PostMessageW,[HeightEdit],EM_LIMITTEXT,4,0
                     invoke SetWindowLongPtrW,[WidthEdit],GWLP_WNDPROC,EditProc
                     mov    [DefEditProc],rax
                     invoke SetWindowLongPtrW,[HeightEdit],GWLP_WNDPROC,EditProc
                     invoke SetWindowLongPtrW,[ApplyButton],GWLP_WNDPROC,ButtonProc
                     invoke SetWindowLongPtrW,[OkButton],GWLP_WNDPROC,ButtonProc
                     mov    eax,1
                     ret
endp

proc CompileShader _program,ShaderType,ShaderCode,ShaderName
  mov    qword[buf],r8
  mov    edi,ecx
  mov    rbp,r9
  invoke glCreateShader,rdx
  mov    ebx,eax
  invoke glShaderSource,ebx,1,buf,0
  invoke glCompileShader,ebx
  invoke glGetShaderiv,ebx,GL_COMPILE_STATUS,buf
  cmp    dword[buf],0
  jne @f
    invoke glGetShaderInfoLog,ebx,bufsize,BkColor,buf
    invoke wsprintfA,buf+1024,errCompileShader,rbp
    mov    byte[buf+1024+rax],0
    invoke MessageBoxA,0,buf,buf+1024,0
    add    rsp,48
    mov    rcx,[DialogWindow]
    jmp    DialogFunc.WM_CLOSE
  @@:
  invoke glAttachShader,edi,ebx
  invoke glDeleteShader,ebx
  ret
endp

;rbp - pointer to TNestNode
;rbx - DC
PointTypes db PT_MOVETO,PT_LINETO,PT_BEZIERTO,PT_BEZIERTO
proc DrawShape Shape:QWORD
  mov      [Shape],rcx
  comcall  rcx,IVGShape,Get_type,buf
  mov      eax,dword[buf]
  cmp      eax,cdrGroupShape
  ja      .cdrRectangleShape
    jmp     [.case+rax*8]
    .case dq .cdrRectangleShape,.cdrRectangleShape,.cdrEllipseShape,.cdrCurveShape,.cdrRectangleShape,.cdrBitmapShape,.cdrRectangleShape,.cdrGroupShape
    .cdrRectangleShape:mov      r12,[Rectangle]
                       jmp @f
      .cdrEllipseShape:mov      r12,[Ellipse]
                    @@:cominvk  Shape,GetBoundingBox,buf,buf+8,buf+16,buf+24,0
                       cvtpi2pd xmm0,qword[rbp+TNestNode.TexWidth]
                       movapd   xmm1,dqword[buf]
                       subpd    xmm0,dqword[rbp+TNestNode.Width]
                       subpd    xmm1,dqword[rbp+TNestNode.x]
                       mulpd    xmm0,[dbl_05]
                       addpd    xmm0,xmm1
                       cvtpd2dq xmm1,xmm0
                       addpd    xmm0,dqword[buf+16]
                       cvtpd2dq xmm0,xmm0
                       movd     edx,xmm1
                       pextrd   r8d,xmm1,1
                       movd     r9d,xmm0
                       pextrd   eax,xmm0,1
                       stdcall  r12,rbx,rdx,r8,r9,rax
                       ret
        .cdrCurveShape:comcall [Shape],IVGShape,Get_Curve,Curve
            .drawCurve:cominvk Curve,GetCurveInfo,CurveInfo
                       cvtpi2pd xmm0,qword[rbp+TNestNode.TexWidth]
                       movapd   xmm1,dqword[rbp+TNestNode.x]
                       subpd    xmm0,dqword[rbp+TNestNode.Width]
                       mulpd    xmm0,[dbl_05]
                       subpd    xmm1,xmm0
                       mov      rdx,[CurveInfo]
                       mov      ecx,[rdx+SAFEARRAY.rgsabound+SAFEARRAYBOUND.cElements]
                       mov      dword[buf+24],ecx
                       mov      rdx,[rdx+SAFEARRAY.pvData]
                       lea      r12,[rdx+rcx*sizeof.POINT]
                       mov      qword[buf+16],rdx
                       neg      rcx
                       @@:movupd   xmm0,dqword[rdx+CurveElement.PositionX]
                          subpd    xmm0,xmm1
                          cvtpd2dq xmm0,xmm0
                          movq     [r12+rcx*sizeof.POINT],xmm0
                          mov      eax,[rdx+CurveElement.ElementType]
                          mov      al,byte[PointTypes+rax]
                          mov      [rsp+rcx],al
                          add      rdx,sizeof.CurveElement
                          inc      rcx
                       jne @b
                       mov     r12d,dword[buf+24]
                       mov     rax,rsp
                       sub     rax,r12
                       add     r12,32+15
                       and     r12,-16
                       sub     rsp,r12
                       invoke  PolyDraw,rbx,qword[buf+16],rax,dword[buf+24]
                       invoke  SafeArrayDestroy,[CurveInfo]
                       cominvk Curve,Release
                       add     rsp,r12
                       ret
       .cdrBitmapShape:comcall [Shape],IVGShape,Get_Bitmap,Bitmap
                       cominvk Bitmap,Get_CropEnvelope,Curve
                       cominvk Bitmap,Release
                       jmp .drawCurve
        .cdrGroupShape:sub     rsp,64
                       comcall [Shape+64],IVGShape,Get_Shapes,addr rsp+32
                       comcall qword[rsp+32],IVGShapeRange,Get_Count,addr rsp+48
                       mov     qword[rsp+40],VT_I4
                       @@:comcall qword[rsp+32],IVGShapeRange,Get_Item,addr rsp+40,addr rsp+64
                          stdcall DrawShape,qword[rsp+64]
                          comcall qword[rsp+64],IVGShape,Release
                          dec     dword[rsp+48]
                       jne @b
                       comcall qword[rsp+32],IVGShapeRange,Release
                       add     rsp,64
                       ret
endp

proc DialogFunc uses rbx rdi rsi rbp, wnd,msg,wParam,lParam
  cmp edx,WM_MOUSEWHEEL
  je .WM_MOUSEWHEEL
  cmp edx,WM_SIZE
  je .WM_SIZE
  cmp edx,WM_GETMINMAXINFO
  je .WM_GETMINMAXINFO
  cmp edx,COREL_COLORCHANGE
  je .COREL_COLORCHANGE
  cmp edx,WM_INITDIALOG
  je .WM_INITDIALOG
  cmp edx,WM_CLOSE
  je .WM_CLOSE
  cmp edx,WM_DESTROY
  je .WM_DESTROY
    xor eax,eax
  ret
      .WM_MOUSEWHEEL:mov       rbx,r8
                     sar       ebx,16
                     invoke    GetWindowRect,[PreviewWindow],buf
                     cvtsi2ss  xmm3,ebx
                     psubd     xmm3,dqword[flt_exp_9]
                     addss     xmm3,[flt_1]
                     movd      xmm0,r9d
                     movsldup  xmm1,dqword[Preview.scale]
                     punpcklwd xmm0,xmm0
                     movsldup  xmm3,xmm3
                     psrad     xmm0,16
                     movq      xmm4,qword[Preview.x]
                     pshufd    xmm5,dqword[buf+RECT.left],1100b
                     psubd     xmm0,xmm5
                     cvtdq2ps  xmm0,xmm0
                     divps     xmm0,xmm1
                     mulss     xmm1,xmm3
                     movss     xmm5,xmm1
                     maxss     xmm1,[MinScale]
                     minss     xmm1,[MaxScale]
                     movss     [Preview.scale],xmm1
                     divss     xmm1,xmm5
                     shufps    xmm1,xmm1,0
                     mulps     xmm3,xmm1
                     movaps    xmm2,xmm0
                     divps     xmm0,xmm3
                     subps     xmm2,xmm0
                     addsubps  xmm4,xmm2
                     movq      qword[Preview.x],xmm4
                     call      Rescale
                     stdcall   PreviewProc,0,WM_PAINT,0,0
                     ret
      .WM_INITDIALOG:mov    rbx,rcx
                     mov    [DialogWindow],rcx
                     invoke CreateDialogIndirectParamW,0,ControlPanelDlg,ebx,ControlPanelFunc,0
                     mov    [ControlPanel],rax
                     invoke GetDlgItem,rbx,0
                     mov    [PreviewWindow],rax

                     invoke  GetDC,rax
                     mov     [Preview.DC],rax
                     invoke  ChoosePixelFormat,rax,pfd
                     invoke  SetPixelFormat,[Preview.DC],rax,pfd
                     invoke  wglCreateContext,[Preview.DC]
                     mov     [RC],rax
                     invoke  wglMakeCurrent,[Preview.DC],rax

;Load OpenGL extensions
                     cmp word[GLExtFunctions],'gl'
                     jne .ExtensionsLoaded
                       mov    rdi,GLExtFunctions
                       @@:invoke wglGetProcAddress,rdi
                          test   rax,rax
                          jne .success
                            invoke wsprintfA,buf,errVideoNotSupported,rdi
                            mov    byte[buf+rax],0
                            invoke MessageBoxA,ebx,buf,0,0
                            jmp .WM_CLOSE
                          .success:
                          stosq
                          mov    ecx,-1
                          xor    eax,eax
                          repne  scasb
                          cmp    rdi,glActiveTexture
                       jna @b
                     .ExtensionsLoaded:

;Compile shaders
                     mov     [CalcArea],0
                     invoke  glCreateProgram
                     mov     [CalcArea],eax
                     stdcall CompileShader,eax,GL_COMPUTE_SHADER,CS_CalcArea,ShaderNames.CS_CalcArea
                     invoke  glCreateProgram
                     mov     [GetBounds],eax
                     stdcall CompileShader,eax,GL_VERTEX_SHADER,VertexShader,ShaderNames.VertexShader
                     stdcall CompileShader,[GetBounds],GL_GEOMETRY_SHADER,GS_GetBounds,ShaderNames.GS_GetBounds
                     stdcall CompileShader,[GetBounds],GL_FRAGMENT_SHADER,FS_GetBounds,ShaderNames.FS_GetBounds
                     invoke  glCreateProgram
                     mov     [Place],eax
                     stdcall CompileShader,eax,GL_VERTEX_SHADER,VS_Place,ShaderNames.VS_Place
                     stdcall CompileShader,[Place],GL_GEOMETRY_SHADER,GS_Place,ShaderNames.GS_Place
                     stdcall CompileShader,[Place],GL_FRAGMENT_SHADER,FS_Place,ShaderNames.FS_Place
                     invoke  glCreateProgram
                     mov     [Put],eax
                     stdcall CompileShader,eax,GL_VERTEX_SHADER,VertexShader,ShaderNames.VertexShader
                     stdcall CompileShader,[Put],GL_GEOMETRY_SHADER,GS_Put,ShaderNames.GS_Put
                     stdcall CompileShader,[Put],GL_FRAGMENT_SHADER,FS_Put,ShaderNames.FS_Put
                     invoke  glCreateProgram
                     mov     [Preview.Shader],eax
                     stdcall CompileShader,eax,GL_VERTEX_SHADER,VertexShader,ShaderNames.VertexShader
                     stdcall CompileShader,[Preview.Shader],GL_GEOMETRY_SHADER,GS_Preview,ShaderNames.GS_Preview
                     stdcall CompileShader,[Preview.Shader],GL_FRAGMENT_SHADER,FS_Preview,ShaderNames.FS_Preview
                     invoke  glPixelStorei,GL_PACK_ALIGNMENT,1
                     invoke  glBlendFunc,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA
                     invoke  glAlphaFunc,GL_GREATER,0
                     invoke  glEnable,GL_ALPHA_TEST
                     invoke  glGetString,GL_RENDERER
                     invoke  SendMessageA,[DialogWindow],WM_SETTEXT,0,rax

;Query CorelDraw interfaces
                     cominvk CorelApp,Set_Optimization,1
                     cominvk CorelApp,QueryInterface,IID_ICUIApplication,CorelCUIApp
                     cominvk CorelCUIApp,Get_DataContext,CorelDataContext
                     mov     [CustomizationDS],0
                     cominvk CorelDataContext,GetDataSource,strCustomizationDS,CustomizationDS
                     cominvk CorelDataContext,Release
                     cominvk CorelCUIApp,Release
                     mov     [UserData],0
                     cominvk CorelApp,Get_GlobalUserData,UserData
                     mov     [tmpVariant.type],0
                     cominvk UserData,Get_Item,strGPUNestParams,0,tmpVariant
                     cmp     [tmpVariant.type],VT_I8
                     jne @f
                       mov rax,[tmpVariant.data]
                       mov [GPUNestParams],rax
                    @@:

;Scaling preview according to selection sizes
                     invoke   GetClientRect,[DialogWindow],Preview.x
                     sub      [Preview.Width],190
                     cominvk  Selection,GetBoundingBox,SelectionRect.x,SelectionRect.y,SelectionRect.width,SelectionRect.height,1
                     cvtpd2ps xmm3,dqword[SelectionRect.x]
                     xorps    xmm3,dqword[flt_sign_mask]
                     cvtpi2pd xmm0,qword[Preview.Width]
                     movapd   xmm2,xmm0
                     divpd    xmm0,dqword[SelectionRect.width]
                     pshufd   xmm1,xmm0,01001110b

                     divpd    xmm2,xmm1
                     subpd    xmm2,dqword[SelectionRect.width]
                     mulpd    xmm2,dqword[dbl_05]
                     subpd    xmm2,dqword[SelectionRect.x]
                     cvtpd2ps xmm2,xmm2
                     movsd    qword[Preview.x],xmm2

                     comisd   xmm0,xmm1
                     jna @f
                       psrlq   xmm3,32
                       movss   [Preview.y],xmm3
                       cvtsd2ss xmm0,xmm1
                       jmp .nxt
                     @@:
                       movss    [Preview.x],xmm3
                       cvtsd2ss xmm0,xmm0
                     .nxt:
                     movss   [Preview.scale],xmm0

;Allocate memory
                     cominvk  Selection,Get_Count,NestNodeCount
                     mov      edi,[NestNodeCount]
                     imul     rbx,rdi,sizeof.TNestNode
                     lea      rdx,[rbx+rdi*8]
                     lea      rdx,[rdx+rdi*4]
                     invoke   VirtualAlloc,0,rdx,MEM_COMMIT,PAGE_READWRITE
                     mov      [NestNodes],rax
                     mov      rbp,rax
                     add      rax,rbx
                     mov      [Textures],rax
                     lea      rax,[rax+rdi*8]
                     mov      [NestingOrder],rax
                     mov      rsi,rax
                     invoke   glGenTextures,edi,[Textures]

;Load shapes from CorelDraw
                     movzx    eax,[GPUNestParams.MinDist]
                     invoke   CreatePen,PS_SOLID,eax,255
                     mov      [ContourPen],rax
                     mov      [bmiColors+1020],0FF0000h
                     mov      [tmpVariant.type],VT_I4
                     @@:invoke   CreateCompatibleDC,0
                        mov      [rsi],rbp
                        mov      [rbp+TNestNode.DC],rax
                        mov      rbx,rax
                        mov      [tmpVariant.data],rdi
                        cominvk  Selection,Get_Item,tmpVariant,addr rbp+TNestNode.Shape
                        comcall  [rbp+TNestNode.Shape],IVGShape,GetBoundingBox,addr rbp+TNestNode.x,addr rbp+TNestNode.y,addr rbp+TNestNode.Width,addr rbp+TNestNode.Height,1
                        movapd   xmm0,dqword[rbp+TNestNode.Width]
                        mulpd    xmm0,[dbl_05]
                        addpd    xmm0,dqword[rbp+TNestNode.x]
                        cvtpd2dq xmm0,xmm0
                        movq     qword[rbp+TNestNode.cx],xmm0
                        cvtpd2dq xmm0,dqword[rbp+TNestNode.Width]
                        movq     qword[rbp+TNestNode.TexWidth],xmm0
                        add      [rbp+TNestNode.TexWidth],MAX_DIST+3
                        add      [rbp+TNestNode.TexHeight],MAX_DIST+3
                        and      [rbp+TNestNode.TexWidth],-4
                        and      [rbp+TNestNode.TexHeight],-4
                        mov      rax,[Textures]
                        mov      eax,[rax+rdi*4-4]
                        mov      [rbp+TNestNode.texture],eax
                        mov      eax,[rbp+TNestNode.TexWidth]
                        mov      edx,[rbp+TNestNode.TexHeight]
                        mov      [bmi.biWidth],eax
                        imul     eax,edx
                        add      edx,edx
                        neg      edx
                        mov      [bmi.biHeight],edx
                        mov      [bmi.biSizeImage],eax
                        invoke   CreateDIBSection,rbx,bmi,0,addr rbp+TNestNode.Raster,0,0
                        mov      [rbp+TNestNode.BitmapHandle],rax
                        mov      edx,[bmi.biSizeImage]
                        invoke   SelectObject,rbx,[rbp+TNestNode.BitmapHandle]
                        invoke   BeginPath,rbx
                        stdcall  DrawShape,[rbp+TNestNode.Shape]
                        invoke   EndPath,rbx
                        invoke   SaveDC,rbx
                        invoke   FillPath,rbx
                        invoke   RestoreDC,rbx,-1
                        invoke   SaveDC,rbx
                        invoke   BitBlt,rbx,0,[rbp+TNestNode.TexHeight],[rbp+TNestNode.TexWidth],[rbp+TNestNode.TexHeight],rbx,0,0,SRCCOPY
                        invoke   SelectObject,rbx,[ContourPen]
                        invoke   StrokePath,rbx
                        invoke   glBindTexture,GL_TEXTURE_2D,[rbp+TNestNode.texture]
                        invoke   glTexParameteri,GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST
                        invoke   glTexParameteri,GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST
                        invoke   glTexParameteri,GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE
                        invoke   glTexParameteri,GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE
                        mov      eax,[bmi.biHeight]
                        neg      eax
                        invoke   glTexImage2D,GL_TEXTURE_2D,0,1,[rbp+TNestNode.TexWidth],rax,0,GL_RED,GL_UNSIGNED_BYTE,[rbp+TNestNode.Raster]
                        add      rbp,sizeof.TNestNode
                        add      rsi,8
                        dec      rdi
                     jne @b
                     cominvk CorelApp,Set_Optimization,0

;Sorting shapes by size
                     mov rbx,[NestingOrder]
                     mov ecx,[NestNodeCount]
                     .Sort:
                       mov   edx,ecx
                       mov   rax,[rbx+rcx*8-8]
                       movsd xmm0,[rax+TNestNode.Width]
                       addsd xmm0,[rax+TNestNode.Height]
                       .Sort2:
                          mov    rax,[rbx+rdx*8-8]
                          movsd  xmm1,[rax+TNestNode.Width]
                          addsd  xmm1,[rax+TNestNode.Height]
                          comisd xmm0,xmm1
                          ja @f
                            movsd xmm0,xmm1
                            mov   rax,[rbx+rcx*8-8]
                            xchg  rax,[rbx+rdx*8-8]
                            mov   [rbx+rcx*8-8],rax
                          @@:
                          dec    edx
                       jne .Sort2
                     loop .Sort

                     mov      eax,[NestNodeCount]
                     mov      rax,[rbx+rax*8-8]
                     movapd   xmm0,dqword[rax+TNestNode.Width]
                     dppd     xmm0,xmm0,110001b
                     sqrtsd   xmm0,xmm0
                     cvtsd2si eax,xmm0
                     add      eax,MAX_DIST+1
                     mov      [MaxDiameter],eax

;Update GUI
                     movzx   r8,[GPUNestParams.SheetWidth]
                     invoke  wsprintfW,buf,fmtInt2Str,r8
                     mov     [buf+rax*2],0
                     invoke  SendMessageW,[WidthEdit],WM_SETTEXT,0,buf
                     movzx   r8,[GPUNestParams.SheetHeight]
                     invoke  wsprintfW,buf,fmtInt2Str,r8
                     mov     [buf+rax*2],0
                     invoke  SendMessageW,[HeightEdit],WM_SETTEXT,0,buf
                     movzx   r8,[GPUNestParams.FitToBiggest]
                     invoke  SendMessageW,[FitToBiggestCheckBox],BM_SETCHECK,r8,0
                     stdcall ControlPanelFunc,0,WM_COMMAND,1,0
                     movzx   r9,[GPUNestParams.RotationsLevel]
                     invoke  SendMessageW,[RotationsTrackBar],TBM_SETPOS,1,r9
                     stdcall ControlPanelFunc,0,WM_HSCROLL,0,[RotationsTrackBar]
                     movzx   ebx,[GPUNestParams.MinDist]
                     shr     ebx,1
                     invoke  SendMessageW,[MinDistTrackBar],TBM_SETPOS,1,ebx
                     add     ebx,ebx
                     invoke  wsprintfW,buf,fmtInt2Str,ebx
                     mov     [buf+rax*2],0
                     invoke  SendMessageW,[MinDistValue],WM_SETTEXT,0,buf
                     movzx   eax,[GPUNestParams.NestingAlgo]
                     invoke  PostMessageW,[ByCoordsRadio+rax*8],BM_SETCHECK,1,0
                     invoke  SetWindowLongPtrW,[PreviewWindow],GWLP_WNDPROC,PreviewProc
                     stdcall DialogFunc,0,COREL_COLORCHANGE,0,0
            .WM_SIZE:invoke GetClientRect,[DialogWindow],buf
                     sub    dword[buf+RECT.right],190
                     movq   xmm0,qword[buf+RECT.right]
                     movq   qword[Preview.Width],xmm0
                     invoke MoveWindow,[ControlPanel],[Preview.Width],0,190,[Preview.Height],1
                     sub    dword[buf+RECT.bottom],34
                     invoke MoveWindow,[ApplyButton],15,dword[buf+RECT.bottom],100,25,1
                     invoke MoveWindow,[OkButton],125,dword[buf+RECT.bottom],50,25,1
                     invoke MoveWindow,[PreviewWindow],0,0,[Preview.Width],[Preview.Height],1
                     call   Rescale
                     invoke InvalidateRect,[DialogWindow],0,0
                     invoke InvalidateRect,[PreviewWindow],0,0
                     ret
   .WM_GETMINMAXINFO:mov [r9+MINMAXINFO.ptMinTrackSize.x],300
                     mov [r9+MINMAXINFO.ptMinTrackSize.y],400
                     ret
  .COREL_COLORCHANGE:invoke    SendMessageW,[CorelWndHandle],0B451h,133h,0
                     mov       [EditColor],eax
                     invoke    SendMessageW,[CorelWndHandle],0B451h,136h,0
                     mov       [BkColor],eax
                     test      eax,0FF000000h
                     jne @f
                       invoke GetSysColor,COLOR_WINDOW
                       mov    [EditColor],eax
                       invoke GetSysColor,COLOR_BTNFACE
                       mov    [BkColor],eax
                     @@:
                     and        [BkColor],0FFFFFFh
                     and        [EditColor],0FFFFFFh
                     or         al,ah
                     or         al,byte[BkColor+2]
                     mov        [TextColor],0C0C0C0h
                     cmp        al,128
                     sbb        edx,edx
                     and        [TextColor],edx

                     mov       dword[tmpVariant.type],0
                     cmp       [CustomizationDS],0
                     je @f
                       cominvk   CustomizationDS,GetProperty,strPageMatColor,tmpVariant
                     @@:
                     cmp       dword[tmpVariant.type],0
                     jne .ok
                       movzx   r8,byte[CorelVersion+1]
                       invoke  wsprintfW,buf,WindowSchemeRegPath,r8
                       mov     [buf+rax*2],0
                       invoke  RegOpenKeyExW,HKEY_CURRENT_USER,buf,0,KEY_QUERY_VALUE,tmpVariant.type
                       test    rax,rax
                       jne @f
                         mov     [tmpVariant.data],bufsize
                         invoke  RegQueryValueExW,[qword tmpVariant.type],strPageMatColor,0,0,buf,tmpVariant.data
                         test    rax,rax
                         jne @f
                           mov  dx,word[buf]
                           sub  dx,'0'
                           cmp  dx,'9'-'0'
                           seta al
                       @@:
                       mov     edx,222222h
                       movd    xmm0,[BkColor]
                       movd    xmm1,edx
                       paddusb xmm0,xmm1
                       movd    eax,xmm0
                       jne @f
                         call    BufStr2IntW
                       @@:
                       mov     dword[tmpVariant.data],eax
                       invoke  RegCloseKey,[tmpVariant.type]
                     .ok:
                     movd      xmm0,dword[tmpVariant.data]
                     punpcklbw xmm0,xmm0
                     punpcklwd xmm0,xmm0
                     psrld     xmm0,24
                     cvtdq2ps  xmm0,xmm0
                     mulps     xmm0,dqword[flt_rcp_255]
                     movaps    [ClearColor],xmm0

                     invoke     DeleteObject,[BkBrush]
                     invoke     DeleteObject,[EditBrush]
                     invoke     DeleteObject,[HighlightBrush]
                     invoke     DeleteObject,[HighlightPen]
                     invoke     DeleteObject,[BorderPen]
                     invoke     CreateSolidBrush,[BkColor]
                     mov        [BkBrush],rax
                     invoke     CreateSolidBrush,[EditColor]
                     mov        [EditBrush],rax
                     mov        eax,804000h
                     movd       xmm0,eax
                     paddusb    xmm0,dqword[BkColor]
                     movd       ecx,xmm0
                     invoke     CreateSolidBrush,rcx
                     mov        [HighlightBrush],rax
                     invoke     CreatePen,PS_SOLID,1,$FEAD00
                     mov        [HighlightPen],rax
                     invoke     CreatePen,PS_SOLID,1,[TextColor]
                     mov        [BorderPen],rax
                     ret
           .WM_CLOSE:mov    [StopNesting],1
                     @@:cmp [NestingThreadId],0
                     jne @b
                     invoke EndDialog,rcx,0
                     invoke SetWindowLongPtrW,[PreviewWindow],GWLP_WNDPROC,[DefWindowProcW] ;win64 sends WM_PAINT after WM_DESTROY
                     ret
         .WM_DESTROY:cmp     [CalcArea],0
                     je .skip
                      invoke DeleteObject,[BkBrush]
                      invoke DeleteObject,[BorderPen]
                      invoke DeleteObject,[EditBrush]
                      invoke DeleteObject,[ContourPen]
                      invoke DeleteObject,[HighlightPen]
                      invoke DeleteObject,[HighlightBrush]
                      invoke glDeleteProgram,[Put]
                      invoke glDeleteProgram,[Place]
                      invoke glDeleteProgram,[GetBounds]
                      invoke glDeleteProgram,[CalcArea]
                      invoke glDeleteProgram,[Preview.Shader]
                      cmp    [CustomizationDS],0
                      je @f
                        cominvk CustomizationDS,Release
                      @@:
                      cmp    [UserData],0
                      je .skip
                        call    GetParams
                        cominvk UserData,Set_Item,strGPUNestParams,0,GPUNestParamsVariant
                        cominvk UserData,Release
                        invoke  glDeleteTextures,[NestNodeCount],[Textures]
                        mov     rbx,[NestNodes]
                        mov     edi,[NestNodeCount]
                        @@:comcall [rbx+TNestNode.Shape],IVGShape,Release
                           invoke  DeleteObject,[rbx+TNestNode.BitmapHandle]
                           invoke  DeleteDC,[rbx+TNestNode.DC]
                           invoke  VirtualFree,[rbx+TNestNode.HeightMap.Data],0,MEM_RELEASE
                           add     rbx,sizeof.TNestNode
                           dec     edi
                        jne @b
                        invoke  VirtualFree,[NestNodes],0,MEM_RELEASE
                     .skip:
                     invoke wglMakeCurrent,0,0
                     invoke ReleaseDC,[PreviewWindow],[Preview.DC]
                     invoke wglDeleteContext,[RC]
                     ret
endp