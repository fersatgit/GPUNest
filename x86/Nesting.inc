Nesting:
  pushad
  sub esp,16
  invoke wglMakeCurrent,dword[Preview.DC],[RC]
  movzx  eax,[GPUNestParams.SheetWidth]
  movzx  edx,[GPUNestParams.SheetHeight]
  mul    edx
  mov    [FrameBufferLen],eax
  mov    edi,eax
  mov    cl,[GPUNestParams.RotationsLevel]
  add    cl,2
  shl    eax,cl
  mov    [PlacementMapLen],eax
  mov    ebx,errRAM
  shl    edi,2
  invoke VirtualAlloc,0,edi,MEM_COMMIT,PAGE_READWRITE
  test   eax,eax
  je .error
  mov    [FrameBuffer],eax
  add    edi,edi
  invoke VirtualAlloc,0,edi,MEM_COMMIT,PAGE_READWRITE
  test   eax,eax
  je .error
  mov    [NFP],eax
  mov    edx,dword[GPUNestParams.SheetWidth]
  add    edx,00010001h
  mov    [eax],edx       ;First element of NFP used for TDR detection
  invoke VirtualAlloc,0,[PlacementMapLen],MEM_COMMIT,PAGE_READWRITE
  test   eax,eax
  je .error
  mov    [PlacementMap],eax

  invoke glGenVertexArrays,1,VAO
  invoke glBindVertexArray,[VAO]
  invoke glGenBuffers,1,VBO
  invoke glBindBuffer,GL_ARRAY_BUFFER,[VBO]
  invoke glVertexAttribPointer,0,4,GL_UNSIGNED_SHORT,GL_FALSE,8,0
  invoke glEnableVertexAttribArray,0

  mov    ebx,errVideoRAM
  invoke glGenBuffers,3,SSBO
  invoke glBindBuffer,GL_SHADER_STORAGE_BUFFER,[SSBO]
  invoke glBindBufferBase,GL_SHADER_STORAGE_BUFFER,0,[SSBO]
  mov    eax,[MaxDiameter]
  shl    eax,MAX_ROTATIONS_LEVEL+2 ;size of HeightMap
  add    eax,sizeof.DistanceMap
  invoke glBufferData,GL_SHADER_STORAGE_BUFFER,eax,0,GL_STREAM_DRAW
  invoke glGetError
  test   eax,eax
  jne .error
  invoke glBindBuffer,GL_SHADER_STORAGE_BUFFER,[SSBO+4]
  invoke glBindBufferBase,GL_SHADER_STORAGE_BUFFER,1,[SSBO+4]
  invoke glBufferData,GL_SHADER_STORAGE_BUFFER,[PlacementMapLen],0,GL_STREAM_DRAW
  invoke glGetError
  test   eax,eax
  jne .error
  invoke glBindBuffer,GL_SHADER_STORAGE_BUFFER,[SSBO+8]
  invoke glBindBufferBase,GL_SHADER_STORAGE_BUFFER,2,[SSBO+8]
  add    edi,sizeof.TNestingParams+sizeof.TSheet.HeightMap
  invoke glBufferData,GL_SHADER_STORAGE_BUFFER,edi,0,GL_STREAM_DRAW
  invoke glGetError
  test   eax,eax
  jne .error
  invoke glBindBuffer,GL_SHADER_STORAGE_BUFFER,0

