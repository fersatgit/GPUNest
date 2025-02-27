ButtonProc: ;(wnd,msg,wParam,lParam: dword):dword;stdcall;
  cmp dword[esp+8],WM_PAINT
  je .WM_PAINT
  cmp dword[esp+8],WM_LBUTTONUP
  je .WM_LBUTTONUP
  cmp dword[esp+8],WM_MOUSEHOVER
  je .WM_MOUSEHOVER
  cmp dword[esp+8],WM_MOUSELEAVE
  je .WM_MOUSELEAVE
  cmp dword[esp+8],WM_MOUSEMOVE
  je .WM_MOUSEMOVE
  jmp [DefWindowProcW]
  .WM_MOUSEHOVER:mov dword[esp+12],1
  .WM_MOUSELEAVE:
       .WM_PAINT:push ebx
                 invoke GetDC,dword[esp+8]
                 mov    ebx,eax
                 sub    esp,16
                 invoke GetClientRect,dword[esp+28],esp
                 invoke SetBkMode,ebx,OPAQUE
                 cmp    dword[esp+32],1
                 push   [HighlightBrush]
                 push   [HighlightPen]
                 je @f
                   add    esp,8
                   push   [BkBrush]
                   push   [BorderPen]
                 @@:
                 invoke SelectObject,ebx
                 invoke SelectObject,ebx
                 invoke Rectangle,ebx
                 sub    esp,16
                 invoke SetTextColor,ebx,[TextColor]
                 invoke SetBkMode,ebx,TRANSPARENT
                 invoke SendMessageW,dword[esp+36],WM_GETTEXT,bufsize shr 1,buf
                 mov    eax,esp
                 invoke DrawTextW,ebx,buf,-1,eax,DT_CENTER+DT_VCENTER+DT_SINGLELINE
                 add    esp,16
                 invoke ValidateRect,dword[esp+12],0
                 invoke ReleaseDC,dword[esp+12],ebx
                 pop    ebx
                 ret 16
   .WM_LBUTTONUP:mov    edx,[esp+4]
                 xor    eax,eax
                 cmp    edx,[OkButton]
                 sete   al
                 add    eax,11
                 invoke SendMessageW,[ControlPanel],WM_COMMAND,eax,0
                 ret 16
   .WM_MOUSEMOVE:mov    eax,[esp+4]
                 mov    [EventTrack.hwndTrack],eax
                 invoke TrackMouseEvent,EventTrack
                 ret 16

EditProc: ;(wnd,msg,wParam,lParam: dword):dword;stdcall;
  cmp dword[esp+8],WM_NCPAINT
  je .WM_NCPAINT
  cmp dword[esp+8],WM_KILLFOCUS
  je .WM_KILLFOCUS
  cmp dword[esp+8],WM_PASTE
  jne .DEFAULT
    ret 16
    .WM_NCPAINT:push   ebx
                invoke GetDC,dword[esp+8]
                mov    ebx,eax
                invoke SelectObject,ebx,[BorderPen]
                invoke SelectObject,ebx,[EditBrush]
                invoke Rectangle,ebx,-1,-3,38,17
                invoke ReleaseDC,dword[esp+12],ebx
                pop ebx
                ret 16
  .WM_KILLFOCUS:invoke SendMessageW,dword[esp+16],WM_GETTEXT,bufsize,buf
                call   BufStr2IntW
                cmp    eax,10
                jae @f
                  invoke SendMessageW,dword[esp+16],WM_SETTEXT,0,strTen
                @@:
       .DEFAULT:pop  eax
                push [DefEditProc]
                push eax
                jmp  [CallWindowProcW]

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

PreviewProc: ;(wnd,msg,wParam,lParam: dword):dword;stdcall;
  cmp dword[esp+8],WM_PAINT
  je .WM_PAINT
  cmp dword[esp+8],WM_ERASEBKGND
  je .WM_ERASEBKGND
  cmp dword[esp+8],WM_MOUSEMOVE
  je .WM_MOUSEMOVE
  cmp dword[esp+8],WM_LBUTTONDOWN
  je .WM_LBUTTONDOWN
  cmp dword[esp+8],WM_LBUTTONUP
  je .WM_LBUTTONUP
  jmp [DefWindowProcW]
   .WM_LBUTTONDOWN:movsx  eax,word[esp+16]
                   movsx  edx,word[esp+18]
                   mov    [Mouse.x],eax
                   mov    [Mouse.y],edx
                   invoke SetCapture,dword[esp+4]
                   jmp .WM_PAINT
     .WM_LBUTTONUP:invoke ReleaseCapture
    .WM_ERASEBKGND:ret 16
     .WM_MOUSEMOVE:movd      xmm4,[esp+16]
                   punpcklwd xmm4,xmm4
                   psrad     xmm4,16
                   test      dword[esp+12],MK_LBUTTON
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
         .WM_PAINT:invoke wglMakeCurrent,dword[Preview.DC],[RC]
                   invoke glBindFramebuffer,GL_FRAMEBUFFER,0
                   invoke glViewport,0,0,[Preview.Width],[Preview.Height]
                   invoke glClearColor,[ClearColor.r],[ClearColor.g],[ClearColor.b],1.0
                   invoke glClear,GL_COLOR_BUFFER_BIT