;Calculate distance and height maps
  invoke   glGenFramebuffers,1,FBO
  invoke   glBindFramebuffer,GL_FRAMEBUFFER,[FBO]
  invoke   glGenTextures,1,Texture
  invoke   glBindTexture,GL_TEXTURE_RECTANGLE,[Texture]
  invoke   glTexParameteri,GL_TEXTURE_RECTANGLE,GL_TEXTURE_MAG_FILTER,GL_NEAREST
  invoke   glTexParameteri,GL_TEXTURE_RECTANGLE,GL_TEXTURE_MIN_FILTER,GL_NEAREST
  invoke   glTexImage2D,GL_TEXTURE_RECTANGLE,0,GL_RGBA,[MaxDiameter],[MaxDiameter],0,GL_RGBA,GL_UNSIGNED_BYTE,0
  invoke   glFramebufferTexture2D,GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_RECTANGLE,[Texture],0

  invoke   glLinkProgram,[GetBounds]
  invoke   glUseProgram,[GetBounds]
  invoke   glViewport,0,0,[MaxDiameter],[MaxDiameter]
  invoke   glUniform1i,0,[MaxDiameter]

  mov      eax,[MaxDiameter]
  shl      eax,MAX_ROTATIONS_LEVEL+2
  mov      [tmpHeightMapSize],eax
  invoke   VirtualAlloc,0,eax,MEM_COMMIT,PAGE_READWRITE
  mov      [tmpHeightMap],eax

  cvtsi2ss xmm0,[MaxDiameter]
  mulss    xmm0,[flt_05]
  subss    xmm0,[flt_1]
  roundss  xmm0,xmm0,2 ;roundup(MaxDiameter*0.5-1)
  shufps   xmm0,xmm0,0
  cvtps2dq xmm0,xmm0
  movdqa   dqword[delta],xmm0

  mov      esi,[NestNodeCount]
  mov      ebp,[NestNodes]
  .GetBounds:
     movapd   xmm0,dqword[ebp+TNestNode.Width]
     dppd     xmm0,xmm0,110001b
     sqrtsd   xmm0,xmm0
     cvtsd2si eax,xmm0
     add      eax,MAX_DIST+1
     mov      [ebp+TNestNode.HeightMap.Width],eax
     shl      eax,MAX_ROTATIONS_LEVEL+2
     mov      [HeightMapSize],eax
     invoke   VirtualAlloc,0,eax,MEM_COMMIT,PAGE_READWRITE
     mov      [ebp+TNestNode.HeightMap.Data],eax
     invoke   glBindTexture,GL_TEXTURE_2D,[ebp+TNestNode.texture]
     mov      ecx,[tmpHeightMapSize]
     shr      ecx,2
     mov      edi,[tmpHeightMap]
     xor      eax,eax
     rep      stosd
     lea      eax,[ebp+TNestNode.DistanceMap]
     invoke   glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,eax
     invoke   glNamedBufferSubData,[SSBO],sizeof.DistanceMap,[tmpHeightMapSize],[tmpHeightMap]
     invoke   glBegin,GL_POINTS
     cvtpd2ps xmm0,dqword[ebp+TNestNode.Width]
     movzx    eax,[GPUNestParams.MinDist]
     shufps   xmm0,xmm0,01000100b
     cvtsi2ss xmm1,eax
     cvtsi2ss xmm2,[MaxDiameter]
     cvtpi2ps xmm3,qword[ebp+TNestNode.TexWidth]
     shufps   xmm1,xmm1,0
     shufps   xmm2,xmm3,01000000b
     addps    xmm0,xmm1
     divps    xmm0,xmm2
     psubd    xmm0,dqword[flt_exp_0_0_1_2] ;(Width+GPUNestParams.MinDist)/MaxDiameter,(Height+GPUNestParams.MinDist)/MaxDiameter,(Width+GPUNestParams.MinDist)/TexWidth*0.5,(Height+GPUNestParams.MinDist)/TexHeight*0.25
     movdqu   [esp],xmm0
     invoke   glVertex4fv,esp
     invoke   glEnd
     invoke   glFinish
     lea      eax,[ebp+TNestNode.DistanceMap]
     invoke   glGetNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,eax
     invoke   glGetNamedBufferSubData,[SSBO],sizeof.DistanceMap,[tmpHeightMapSize],[tmpHeightMap]

     mov      ecx,MAX_ROTATIONS*16-16
     movdqa   xmm1,dqword[delta]
     mov      edx,[tmpHeightMap]
     .RecalcDistanceMap:
       movdqa    xmm0,dqword[ebp+TNestNode.DistanceMap+ecx]
       pshufd    xmm0,xmm0,1000b
       punpcklwd xmm0,xmm0
       psrad     xmm0,16
       psubd     xmm0,xmm1
       cmp       word[ebp+TNestNode.DistanceMap+ecx+2],0
       ja @f
         mov  eax,[MaxDiameter]
         pxor xmm0,xmm0
         imul eax,ecx
         shr  eax,4
         mov  dword[edx+eax*4],1
       @@:
       movdqa dqword[ebp+TNestNode.DistanceMap+ecx],xmm0
       movdqa dqword[ebp+TNestNode.DistanceMap+ecx+MAX_ROTATIONS*16],xmm0
       sub    ecx,16
     jnl .RecalcDistanceMap

     mov      edx,(MAX_ROTATIONS-1)*4
     mov      ebx,[ebp+TNestNode.HeightMap.Data]
     add      ebx,[HeightMapSize]
     .RecalcHeightMap:
       mov  ecx,[MaxDiameter]
       mov  edi,ecx
       imul edi,edx
       add  edi,[tmpHeightMap]
       xor  eax,eax
       repe scasd
       mov  eax,[ebp+TNestNode.HeightMap.Width]
       shl  eax,2
       sub  ebx,eax
       xor  ecx,ecx
       @@:mov eax,[edi+ecx*4-4]
          sub eax,[delta]
          add eax,[ebp+TNestNode.DistanceMap+DIST_BOTTOM*16+edx*4+4]
          mov [ebx+ecx*4+4],eax
          inc ecx
          cmp dword[edi+ecx*4-4],0
       jne @b
       mov  [ebx],ecx
       sub  edx,4
     jnl .RecalcHeightMap
     add      ebp,sizeof.TNestNode
     dec      esi
  jne .GetBounds
  invoke VirtualFree,[tmpHeightMap],0,MEM_RELEASE
  invoke glDeleteFramebuffers,1,FBO
  invoke glDeleteTextures,1,Texture