;Draw shapes
                   invoke glEnable,GL_BLEND
                   invoke glLinkProgram,[Preview.Shader]
                   invoke glUseProgram,[Preview.Shader]
                   invoke glUniformMatrix4fv,1,1,GL_FALSE,Preview.matrix
                   invoke glActiveTexture,GL_TEXTURE0
                   pushad
                   sub    esp,16
                   mov    ebx,[NestNodeCount]
                   mov    ebp,[NestNodes]
                   @@:invoke   glBindTexture,GL_TEXTURE_2D,[ebp+TNestNode.texture]
                      cvtpi2ps xmm1,qword[Preview.Width]
                      cvtpi2ps xmm0,qword[ebp+TNestNode.TexWidth]
                      mulps    xmm1,[flt_05]
                      movlhps  xmm0,xmm1
                      movups   [esp],xmm0
                      invoke   glUniform4fv,0,1,esp
                      invoke   glBegin,GL_POINTS
                      cvtdq2ps xmm0,dqword[ebp+TNestNode.cx]
                      movups   [esp],xmm0
                      invoke   glVertex4fv,esp
                      invoke   glEnd
                      add      ebp,sizeof.TNestNode
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
                        invoke glColor4f,1.0,0,0,1.0
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
                   add   esp,16
                   popad

                   invoke glFinish
                   invoke SwapBuffers,dword[Preview.DC]
                   invoke ValidateRect,[PreviewWindow],0
                   ret 16

BufStr2IntW:
  mov edx,buf
  xor eax,eax
  jmp .start
      @@:movzx ecx,byte[edx]
         lea   eax,[eax*4+eax]
         lea   eax,[eax*2+ecx-'0']
         add   edx,2
  .start:cmp   word[edx],0
  jne @b
ret

GetParams:
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

ControlPanelFunc: ;(wnd,msg,wParam,lParam: dword):dword;stdcall;
  cmp dword[esp+8],WM_HSCROLL
  je .WM_HSCROLL
  cmp dword[esp+8],WM_NOTIFY
  je .WM_NOTIFY
  cmp dword[esp+8],WM_CTLCOLORSTATIC
  je .WM_CTLCOLORSTATIC
  cmp dword[esp+8],WM_CTLCOLORDLG
  je .WM_CTLCOLORDLG
  cmp dword[esp+8],WM_CTLCOLOREDIT
  je .WM_CTLCOLOREDIT
  cmp dword[esp+8],WM_COMMAND
  je .WM_COMMAND
  cmp dword[esp+8],WM_INITDIALOG
  je .WM_INITDIALOG
    xor eax,eax
  ret 16
         .WM_HSCROLL:mov eax,[esp+16]
                     cmp eax,[RotationsTrackBar]
                     jne @f
                       invoke  SendMessageW,eax,TBM_GETPOS,0,0
                       mov     [GPUNestParams.RotationsLevel],al
                       mov     ecx,eax
                       mov     eax,1
                       shl     eax,cl
                       cinvoke wsprintfW,buf,fmtInt2Str,eax
                       mov     [buf+eax*2],0
                       invoke  SendMessageW,[RotationsValue],WM_SETTEXT,0,buf
                       ret 16
                     @@:
                     pushad
                     invoke SendMessageW,eax,TBM_GETPOS,0,0
                     lea    ebx,[eax+eax]
                     movzx  edx,[GPUNestParams.MinDist]
                     mov    [GPUNestParams.MinDist],bl
                     mov    edi,ebx
                     sub    edi,edx
                     je @f
                       sar     edi,1
                       cinvoke wsprintfW,buf,fmtInt2Str,ebx
                       mov     [buf+eax*2],0
                       invoke  SendMessageW,[MinDistValue],WM_SETTEXT,0,buf
                       invoke  DeleteObject,[ContourPen]
                       invoke  CreatePen,PS_SOLID,ebx,255
                       mov     [ContourPen],eax

                       mov    esi,[NestNodeCount]
                       mov    ebp,[NestNodes]
                       .UpdateContour:
                         invoke RestoreDC,[ebp+TNestNode.DC],-1
                         invoke BitBlt,[ebp+TNestNode.DC],0,0,[ebp+TNestNode.TexWidth],[ebp+TNestNode.TexHeight],[ebp+TNestNode.DC],0,[ebp+TNestNode.TexHeight],SRCCOPY
                         invoke SaveDC,[ebp+TNestNode.DC]
                         test   ebx,ebx
                         je .skip
                           invoke SelectObject,[ebp+TNestNode.DC],[ContourPen]
                           invoke StrokePath,[ebp+TNestNode.DC]
                         .skip:
                         invoke glBindTexture,GL_TEXTURE_2D,[ebp+TNestNode.texture]
                         invoke glTexSubImage2D,GL_TEXTURE_2D,0,0,0,[ebp+TNestNode.TexWidth],[ebp+TNestNode.TexHeight],GL_RED,GL_UNSIGNED_BYTE,[ebp+TNestNode.Raster]
                         mov    ecx,MAX_ROTATIONS*32
                         .UpdateDistanceMap:
                           add [ebp+TNestNode.DistanceMap+ecx-12],edi
                           sub ecx,16
                         jne .UpdateDistanceMap
                         add    ebp,sizeof.TNestNode
                         dec    esi
                       jne .UpdateContour
                       invoke PostMessageW,[PreviewWindow],WM_PAINT,0,0
                       popad
                       mov    dword[esp+12],1
                       jmp .WM_COMMAND
                     @@:
                     popad
                     ret 16
         .WM_COMMAND:cmp dword[esp+12],1
                     je .FitToBiggestCheckBox
                     cmp dword[esp+12],11
                     je .ApplyButton
                     cmp dword[esp+12],12
                     je .OkButton
                     ret 16
                     .FitToBiggestCheckBox:push ebx
                                           invoke SendMessageW,[FitToBiggestCheckBox],BM_GETCHECK,0,0
                                           and    eax,BST_CHECKED
                                           xor    eax,BST_CHECKED
                                           mov    ebx,eax
                                           jne @f
                                             mov      eax,[NestNodeCount]
                                             shl      eax,2
                                             add      eax,[NestingOrder]
                                             mov      eax,[eax-4]
                                             movapd   xmm0,dqword[eax+TNestNode.Width]
                                             roundpd  xmm0,xmm0,2
                                             cvtpd2dq xmm0,xmm0
                                             sub      esp,8
                                             movq     [esp],xmm0
                                             movzx    eax,[GPUNestParams.MinDist]
                                             add      [esp],eax
                                             add      [esp+4],eax
                                             cinvoke  wsprintfW,buf,fmtInt2Str
                                             add      esp,4
                                             mov      [buf+eax*2],0
                                             invoke   SendMessageW,[WidthEdit],WM_SETTEXT,0,buf
                                             cinvoke  wsprintfW,buf,fmtInt2Str
                                             add      esp,4
                                             mov      [buf+eax*2],0
                                             invoke   SendMessageW,[HeightEdit],WM_SETTEXT,0,buf
                                           @@:
                                           invoke EnableWindow,[WidthEdit],ebx
                                           invoke EnableWindow,[HeightEdit],ebx
                                           pop    ebx
                                           ret 16
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
                                           ret 16
                                 .OkButton:pushad
                                           cominvk CorelApp,Set_Optimization,1
                                           mov     ebx,[NestNodeCount]
                                           mov     ebp,[NestNodes]
                                           @@:cvtpi2pd xmm0,qword[ebp+TNestNode.cx]
                                              addpd    xmm0,dqword[SelectionRect.x]
                                              sub      esp,16
                                              movupd   [esp],xmm0
                                              mov      esi,[ebp+TNestNode.Shape]
                                              comcall  esi,IVGShape,SetPositionEx,cdrCenter
                                              cvtsi2sd xmm0,[ebp+TNestNode.angle]
                                              mulsd    xmm0,[Angle2Degrees]
                                              sub      esp,8
                                              movsd    [esp],xmm0
                                              comcall  esi,IVGShape,Rotate
                                              add      ebp,sizeof.TNestNode
                                              dec      ebx
                                           jne @b
                                           cominvk CorelApp,Set_Optimization,0
                                           invoke  PostMessageW,[DialogWindow],WM_CLOSE,0,0
                                           popad
                                           ret 16
          .WM_NOTIFY:xchg ebx,[esp+16]
                     xor  eax,eax
                     cmp  [ebx+NMCUSTOMDRAW.hdr.idFrom],7
                     je @f
                     cmp  [ebx+NMCUSTOMDRAW.hdr.idFrom],8
                     je @f
                       mov ebx,[esp+16]
                       ret 16
                     @@:
                     cmp [ebx+NMCUSTOMDRAW.hdr.code],NM_CUSTOMDRAW
                     jne .quit
                       cmp [ebx+NMCUSTOMDRAW.dwDrawStage],CDDS_PREPAINT
                       mov eax,CDRF_NOTIFYITEMDRAW+256 ;CDRF_SKIPPOSTPAINT
                       je @f
                         invoke SelectObject,[ebx+NMCUSTOMDRAW.hdc],[BkBrush]
                         invoke SelectObject,[ebx+NMCUSTOMDRAW.hdc],[BorderPen]
                         invoke Rectangle,[ebx+NMCUSTOMDRAW.hdc],[ebx+NMCUSTOMDRAW.rc.left],[ebx+NMCUSTOMDRAW.rc.top],[ebx+NMCUSTOMDRAW.rc.right],[ebx+NMCUSTOMDRAW.rc.bottom]
                         invoke ReleaseDC,[ebx+NMCUSTOMDRAW.hdr.hwndFrom],[ebx+NMCUSTOMDRAW.hdc]
                         mov    eax,CDRF_SKIPDEFAULT
                       @@:
                       invoke SetWindowLongW,dword[esp+12],DWL_MSGRESULT,eax
                       mov    eax,1
                     .quit:
                     mov    ebx,[esp+16]
                     ret 16
  .WM_CTLCOLORSTATIC:
     .WM_CTLCOLORDLG:invoke SetBkColor,dword[esp+16],[BkColor]
                     invoke SetTextColor,dword[esp+16],[TextColor]
                     mov    eax,[BkBrush]
                     ret 16
    .WM_CTLCOLOREDIT:invoke SetBkColor,dword[esp+16],[EditColor]
                     invoke SetTextColor,dword[esp+16],[TextColor]
                     mov    eax,[EditBrush]
                     ret 16
      .WM_INITDIALOG:push ebx
                     mov  ebx,12
                     @@:invoke GetDlgItem,dword[esp+12],ebx
                        mov    [wnds+ebx*4],eax
                        dec    ebx
                     jne @b
                     invoke SendMessageW,[RotationsTrackBar],TBM_SETRANGE,0,MAX_ROTATIONS_LEVEL shl 16
                     invoke SendMessageW,[MinDistTrackBar],TBM_SETRANGE,0,(MAX_DIST shr 1) shl 16
                     invoke PostMessageW,[WidthEdit],EM_LIMITTEXT,4,0
                     invoke PostMessageW,[HeightEdit],EM_LIMITTEXT,4,0
                     invoke SetWindowLongW,[WidthEdit],GWL_WNDPROC,EditProc
                     mov    [DefEditProc],eax
                     invoke SetWindowLongW,[HeightEdit],GWL_WNDPROC,EditProc
                     invoke SetWindowLongW,[ApplyButton],GWL_WNDPROC,ButtonProc
                     invoke SetWindowLongW,[OkButton],GWL_WNDPROC,ButtonProc
                     mov    eax,1
                     pop    ebx
                     ret 16