;Reset shapes position
  mov edx,[NestNodeCount]
  mov ebp,[NestNodes]
  @@:mov eax,[ebp+TNestNode.DistanceMap+DIST_BOTTOM*16+4]
     add ax,[GPUNestParams.SheetHeight]
     add eax,MAX_DIST
     mov [ebp+TNestNode.cy],eax
     mov eax,[ebp+TNestNode.DistanceMap+DIST_LEFT*16+4]
     mov [ebp+TNestNode.cx],eax
     add ebp,sizeof.TNestNode
     dec edx
  jne @b

  mov ecx,sizeof.TSheet*MAX_SHEETS shr 2
  mov edi,Sheets
  xor eax,eax
  rep stosd

;Do nesting
  mov      esi,[NestNodeCount]
  mov      eax,[NestingOrder]
  movzx    ecx,[GPUNestParams.MinDist]
  mov      eax,[eax+esi*4-4]
  cvtsi2sd xmm1,ecx
  movsd    xmm0,[eax+TNestNode.Width]
  addsd    xmm0,xmm1
  movq     [NotPlacedOfs],xmm0
  movzx    eax,[GPUNestParams.FitToBiggest]
  sub      esi,eax
  neg      eax
  and      dword[NotPlacedOfs],eax
  and      dword[NotPlacedOfs+4],eax
  mov      [SheetsCount],0
  .NextShape:
    mov    ebp,[NestingOrder]
    mov    ebp,[ebp+esi*4-4]
    movzx  eax,[GPUNestParams.SheetWidth]
    movzx  edx,[GPUNestParams.SheetHeight]
    invoke glViewport,0,0,eax,edx

    mov    [CurSheet],0
    mov    ebx,Sheets
    .NextSheet:
      mov  eax,[CurSheet]
      cmp  eax,[SheetsCount]
      jl .TryToPlace
        cmp    [SheetsCount],MAX_SHEETS
        jnae @f
          mov ebx,errSheets
          jmp .error
        @@:
        mov    [ebx+TSheet.Empty],1
        lea    eax,[ebx+TSheet.FBO]
        invoke glGenFramebuffers,1,eax
        invoke glBindFramebuffer,GL_FRAMEBUFFER,[ebx+TSheet.FBO]
        lea    eax,[ebx+TSheet.Texture]
        invoke glGenTextures,1,eax
        invoke glBindTexture,GL_TEXTURE_RECTANGLE,[ebx+TSheet.Texture]
        invoke glTexParameteri,GL_TEXTURE_RECTANGLE,GL_TEXTURE_MAG_FILTER,GL_NEAREST
        invoke glTexParameteri,GL_TEXTURE_RECTANGLE,GL_TEXTURE_MIN_FILTER,GL_NEAREST
        movzx  eax,[GPUNestParams.SheetWidth]
        movzx  edx,[GPUNestParams.SheetHeight]
        invoke glTexImage2D,GL_TEXTURE_RECTANGLE,0,GL_RED,eax,edx,0,GL_RED,GL_UNSIGNED_INT,0
        invoke glFramebufferTexture2D,GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_RECTANGLE,[ebx+TSheet.Texture],0
        cmp    [GPUNestParams.FitToBiggest],0
        jne @f
          invoke glClearColor,0,0,0,1.0
          invoke glClear,GL_COLOR_BUFFER_BIT
          jmp    .Finish
        @@:
          mov       edi,[NestingOrder]
          mov       eax,[NestNodeCount]
          mov       edi,[edi+eax*4-4]
          invoke    glClearColor,1.0,0,0,1.0
          invoke    glClear,GL_COLOR_BUFFER_BIT
          invoke    glLinkProgram,[Put]
          invoke    glUseProgram,[Put]
          invoke    glActiveTexture,GL_TEXTURE0
          invoke    glBindTexture,GL_TEXTURE_2D,[edi+TNestNode.texture]
          lea       eax,[edi+TNestNode.DistanceMap]
          invoke    glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,eax
          movups    xmm0,dqword[flt_1+12]
          movd      xmm1,dword[GPUNestParams.SheetWidth]
          cvtpi2ps  xmm2,qword[edi+TNestNode.TexWidth]
          punpcklwd xmm1,xmm1
          divps     xmm0,xmm2
          psrld     xmm1,16
          cvtdq2ps  xmm1,xmm1
          movlhps   xmm0,xmm1
          movups    [esp],xmm0
          invoke    glUniform4fv,0,1,esp;1/TexWidth,0.5/TexHeight,GPUNestParams.SheetWidth,GPUNestParams.SheetHeight
          invoke    glUniform4f,1,0,0,0,1.0
          invoke    glBegin,GL_POINTS
          invoke    glVertex4i,[edi+TNestNode.DistanceMap+DIST_LEFT*16+4],[edi+TNestNode.DistanceMap+DIST_BOTTOM*16+4],0,1
          invoke    glEnd
        .Finish:
        invoke glFinish
        inc    [SheetsCount]
      .TryToPlace:

      invoke glBindFramebuffer,GL_FRAMEBUFFER,[ebx+TSheet.FBO]
      invoke glLinkProgram,[Place]
      invoke glUseProgram,[Place]
      invoke glActiveTexture,GL_TEXTURE1
      invoke glBindTexture,GL_TEXTURE_RECTANGLE,[ebx+TSheet.Texture]
      invoke glActiveTexture,GL_TEXTURE0
      invoke glBindTexture,GL_TEXTURE_2D,[ebp+TNestNode.texture]