CompileShader: ;(_program,ShaderType: dword; ShaderCode,ShaderName: PAnsiChar): dword;
  invoke glCreateShader,dword[esp+8]
  mov    ebx,eax
  lea    eax,[esp+12]
  invoke glShaderSource,ebx,1,eax,0
  invoke glCompileShader,ebx
  invoke glGetShaderiv,ebx,GL_COMPILE_STATUS,esp,0
  pop    eax
  test   eax,eax
  jne @f
    invoke  glGetShaderInfoLog,ebx,bufsize,BkColor,buf
    cinvoke wsprintfA,buf+1024,errCompileShader,dword[esp+16]
    mov     byte[buf+1024+eax],0
    invoke  MessageBoxA,0,buf,buf+1024,0
    add     esp,20
    popad
    jmp     DialogFunc.WM_CLOSE
  @@:
  invoke glAttachShader,dword[esp+8],ebx
  invoke glDeleteShader,ebx
ret 16

;ebp - pointer to TNestNode
;ebx - DC
PointTypes db PT_MOVETO,PT_LINETO,PT_BEZIERTO,PT_BEZIERTO
DrawShape: ;(Shape: IVGShape);stdcall;
  sub      esp,16
  comcall  [esp+24],IVGShape,Get_type,esp
  mov      eax,[esp]
  cmp      eax,cdrGroupShape
  ja      .cdrRectangleShape
    jmp     [.case+eax*4]
    .case dd .cdrRectangleShape,.cdrRectangleShape,.cdrEllipseShape,.cdrCurveShape,.cdrRectangleShape,.cdrBitmapShape,.cdrRectangleShape,.cdrGroupShape
    .cdrRectangleShape:mov      esi,[Rectangle]
                       jmp @f
      .cdrEllipseShape:mov      esi,[Ellipse]
                    @@:comcall  [esp+40],IVGShape,GetBoundingBox,buf,buf+8,buf+16,buf+24,0
                       cvtpi2pd xmm0,qword[ebp+TNestNode.TexWidth]
                       movapd   xmm1,dqword[buf]
                       subpd    xmm0,dqword[ebp+TNestNode.Width]
                       subpd    xmm1,dqword[ebp+TNestNode.x]
                       mulpd    xmm0,[dbl_05]
                       addpd    xmm0,xmm1
                       cvtpd2dq xmm1,xmm0
                       addpd    xmm0,dqword[buf+16]
                       cvtpd2dq xmm0,xmm0
                       movq     [esp],xmm1
                       movq     [esp+8],xmm0
                       stdcall  esi,ebx
                       ret 4
        .cdrCurveShape:comcall [esp+24],IVGShape,Get_Curve,Curve
            .drawCurve:cominvk Curve,GetCurveInfo,CurveInfo
                       cvtpi2pd xmm0,qword[ebp+TNestNode.TexWidth]
                       movapd   xmm1,dqword[ebp+TNestNode.x]
                       subpd    xmm0,dqword[ebp+TNestNode.Width]
                       mulpd    xmm0,[dbl_05]
                       subpd    xmm1,xmm0
                       mov      edx,[CurveInfo]
                       mov      ecx,[edx+SAFEARRAY.rgsabound+SAFEARRAYBOUND.cElements]
                       push     ecx
                       mov      edx,[edx+SAFEARRAY.pvData]
                       lea      esi,[edx+ecx*sizeof.POINT]
                       mov      dword[buf+16],edx
                       neg      ecx
                       @@:movupd   xmm0,dqword[edx+CurveElement.PositionX]
                          subpd    xmm0,xmm1
                          cvtpd2dq xmm0,xmm0
                          movq     [esi+ecx*sizeof.POINT],xmm0
                          mov      eax,[edx+CurveElement.ElementType]
                          mov      al,byte[PointTypes+eax]
                          mov      [esp+ecx],al
                          add      edx,sizeof.CurveElement
                          inc      ecx
                       jne @b
                       mov     esi,[esp]
                       sub     esp,esi
                       mov     eax,esp
                       invoke  PolyDraw,ebx,dword[buf+16],eax,esi
                       invoke  SafeArrayDestroy,[CurveInfo]
                       cominvk Curve,Release
                       lea     esp,[esp+esi+20]
                       ret 4
       .cdrBitmapShape:comcall [esp+24],IVGShape,Get_Bitmap,Bitmap
                       cominvk Bitmap,Get_CropEnvelope,Curve
                       cominvk Bitmap,Release
                       jmp .drawCurve
        .cdrGroupShape:comcall [esp+24],IVGShape,Get_Shapes,esp
                       lea     eax,[esp+4]
                       comcall [esp+4],IVGShapeRange,Get_Count,eax
                       @@:lea     eax,[esp+8]
                          comcall [esp+20],IVGShapeRange,Get_Item,VT_I4,0,dword[esp+12],0,eax
                          stdcall DrawShape,dword[esp+8]
                          comcall [esp+8],IVGShape,Release
                          dec     dword[esp+4]
                       jne @b
                       comcall [esp],IVGShapeRange,Release
                       add     esp,16
ret 4

DialogFunc: ;(wnd,msg,wParam,lParam: dword):dword;stdcall;
  cmp dword[esp+8],WM_MOUSEWHEEL
  je .WM_MOUSEWHEEL
  cmp dword[esp+8],WM_SIZE
  je .WM_SIZE
  cmp dword[esp+8],WM_GETMINMAXINFO
  je .WM_GETMINMAXINFO
  cmp dword[esp+8],COREL_COLORCHANGE
  je .COREL_COLORCHANGE
  cmp dword[esp+8],WM_INITDIALOG
  je .WM_INITDIALOG
  cmp dword[esp+8],WM_CLOSE
  je .WM_CLOSE
  cmp dword[esp+8],WM_DESTROY
  je .WM_DESTROY
    xor eax,eax
  ret 16
      .WM_MOUSEWHEEL:invoke    GetWindowRect,[PreviewWindow],buf
                     movsx     eax,word[esp+14]
                     cvtsi2ss  xmm3,eax
                     psubd     xmm3,dqword[flt_exp_9]
                     addss     xmm3,[flt_1]
                     movd      xmm0,[esp+16]
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
                     jmp       PreviewProc.WM_PAINT
      .WM_INITDIALOG:pushad
                     mov    ebx,[esp+36]
                     mov    [DialogWindow],ebx
                     invoke CreateDialogIndirectParamW,0,ControlPanelDlg,ebx,ControlPanelFunc,0
                     mov    [ControlPanel],eax
                     invoke GetDlgItem,ebx,0
                     mov    [PreviewWindow],eax

                     invoke  GetDC,eax
                     mov     dword[Preview.DC],eax
                     invoke  ChoosePixelFormat,eax,pfd
                     invoke  SetPixelFormat,dword[Preview.DC],eax,pfd
                     invoke  wglCreateContext,dword[Preview.DC]
                     mov     [RC],eax
                     invoke  wglMakeCurrent,dword[Preview.DC],eax