;Generate NFP for sheet
      movzx  eax,[GPUNestParams.SheetWidth]
      movzx  edx,[GPUNestParams.SheetHeight]
      mov    edi,[FrameBuffer]
      invoke glReadPixels,0,0,eax,edx,GL_RED,GL_UNSIGNED_INT,edi
      push   esi
      mov    eax,[NFP]
      add    eax,8
      movzx  edx,[GPUNestParams.SheetWidth]
      lea    esi,[edx-1]
      neg    edx
      xor    ecx,ecx
      .NextRow:
        xor cx,cx
        .NextCol:
          cmp  dword[edi],0
          jne .continue
            test  cx,cx
            je    .left
            cmp   dword[edi-4],0
            je    @f
              .left:mov [eax],ecx
                    mov word[eax+4],0
                    add eax,8
            @@:
            test  ecx,$FFFF0000
            je    .bottom
            cmp   dword[edi+edx*4],0
            je    @f
            .bottom:mov [eax],ecx
                    mov word[eax+4],1
                    add eax,8
            @@:
            cmp   cx,si
            je    .right
            cmp   dword[edi+4],0
            je    @f
             .right:mov [eax],ecx
                    mov word[eax+4],2
                    add eax,8
            @@:
          .continue:
          add edi,4
          inc cx
          cmp cx,si
        jna .NextCol
        add ecx,10000h
        cmp ecx,dword[GPUNestParams.SheetWidth]
      jnae .NextRow
      pop    esi
      sub    eax,[NFP]
      shr    eax,3
      mov    [NFPLen],eax