;Load OpenGL extensions
                     cmp word[GLExtFunctions],'gl'
                     jne .ExtensionsLoaded
                       mov    edi,GLExtFunctions
                       @@:invoke wglGetProcAddress,edi
                          test   eax,eax
                          jne .success
                            cinvoke wsprintfA,buf,errVideoNotSupported,edi
                            mov     byte[buf+eax],0
                            invoke  MessageBoxA,dword[esp+48],buf,0,0
                            popad
                            jmp .WM_CLOSE
                          .success:
                          stosd
                          mov    ecx,-1
                          xor    eax,eax
                          repne  scasb
                          cmp    edi,glActiveTexture
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
                     invoke  SendMessageA,[DialogWindow],WM_SETTEXT,0,eax

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
                     mov     dword[tmpVariant.type],0
                     cominvk UserData,Get_Item,strGPUNestParams,0,tmpVariant
                     cmp     dword[tmpVariant.type],VT_I8
                     jne @f
                       movq xmm0,[tmpVariant.data]
                       movq [GPUNestParams],xmm0
                     @@:

;Scaling preview according to selection sizes
                     invoke   GetClientRect,dword[DialogWindow],Preview.x
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
                     imul     ebx,edi,sizeof.TNestNode
                     lea      eax,[ebx+edi*8]
                     invoke   VirtualAlloc,0,eax,MEM_COMMIT,PAGE_READWRITE
                     mov      [NestNodes],eax
                     mov      ebp,eax
                     add      eax,ebx
                     mov      [Textures],eax
                     lea      eax,[eax+edi*4]
                     mov      [NestingOrder],eax
                     mov      esi,eax
                     invoke   glGenTextures,edi,[Textures]