;Generate placement map
      mov       cl,[GPUNestParams.RotationsLevel]
      shl       eax,cl
      mov       ecx,eax
      shl       eax,2
      mov       [PlacementMapLen],eax
      mov       edi,[PlacementMap]
      xor       eax,eax
      rep       stosd
      lea       eax,[ebp+TNestNode.DistanceMap]
      invoke    glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,eax
      invoke    glNamedBufferSubData,[SSBO+4],0,[PlacementMapLen],[PlacementMap]
      mov       eax,[FrameBufferLen]
      shl       eax,2
      invoke    glNamedBufferSubData,[SSBO+8],0,eax,[FrameBuffer]
      movd      xmm2,dword[GPUNestParams.SheetWidth]
      movups    xmm0,dqword[flt_1+12] ;1.0, 0.5, 0.5, 0.5
      punpcklwd xmm2,xmm2
      cvtpi2ps  xmm1,qword[ebp+TNestNode.TexWidth]
      psrld     xmm2,16
      divps     xmm0,xmm1
      cvtdq2ps  xmm2,xmm2
      movlhps   xmm0,xmm2
      movups    [esp],xmm0
      invoke    glUniform4fv,0,1,esp ;1/TexWidth,0.5/TexHeight,GPUNestParams.SheetWidth,GPUNestParams.SheetHeight
      movzx     eax,[GPUNestParams.RotationsLevel]
      invoke    glUniform1i,1,eax
      mov       eax,[NFPLen]
      shl       eax,3
      invoke    glBufferData,GL_ARRAY_BUFFER,eax,[NFP],GL_DYNAMIC_DRAW
      invoke    glDrawArrays,GL_POINTS,0,[NFPLen]
      invoke    glFinish
      invoke    glGetNamedBufferSubData,[SSBO+4],0,[PlacementMapLen],[PlacementMap]
      mov       eax,[PlacementMap]
      cmp       dword[eax],0
      jne @f
        mov ebx,errTDR
        jmp .error
      @@:

;Choose optimal place according to selected strategy
      mov   [OptimalPlace],-1
      movzx eax,[GPUNestParams.NestingAlgo]
      mov   edi,[PlacementMap]
      movzx ecx,[GPUNestParams.RotationsLevel]
      mov   edx,[NFPLen]
      shl   edx,cl
      jmp   [.case+eax*4]
      .case dd .ByCoords,.ByHeightMap,.ByHeight
         .ByCoords:mov   ecx,edx
                   xor   eax,eax
                   repne scasd
                   sub   edi,[PlacementMap]
                   shr   edi,2
                   cmp   edi,edx
                   jae .next
                     mov [ebx+TSheet.Empty],0
                     mov [TargetSheet],ebx
                     dec edi
                     mov [OptimalPlace],edi
                     jmp .next
      .ByHeightMap:mov    edi,edx
                   movzx  eax,[GPUNestParams.SheetWidth]
                   mov    edx,[ebp+TNestNode.HeightMap.Width]
                   mov    [esp],eax
                   mov    [esp+4],edx
                   mov    [esp+8],ecx
                   invoke glLinkProgram,[CalcArea]
                   invoke glUseProgram,[CalcArea]
                   lea    eax,[ebp+TNestNode.DistanceMap]
                   invoke glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,eax
                   mov    eax,[ebp+TNestNode.HeightMap.Width]
                   shl    eax,MAX_ROTATIONS_LEVEL+2
                   invoke glNamedBufferSubData,[SSBO],sizeof.DistanceMap,eax,[ebp+TNestNode.HeightMap.Data]
                   invoke glNamedBufferSubData,[SSBO+4],0,[PlacementMapLen],[PlacementMap]
                   invoke glNamedBufferSubData,[SSBO+8],0,16,esp
                   lea    eax,[ebx+TSheet.HeightMap]
                   invoke glNamedBufferSubData,[SSBO+8],16,sizeof.TSheet.Heightmap,eax
                   mov    eax,[NFPLen]
                   shl    eax,3
                   invoke glNamedBufferSubData,[SSBO+8],16+sizeof.TSheet.Heightmap,eax,[NFP]
                   invoke glDispatchCompute,[NFPLen],1,1
                   invoke glFinish
                   invoke glGetNamedBufferSubData,[SSBO+4],0,[PlacementMapLen],[PlacementMap]
                   mov    edx,[PlacementMap]
                   mov    eax,-2
                   @@:cmp    eax,[edx+edi*4-4]
                      cmovae eax,[edx+edi*4-4]
                      cmovae ecx,edi
                      dec    edi
                   jne @b
                   cmp eax,-2
                   je .next
                     dec ecx
                     mov [OptimalPlace],ecx
                     mov [ebx+TSheet.Empty],0
                     mov [TargetSheet],ebx
                     jmp .next
         .ByHeight:push esi
                   mov  [MinHeight],-1
                   .Search:
                     cmp  dword[edi+edx*4-4],0
                     jne @f
                       lea   eax,[edx-1]
                       mov   cl,[GPUNestParams.RotationsLevel]
                       xor   esi,esi
                       shrd  esi,eax,cl
                       shr   eax,cl
                       shr   esi,32-MAX_ROTATIONS_LEVEL-4
                       mov   ecx,[NFP]
                       lea   eax,[ecx+eax*8]
                       movzx ecx,word[eax+2]
                       movzx eax,word[eax+4]
                       jmp   [.case3+eax*4]
                       .case3 dd .snap_left,.snap_bottom,.snap_right
                         .snap_left:sub ecx,[ebp+TNestNode.DistanceMap+esi+DIST_LEFT*16+8]
                                    jmp .break
                       .snap_bottom:add ecx,[ebp+TNestNode.DistanceMap+esi+DIST_BOTTOM*16+4]
                                    jmp .break
                        .snap_right:add ecx,[ebp+TNestNode.DistanceMap+esi+DIST_RIGHT*16]
                     .break:
                     add   ecx,[ebp+TNestNode.DistanceMap+esi+DIST_TOP*16+4]
                     add   ecx,[ebp+TNestNode.DistanceMap+esi+DIST_BOTTOM*16+4]
                     cmp   ecx,[MinHeight]
                     ja @f
                       mov [MinHeight],ecx
                       lea ecx,[edx-1]
                       mov [OptimalPlace],ecx
                       mov [ebx+TSheet.Empty],0
                       mov [TargetSheet],ebx
                     @@:
                     dec   edx
                   jne .Search
                   pop esi
      .next:
      cmp [StopNesting],0
      jne .exit

      inc  [CurSheet]
      add  ebx,sizeof.TSheet
      mov  eax,[OptimalPlace]
      inc  eax
      or   eax,dword[ebx-sizeof.TSheet+TSheet.Empty]
    je .NextSheet