;Load shapes from CorelDraw
                     movzx    eax,[GPUNestParams.MinDist]
                     invoke   CreatePen,PS_SOLID,eax,255
                     mov      [ContourPen],eax
                     mov      [bmiColors+1020],0FF0000h
                     @@:invoke   CreateCompatibleDC,0
                        mov      [esi],ebp
                        mov      [ebp+TNestNode.DC],eax
                        mov      ebx,eax
                        lea      eax,[ebp+TNestNode.Shape]
                        cominvk  Selection,Get_Item,VT_I4,0,edi,0,eax
                        push     1
                        lea      eax,[ebp+TNestNode.Height]
                        push     eax
                        lea      eax,[ebp+TNestNode.Width]
                        push     eax
                        lea      eax,[ebp+TNestNode.y]
                        push     eax
                        lea      eax,[ebp+TNestNode.x]
                        push     eax
                        comcall  [ebp+TNestNode.Shape],IVGShape,GetBoundingBox
                        movapd   xmm0,dqword[ebp+TNestNode.Width]
                        mulpd    xmm0,[dbl_05]
                        addpd    xmm0,dqword[ebp+TNestNode.x]
                        cvtpd2dq xmm0,xmm0
                        movq     qword[ebp+TNestNode.cx],xmm0
                        cvtpd2dq xmm0,dqword[ebp+TNestNode.Width]
                        movq     qword[ebp+TNestNode.TexWidth],xmm0
                        add      [ebp+TNestNode.TexWidth],MAX_DIST+3
                        add      [ebp+TNestNode.TexHeight],MAX_DIST+3
                        and      [ebp+TNestNode.TexWidth],-4
                        and      [ebp+TNestNode.TexHeight],-4
                        mov      eax,[Textures]
                        mov      eax,[eax+edi*4-4]
                        mov      [ebp+TNestNode.texture],eax
                        mov      eax,[ebp+TNestNode.TexWidth]
                        mov      edx,[ebp+TNestNode.TexHeight]
                        mov      [bmi.biWidth],eax
                        imul     eax,edx
                        add      edx,edx
                        neg      edx
                        mov      [bmi.biHeight],edx
                        mov      [bmi.biSizeImage],eax
                        lea      eax,[ebp+TNestNode.Raster]
                        invoke   CreateDIBSection,ebx,bmi,0,eax,0,0
                        mov      [ebp+TNestNode.BitmapHandle],eax
                        invoke   SelectObject,ebx,[ebp+TNestNode.BitmapHandle]
                        invoke   BeginPath,ebx
                        push     esi
                        stdcall  DrawShape,[ebp+TNestNode.Shape]
                        pop      esi
                        invoke   EndPath,ebx
                        invoke   SaveDC,ebx
                        invoke   FillPath,ebx
                        invoke   RestoreDC,ebx,-1
                        invoke   SaveDC,ebx
                        invoke   BitBlt,ebx,0,[ebp+TNestNode.TexHeight],[ebp+TNestNode.TexWidth],[ebp+TNestNode.TexHeight],ebx,0,0,SRCCOPY
                        invoke   SelectObject,ebx,[ContourPen]
                        invoke   StrokePath,ebx
                        invoke   glBindTexture,GL_TEXTURE_2D,[ebp+TNestNode.texture]
                        invoke   glTexParameteri,GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST
                        invoke   glTexParameteri,GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST
                        invoke   glTexParameteri,GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE
                        invoke   glTexParameteri,GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE
                        mov      eax,[bmi.biHeight]
                        neg      eax
                        invoke   glTexImage2D,GL_TEXTURE_2D,0,1,[ebp+TNestNode.TexWidth],eax,0,GL_RED,GL_UNSIGNED_BYTE,[ebp+TNestNode.Raster]
                        add      ebp,sizeof.TNestNode
                        add      esi,4
                        dec      edi
                     jne @b
                     cominvk CorelApp,Set_Optimization,0

;Sorting shapes by size
                     mov ebx,[NestingOrder]
                     mov ecx,[NestNodeCount]
                     .Sort:
                       mov   edx,ecx
                       mov   eax,[ebx+ecx*4-4]
                       movsd xmm0,[eax+TNestNode.Width]
                       addsd xmm0,[eax+TNestNode.Height]
                       .Sort2:
                          mov    eax,[ebx+edx*4-4]
                          movsd  xmm1,[eax+TNestNode.Width]
                          addsd  xmm1,[eax+TNestNode.Height]
                          comisd xmm0,xmm1
                          ja @f
                            movsd xmm0,xmm1
                            mov   eax,[ebx+ecx*4-4]
                            xchg  eax,[ebx+edx*4-4]
                            mov   [ebx+ecx*4-4],eax
                          @@:
                          dec    edx
                       jne .Sort2
                     loop .Sort

                     mov      eax,[NestNodeCount]
                     mov      eax,[ebx+eax*4-4]
                     movapd   xmm0,dqword[eax+TNestNode.Width]
                     dppd     xmm0,xmm0,110001b
                     sqrtsd   xmm0,xmm0
                     cvtsd2si eax,xmm0
                     add      eax,MAX_DIST+1
                     mov      [MaxDiameter],eax