;if place is found put shape in the target sheet
    cmp [OptimalPlace],-1
    je .PlaceNotFound
      mov       eax,[TargetSheet]
      invoke    glBindFramebuffer,GL_FRAMEBUFFER,[eax+TSheet.FBO]
      invoke    glLinkProgram,[Put]
      invoke    glUseProgram,[Put]
      invoke    glActiveTexture,GL_TEXTURE0
      invoke    glBindTexture,GL_TEXTURE_2D,[ebp+TNestNode.texture]
      lea       eax,[ebp+TNestNode.DistanceMap]
      invoke    glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,eax
      movups    xmm0,dqword[flt_1+12]
      movd      xmm1,dword[GPUNestParams.SheetWidth]
      cvtpi2ps  xmm2,qword[ebp+TNestNode.TexWidth]
      punpcklwd xmm1,xmm1
      divps     xmm0,xmm2
      psrld     xmm1,16
      cvtdq2ps  xmm1,xmm1
      movlhps   xmm0,xmm1
      movups    [esp],xmm0
      invoke    glUniform4fv,0,1,esp;1/TexWidth,0.5/TexHeight,GPUNestParams.SheetWidth,GPUNestParams.SheetHeight
      invoke    glUniform4fv,1,1,flt_1
      mov       edx,[OptimalPlace]
      mov       cl,[GPUNestParams.RotationsLevel]
      xor       ebx,ebx
      shrd      ebx,edx,cl
      shr       edx,cl
      shr       ebx,32-MAX_ROTATIONS_LEVEL
      mov       [ebp+TNestNode.angle],ebx  ;angle:=(OptimalPlace and ((1 shl GPUNestParams.RotationsLevel)-1))shl(MAX_ROTATIONS_LEVEL-GPUNestParams.RotationsLevel);
      shl       ebx,4
      shl       edx,3
      add       edx,[NFP]
      movd      xmm0,[edx]
      punpcklwd xmm0,xmm0
      psrld     xmm0,16
      cvtdq2pd  xmm0,xmm0
      movapd    dqword[ebp+TNestNode.x],xmm0
      movzx     eax,word[edx+4]
      movzx     ecx,word[edx]
      movzx     edx,word[edx+2]
      jmp       [.case2+eax*4]
      .case2 dd .SnapLeft,.SnapBottom,.SnapRight
        .SnapLeft:mov [ebp+TNestNode.cx],ecx
                  sub edx,[ebp+TNestNode.DistanceMap+ebx+DIST_LEFT*16+8]
                  sub edx,[ebp+TNestNode.DistanceMap+ebx+DIST_BOTTOM*16+4]
                  mov [ebp+TNestNode.cy],edx
                  jmp near @f
      .SnapBottom:add ecx,[ebp+TNestNode.DistanceMap+ebx+DIST_BOTTOM*16]
                  sub ecx,[ebp+TNestNode.DistanceMap+ebx+DIST_LEFT*16+4]
                  mov [ebp+TNestNode.cx],ecx
                  mov [ebp+TNestNode.cy],edx
                  jmp near @f
       .SnapRight:sub ecx,[ebp+TNestNode.DistanceMap+ebx+DIST_RIGHT*16+4]
                  sub ecx,[ebp+TNestNode.DistanceMap+ebx+DIST_LEFT*16+4]
                  sub edx,[ebp+TNestNode.DistanceMap+ebx+DIST_BOTTOM*16+4]
                  add edx,[ebp+TNestNode.DistanceMap+ebx+DIST_RIGHT*16]
                  mov [ebp+TNestNode.cx],ecx
                  mov [ebp+TNestNode.cy],edx
      @@:

     cmp [GPUNestParams.NestingAlgo],1
     jne .SkipHeightMap
       mov eax,[ebp+TNestNode.angle]
       mul [ebp+TNestNode.HeightMap.Width]
       mov edi,[ebp+TNestNode.HeightMap.Data]
       lea edi,[edi+eax*4]
       mov eax,[ebp+TNestNode.cx]
       mov ecx,[TargetSheet]
       lea eax,[ecx+TSheet.HeightMap+eax*4]
       mov ecx,[edi]
       @@:mov   edx,[edi+ecx*4]
          add   edx,[ebp+TNestNode.cy]
          cmp   [eax+ecx*4-4],edx
          cmova edx,[eax+ecx*4-4]
          mov   [eax+ecx*4-4],edx
       loop @b
     .SkipHeightMap:

      mov      ecx,[ebp+TNestNode.DistanceMap+ebx+DIST_LEFT*16+4]
      mov      edx,[ebp+TNestNode.DistanceMap+ebx+DIST_BOTTOM*16+4]
      add      [ebp+TNestNode.cx],ecx
      add      [ebp+TNestNode.cy],edx
      invoke   glBegin,GL_POINTS
      invoke   glVertex4i,[ebp+TNestNode.cx],[ebp+TNestNode.cy],[ebp+TNestNode.angle],1
      invoke   glEnd

      mov      eax,[TargetSheet]
      sub      eax,Sheets
      mov      ecx,sizeof.TSheet
      cdq
      div      ecx
      movzx    ecx,[GPUNestParams.SheetWidth]
      add      ecx,MAX_DIST
      mul      ecx
      add      eax,[ebp+TNestNode.cx]
      cvtsi2sd xmm0,eax
      mov      [ebp+TNestNode.cx],eax ;(GPUNestParams.SheetWidth+MAX_DIST)*TargetSheetIndex
      movsd    [ebp+TNestNode.x],xmm0
      jmp @f
    .PlaceNotFound:
      mov      [ebp+TNestNode.angle],0
      movzx    eax,[GPUNestParams.SheetHeight]
      add      eax,MAX_DIST
      cvtsi2sd xmm0,eax
      movlhps  xmm0,xmm0
      movlps   xmm0,[NotPlacedOfs]
      movaps   dqword[ebp+TNestNode.x],xmm0
      cvtsd2si ecx,xmm0
      movzx    edx,[GPUNestParams.SheetHeight]
      add      ecx,[ebp+TNestNode.DistanceMap+DIST_LEFT*16+4]
      add      edx,[ebp+TNestNode.DistanceMap+DIST_BOTTOM*16+4]
      add      edx,MAX_DIST
      mov      [ebp+TNestNode.cx],ecx
      mov      [ebp+TNestNode.cy],edx
      movzx    eax,[GPUNestParams.MinDist]
      cvtsi2sd xmm1,eax
      addsd    xmm0,[ebp+TNestNode.Width]
      addsd    xmm0,xmm1
      movsd    [NotPlacedOfs],xmm0
    @@:

;Update preview window
    movzx     eax,[GPUNestParams.SheetWidth]
    add       eax,MAX_DIST
    imul      eax,[SheetsCount]
    sub       eax,MAX_DIST
    cvtpi2ps  xmm0,qword[Preview.Width]
    movd      xmm1,eax
    pinsrw    xmm1,[GPUNestParams.SheetHeight],2
    cvtdq2ps  xmm1,xmm1
    divps     xmm0,xmm1
    movaps    xmm1,xmm0
    shufps    xmm0,xmm0,1
    minss     xmm0,xmm1
    movss     [Preview.scale],xmm0
    mov       [Preview.x],0
    mov       [Preview.y],0
    call      Rescale
    stdcall   PreviewProc,[PreviewWindow],WM_PAINT,0,0

    dec esi
  jne .NextShape

  jmp .exit
    .error:
    invoke MessageBoxW,[DialogWindow],ebx,0,0
  .exit:

  invoke VirtualFree,[NFP],0,MEM_RELEASE
  invoke VirtualFree,[FrameBuffer],0,MEM_RELEASE
  invoke VirtualFree,[PlacementMap],0,MEM_RELEASE
  invoke glDeleteVertexArrays,1,VAO
  invoke glDeleteBuffers,1,VBO
  invoke glDeleteBuffers,3,SSBO
  mov    edi,[SheetsCount]
  mov    esi,Sheets
  @@:lea    eax,[esi+TSheet.FBO]
     invoke glDeleteFramebuffers,1,eax
     lea    eax,[esi+TSheet.Texture]
     invoke glDeleteTextures,1,eax
     add    esi,sizeof.TSheet
     sub    edi,1
  jnle @b

  mov    [NestingThreadId],0
  invoke SendMessageW,[ApplyButton],WM_SETTEXT,0,strApply
  invoke InvalidateRect,[ApplyButton],0,0
  invoke EnableWindow,[OkButton],1
  invoke EnableWindow,[RotationsTrackBar],1
  invoke EnableWindow,[MinDistTrackBar],1

  cmp ebx,errTDR
  jne @f
    invoke  PostMessageW,[DialogWindow],WM_CLOSE,0,0
  @@:
  add esp,16
  popad
ret 4