;Update GUI
                     movzx   eax,[GPUNestParams.SheetWidth]
                     cinvoke wsprintfW,buf,fmtInt2Str,eax
                     mov     [buf+eax*2],0
                     invoke  SendMessageW,[WidthEdit],WM_SETTEXT,0,buf
                     movzx   eax,[GPUNestParams.SheetHeight]
                     cinvoke wsprintfW,buf,fmtInt2Str,eax
                     mov     [buf+eax*2],0
                     invoke  SendMessageW,[HeightEdit],WM_SETTEXT,0,buf
                     movzx   eax,[GPUNestParams.FitToBiggest]
                     invoke  SendMessageW,[FitToBiggestCheckBox],BM_SETCHECK,eax,0
                     stdcall ControlPanelFunc,0,WM_COMMAND,1,0
                     movzx   eax,[GPUNestParams.RotationsLevel]
                     invoke  SendMessageW,[RotationsTrackBar],TBM_SETPOS,1,eax
                     stdcall ControlPanelFunc,0,WM_HSCROLL,0,[RotationsTrackBar]
                     movzx   ebx,[GPUNestParams.MinDist]
                     shr     ebx,1
                     invoke  SendMessageW,[MinDistTrackBar],TBM_SETPOS,1,ebx
                     add     ebx,ebx
                     cinvoke wsprintfW,buf,fmtInt2Str,ebx
                     mov     [buf+eax*2],0
                     invoke  SendMessageW,[MinDistValue],WM_SETTEXT,0,buf
                     movzx   eax,[GPUNestParams.NestingAlgo]
                     invoke  PostMessageW,[ByCoordsRadio+eax*4],BM_SETCHECK,1,0
                     invoke  SetWindowLongW,[PreviewWindow],GWL_WNDPROC,PreviewProc
                     stdcall DialogFunc,0,COREL_COLORCHANGE,0,0
                     popad
            .WM_SIZE:invoke GetClientRect,dword[esp+8],buf
                     sub    dword[buf+RECT.right],190
                     movq   xmm0,qword[buf+RECT.right]
                     movq   qword[Preview.Width],xmm0
                     invoke MoveWindow,[ControlPanel],[Preview.Width],0,190,[Preview.Height],1
                     sub    dword[buf+RECT.bottom],34
                     invoke MoveWindow,[ApplyButton],15,dword[buf+RECT.bottom],100,25,1
                     invoke MoveWindow,[OkButton],125,dword[buf+RECT.bottom],50,25,1
                     invoke MoveWindow,[PreviewWindow],0,0,[Preview.Width],[Preview.Height],1
                     call   Rescale
                     invoke InvalidateRect,dword[esp+12],0,0
                     invoke InvalidateRect,[PreviewWindow],0,0
                     ret 16
   .WM_GETMINMAXINFO:mov eax,[esp+16]
                     mov [eax+MINMAXINFO.ptMinTrackSize.x],300
                     mov [eax+MINMAXINFO.ptMinTrackSize.y],400
                     ret 16
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
                       movzx   eax,byte[CorelVersion+1]
                       cinvoke wsprintfW,buf,WindowSchemeRegPath,eax
                       mov     [buf+eax*2],0
                       invoke  RegOpenKeyExW,HKEY_CURRENT_USER,buf,0,KEY_QUERY_VALUE,tmpVariant.type
                       test    eax,eax
                       jne @f
                         mov     dword[tmpVariant.data],bufsize
                         invoke  RegQueryValueExW,dword[tmpVariant.type],strPageMatColor,0,0,buf,tmpVariant.data
                         test    eax,eax
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
                       invoke  RegCloseKey,dword[tmpVariant.type]
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
                     mov        [BkBrush],eax
                     invoke     CreateSolidBrush,[EditColor]
                     mov        [EditBrush],eax
                     mov        eax,804000h
                     movd       xmm0,eax
                     paddusb    xmm0,dqword[BkColor]
                     movd       eax,xmm0
                     invoke     CreateSolidBrush,eax
                     mov        [HighlightBrush],eax
                     invoke     CreatePen,PS_SOLID,1,$FEAD00
                     mov        [HighlightPen],eax
                     invoke     CreatePen,PS_SOLID,1,[TextColor]
                     mov        [BorderPen],eax
                     ret 16
           .WM_CLOSE:mov    [StopNesting],1
                     @@:cmp [NestingThreadId],0
                     jne @b
                     invoke EndDialog,dword[esp+8],0
                     ret 16
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
                        cominvk UserData,Set_Item,strGPUNestParams,0,VT_I8,0,dword[GPUNestParams],dword[GPUNestParams+4]
                        cominvk UserData,Release
                        invoke  glDeleteTextures,[NestNodeCount],[Textures]
                        pushad
                        mov     ebx,[NestNodes]
                        mov     edi,[NestNodeCount]
                        @@:comcall [ebx+TNestNode.Shape],IVGShape,Release
                           invoke  DeleteObject,[ebx+TNestNode.BitmapHandle]
                           invoke  DeleteDC,[ebx+TNestNode.DC]
                           invoke  VirtualFree,[ebx+TNestNode.HeightMap],0,MEM_RELEASE
                           add     ebx,sizeof.TNestNode
                           dec     edi
                        jne @b
                        popad
                        invoke  VirtualFree,[NestNodes],0,MEM_RELEASE
                     .skip:
                     invoke wglMakeCurrent,0,0
                     invoke ReleaseDC,[PreviewWindow],dword[Preview.DC]
                     invoke wglDeleteContext,[RC]
                     ret 16