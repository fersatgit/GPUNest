proc Nesting lParam
local tmpDQWORD:DQWORD
  invoke wglMakeCurrent,[Preview.DC],[RC]
  movzx  eax,[GPUNestParams.SheetWidth]
  movzx  edx,[GPUNestParams.SheetHeight]
  mul    edx
  mov    [FrameBufferLen],eax
  mov    edi,eax
  mov    cl,[GPUNestParams.RotationsLevel]
  add    cl,2
  shl    eax,cl
  mov    [PlacementMapLen],eax
  mov    rbx,errRAM
  shl    edi,2
  invoke VirtualAlloc,0,rdi,MEM_COMMIT,PAGE_READWRITE
  test   rax,rax
  je .error
  mov    [FrameBuffer],rax
  add    edi,edi
  invoke VirtualAlloc,0,rdi,MEM_COMMIT,PAGE_READWRITE
  test   rax,rax
  je .error
  mov    [NFP],rax
  mov    edx,dword[GPUNestParams.SheetWidth]
  add    edx,00010001h
  mov    [rax],edx       ;First element of NFP used for TDR detection
  invoke VirtualAlloc,0,[PlacementMapLen],MEM_COMMIT,PAGE_READWRITE
  test   rax,rax
  je .error
  mov    [PlacementMap],rax

  invoke glGenVertexArrays,1,VAO
  invoke glBindVertexArray,[VAO]
  invoke glGenBuffers,1,VBO
  invoke glBindBuffer,GL_ARRAY_BUFFER,[VBO]
  invoke glVertexAttribPointer,0,4,GL_UNSIGNED_SHORT,GL_FALSE,8,0
  invoke glEnableVertexAttribArray,0

  mov    rbx,errVideoRAM
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
  mov      [tmpHeightMap],rax

  cvtsi2ss xmm0,[MaxDiameter]
  mulss    xmm0,[flt_05]
  subss    xmm0,[flt_1]
  roundss  xmm0,xmm0,2 ;roundup(MaxDiameter*0.5-1)
  shufps   xmm0,xmm0,0
  cvtps2dq xmm0,xmm0
  movdqa   dqword[delta],xmm0

  mov      esi,[NestNodeCount]
  mov      rbp,[NestNodes]
  .GetBounds:
     movapd   xmm0,dqword[rbp+TNestNode.Width]
     dppd     xmm0,xmm0,110001b
     sqrtsd   xmm0,xmm0
     cvtsd2si eax,xmm0
     add      eax,MAX_DIST+1
     mov      [rbp+TNestNode.HeightMap.Width],eax
     shl      eax,MAX_ROTATIONS_LEVEL+2
     mov      [HeightMapSize],eax
     invoke   VirtualAlloc,0,eax,MEM_COMMIT,PAGE_READWRITE
     mov      [rbp+TNestNode.HeightMap.Data],rax
     invoke   glBindTexture,GL_TEXTURE_2D,[rbp+TNestNode.texture]
     mov      ecx,[tmpHeightMapSize]
     shr      ecx,2
     mov      rdi,[tmpHeightMap]
     xor      eax,eax
     rep      stosd
     invoke   glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,addr rbp+TNestNode.DistanceMap
     invoke   glNamedBufferSubData,[SSBO],sizeof.DistanceMap,[tmpHeightMapSize],[tmpHeightMap]
     invoke   glBegin,GL_POINTS
     cvtpd2ps xmm0,dqword[rbp+TNestNode.Width]
     movzx    eax,[GPUNestParams.MinDist]
     shufps   xmm0,xmm0,01000100b
     cvtsi2ss xmm1,eax
     cvtsi2ss xmm2,[MaxDiameter]
     cvtpi2ps xmm3,qword[rbp+TNestNode.TexWidth]
     shufps   xmm1,xmm1,0
     shufps   xmm2,xmm3,01000000b
     addps    xmm0,xmm1
     divps    xmm0,xmm2
     psubd    xmm0,dqword[flt_exp_0_0_1_2] ;(Width+GPUNestParams.MinDist)/MaxDiameter,(Height+GPUNestParams.MinDist)/MaxDiameter,(Width+GPUNestParams.MinDist)/TexWidth*0.5,(Height+GPUNestParams.MinDist)/TexHeight*0.25
     movdqu   [tmpDQWORD],xmm0
     invoke   glVertex4fv,addr tmpDQWORD
     invoke   glEnd
     invoke   glFinish
     invoke   glGetNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,addr rbp+TNestNode.DistanceMap
     invoke   glGetNamedBufferSubData,[SSBO],sizeof.DistanceMap,[tmpHeightMapSize],[tmpHeightMap]

     mov      ecx,MAX_ROTATIONS*16-16
     movdqa   xmm1,dqword[delta]
     mov      rdx,[tmpHeightMap]
     .RecalcDistanceMap:
       movdqa    xmm0,dqword[rbp+TNestNode.DistanceMap+rcx]
       pshufd    xmm0,xmm0,1000b
       punpcklwd xmm0,xmm0
       psrad     xmm0,16
       psubd     xmm0,xmm1
       cmp       word[rbp+TNestNode.DistanceMap+rcx+2],0
       ja @f
         mov  eax,[MaxDiameter]
         pxor xmm0,xmm0
         imul eax,ecx
         shr  eax,4
         mov  dword[rdx+rax*4],1
       @@:
       movdqa dqword[rbp+TNestNode.DistanceMap+rcx],xmm0
       movdqa dqword[rbp+TNestNode.DistanceMap+rcx+MAX_ROTATIONS*16],xmm0
       sub    ecx,16
     jnl .RecalcDistanceMap

     mov      edx,(MAX_ROTATIONS-1)*4
     mov      rbx,[rbp+TNestNode.HeightMap.Data]
     mov      eax,[HeightMapSize]
     add      rbx,rax
     .RecalcHeightMap:
       mov  ecx,[MaxDiameter]
       mov  edi,ecx
       imul edi,edx
       add  rdi,[tmpHeightMap]
       xor  eax,eax
       repe scasd
       mov  eax,[rbp+TNestNode.HeightMap.Width]
       shl  eax,2
       sub  rbx,rax
       xor  ecx,ecx
       mov  r8d,[rbp+TNestNode.DistanceMap+DIST_BOTTOM*16+rdx*4+4]
       sub  r8d,[delta]
       @@:mov eax,[rdi+rcx*4-4]
          add rax,r8
          mov [rbx+rcx*4+4],eax
          inc ecx
          cmp dword[rdi+rcx*4-4],0
       jne @b
       mov  [rbx],ecx
       sub  edx,4
     jnl .RecalcHeightMap
     add      rbp,sizeof.TNestNode
     dec      esi
  jne .GetBounds
  invoke VirtualFree,[tmpHeightMap],0,MEM_RELEASE
  invoke glDeleteFramebuffers,1,FBO
  invoke glDeleteTextures,1,Texture

;Reset shapes position
  mov edx,[NestNodeCount]
  mov rbp,[NestNodes]
  @@:mov eax,[rbp+TNestNode.DistanceMap+DIST_BOTTOM*16+4]
     add ax,[GPUNestParams.SheetHeight]
     add eax,MAX_DIST
     mov [rbp+TNestNode.cy],eax
     mov eax,[rbp+TNestNode.DistanceMap+DIST_LEFT*16+4]
     mov [rbp+TNestNode.cx],eax
     add rbp,sizeof.TNestNode
     dec edx
  jne @b

  mov ecx,sizeof.TSheet*MAX_SHEETS shr 2
  mov rdi,Sheets
  xor eax,eax
  rep stosd

;Do nesting
  mov      esi,[NestNodeCount]
  mov      rax,[NestingOrder]
  movzx    ecx,[GPUNestParams.MinDist]
  mov      rax,[rax+rsi*8-8]
  cvtsi2sd xmm1,ecx
  movsd    xmm0,[rax+TNestNode.Width]
  addsd    xmm0,xmm1
  movq     [NotPlacedOfs],xmm0
  movzx    eax,[GPUNestParams.FitToBiggest]
  sub      esi,eax
  neg      rax
  and      [NotPlacedOfs],rax
  mov      [SheetsCount],0
  .NextShape:
    mov    rbp,[NestingOrder]
    mov    rbp,[rbp+rsi*8-8]
    movzx  r8,[GPUNestParams.SheetWidth]
    movzx  r9,[GPUNestParams.SheetHeight]
    invoke glViewport,0,0,r8,r9

    mov    [CurSheet],0
    mov    rbx,Sheets
    .NextSheet:
      mov  eax,[CurSheet]
      cmp  eax,[SheetsCount]
      jl .TryToPlace
        cmp    [SheetsCount],MAX_SHEETS
        jnae @f
          mov rbx,errSheets
          jmp .error
        @@:
        mov    [rbx+TSheet.Empty],1
        invoke glGenFramebuffers,1,addr rbx+TSheet.FBO
        invoke glBindFramebuffer,GL_FRAMEBUFFER,[rbx+TSheet.FBO]
        invoke glGenTextures,1,addr rbx+TSheet.Texture
        invoke glBindTexture,GL_TEXTURE_RECTANGLE,[rbx+TSheet.Texture]
        invoke glTexParameteri,GL_TEXTURE_RECTANGLE,GL_TEXTURE_MAG_FILTER,GL_NEAREST
        invoke glTexParameteri,GL_TEXTURE_RECTANGLE,GL_TEXTURE_MIN_FILTER,GL_NEAREST
        movzx  r9,[GPUNestParams.SheetWidth]
        movzx  r10,[GPUNestParams.SheetHeight]
        invoke glTexImage2D,GL_TEXTURE_RECTANGLE,0,GL_RED,r9,r10,0,GL_RED,GL_UNSIGNED_INT,0
        invoke glFramebufferTexture2D,GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_RECTANGLE,[rbx+TSheet.Texture],0
        cmp    [GPUNestParams.FitToBiggest],0
        xorps  xmm1,xmm1
        xorps  xmm2,xmm2
        movss  xmm3,[flt_1]
        jne @f
          invoke glClearColor,float xmm1,xmm1,xmm2,xmm3
          invoke glClear,GL_COLOR_BUFFER_BIT
          jmp    .Finish
        @@:
          mov       rdi,[NestingOrder]
          mov       eax,[NestNodeCount]
          mov       rdi,[rdi+rax*8-8]
          invoke    glClearColor,float xmm3,xmm1,xmm2,xmm3
          invoke    glClear,GL_COLOR_BUFFER_BIT
          invoke    glLinkProgram,[Put]
          invoke    glUseProgram,[Put]
          invoke    glActiveTexture,GL_TEXTURE0
          invoke    glBindTexture,GL_TEXTURE_2D,[rdi+TNestNode.texture]
          invoke    glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,addr rdi+TNestNode.DistanceMap
          movups    xmm0,dqword[flt_1+12]
          movd      xmm1,dword[GPUNestParams.SheetWidth]
          cvtpi2ps  xmm2,qword[rdi+TNestNode.TexWidth]
          punpcklwd xmm1,xmm1
          divps     xmm0,xmm2
          psrld     xmm1,16
          cvtdq2ps  xmm1,xmm1
          movlhps   xmm0,xmm1
          movups    [tmpDQWORD],xmm0
          invoke    glUniform4fv,0,1,addr tmpDQWORD;1/TexWidth,0.5/TexHeight,GPUNestParams.SheetWidth,GPUNestParams.SheetHeight
          xorps     xmm0,xmm0
          xorps     xmm1,xmm1
          xorps     xmm2,xmm2
          invoke    glUniform4f,1,xmm0,xmm1,xmm2,float [flt_1]
          invoke    glBegin,GL_POINTS
          invoke    glVertex4i,[rdi+TNestNode.DistanceMap+DIST_LEFT*16+4],[rdi+TNestNode.DistanceMap+DIST_BOTTOM*16+4],0,1
          invoke    glEnd
        .Finish:
        invoke glFinish
        inc    [SheetsCount]
      .TryToPlace:

      invoke glBindFramebuffer,GL_FRAMEBUFFER,[rbx+TSheet.FBO]
      invoke glLinkProgram,[Place]
      invoke glUseProgram,[Place]
      invoke glActiveTexture,GL_TEXTURE1
      invoke glBindTexture,GL_TEXTURE_RECTANGLE,[rbx+TSheet.Texture]
      invoke glActiveTexture,GL_TEXTURE0
      invoke glBindTexture,GL_TEXTURE_2D,[rbp+TNestNode.texture]

;Generate NFP for sheet
      movzx  r8,[GPUNestParams.SheetWidth]
      movzx  r9,[GPUNestParams.SheetHeight]
      mov    rdi,[FrameBuffer]
      invoke glReadPixels,0,0,r8,r9,GL_RED,GL_UNSIGNED_INT,rdi
      mov    rax,[NFP]
      add    rax,8
      movzx  edx,[GPUNestParams.SheetWidth]
      lea    r12,[rdx-1]
      neg    rdx
      xor    ecx,ecx
      .NextRow:
        xor cx,cx
        .NextCol:
          cmp  dword[rdi],0
          jne .continue
            test  cx,cx
            je    .left
            cmp   dword[rdi-4],0
            je    @f
              .left:mov [rax],ecx
                    mov word[rax+4],0
                    add rax,8
            @@:
            test  ecx,$FFFF0000
            je    .bottom
            cmp   dword[rdi+rdx*4],0
            je    @f
            .bottom:mov [rax],ecx
                    mov word[rax+4],1
                    add rax,8
            @@:
            cmp   cx,r12w
            je    .right
            cmp   dword[rdi+4],0
            je    @f
             .right:mov [rax],ecx
                    mov word[rax+4],2
                    add rax,8
            @@:
          .continue:
          add rdi,4
          inc cx
          cmp cx,r12w
        jna .NextCol
        add ecx,10000h
        cmp ecx,dword[GPUNestParams.SheetWidth]
      jnae .NextRow
      sub    rax,[NFP]
      shr    rax,3
      mov    [NFPLen],eax

;Generate placement map
      mov       cl,[GPUNestParams.RotationsLevel]
      shl       eax,cl
      mov       ecx,eax
      shl       eax,2
      mov       [PlacementMapLen],eax
      mov       rdi,[PlacementMap]
      xor       eax,eax
      rep       stosd
      invoke    glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,addr rbp+TNestNode.DistanceMap
      invoke    glNamedBufferSubData,[SSBO+4],0,[PlacementMapLen],[PlacementMap]
      mov       r8d,[FrameBufferLen]
      shl       r8,2
      invoke    glNamedBufferSubData,[SSBO+8],0,r8,[FrameBuffer]
      movd      xmm2,dword[GPUNestParams.SheetWidth]
      movups    xmm0,dqword[flt_1+12] ;1.0, 0.5, 0.5, 0.5
      punpcklwd xmm2,xmm2
      cvtpi2ps  xmm1,qword[rbp+TNestNode.TexWidth]
      psrld     xmm2,16
      divps     xmm0,xmm1
      cvtdq2ps  xmm2,xmm2
      movlhps   xmm0,xmm2
      movups    [tmpDQWORD],xmm0
      invoke    glUniform4fv,0,1,addr tmpDQWORD ;1/TexWidth,0.5/TexHeight,GPUNestParams.SheetWidth,GPUNestParams.SheetHeight
      movzx     edx,[GPUNestParams.RotationsLevel]
      invoke    glUniform1i,1,rdx
      mov       edx,[NFPLen]
      shl       edx,3
      invoke    glBufferData,GL_ARRAY_BUFFER,rdx,[NFP],GL_DYNAMIC_DRAW
      invoke    glDrawArrays,GL_POINTS,0,[NFPLen]
      invoke    glFinish
      invoke    glGetNamedBufferSubData,[SSBO+4],0,[PlacementMapLen],[PlacementMap]
      mov       rax,[PlacementMap]
      cmp       dword[rax],0
      jne @f
        mov rbx,errTDR
        jmp .error
      @@:

;Choose optimal place according to selected strategy
      mov   [OptimalPlace],-1
      movzx eax,[GPUNestParams.NestingAlgo]
      mov   rdi,[PlacementMap]
      movzx ecx,[GPUNestParams.RotationsLevel]
      mov   edx,[NFPLen]
      shl   edx,cl
      jmp   [.case+rax*8]
      .case dq .ByCoords,.ByHeightMap,.ByHeight
         .ByCoords:mov   ecx,edx
                   xor   eax,eax
                   repne scasd
                   sub   rdi,[PlacementMap]
                   shr   edi,2
                   cmp   edi,edx
                   jae .next
                     mov [rbx+TSheet.Empty],0
                     mov [TargetSheet],rbx
                     dec edi
                     mov [OptimalPlace],edi
                     jmp .next
      .ByHeightMap:mov    edi,edx
                   movzx  eax,[GPUNestParams.SheetWidth]
                   mov    edx,[rbp+TNestNode.HeightMap.Width]
                   mov    dword[tmpDQWORD],eax
                   mov    dword[tmpDQWORD+4],edx
                   mov    dword[tmpDQWORD+8],ecx
                   invoke glLinkProgram,[CalcArea]
                   invoke glUseProgram,[CalcArea]
                   invoke glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,addr rbp+TNestNode.DistanceMap
                   mov    r8d,[rbp+TNestNode.HeightMap.Width]
                   shl    r8,MAX_ROTATIONS_LEVEL+2
                   invoke glNamedBufferSubData,[SSBO],sizeof.DistanceMap,r8,[rbp+TNestNode.HeightMap.Data]
                   invoke glNamedBufferSubData,[SSBO+4],0,[PlacementMapLen],[PlacementMap]
                   invoke glNamedBufferSubData,[SSBO+8],0,16,addr tmpDQWORD
                   invoke glNamedBufferSubData,[SSBO+8],16,sizeof.TSheet.Heightmap,addr rbx+TSheet.HeightMap
                   mov    r8d,[NFPLen]
                   shl    r8,3
                   invoke glNamedBufferSubData,[SSBO+8],16+sizeof.TSheet.Heightmap,r8,[NFP]
                   invoke glDispatchCompute,[NFPLen],1,1
                   invoke glFinish
                   invoke glGetNamedBufferSubData,[SSBO+4],0,[PlacementMapLen],[PlacementMap]
                   mov    rdx,[PlacementMap]
                   mov    eax,-2
                   @@:cmp    eax,[rdx+rdi*4-4]
                      cmovae eax,[rdx+rdi*4-4]
                      cmovae ecx,edi
                      dec    edi
                   jne @b
                   cmp eax,-2
                   je .next
                     dec ecx
                     mov [OptimalPlace],ecx
                     mov [rbx+TSheet.Empty],0
                     mov [TargetSheet],rbx
                     jmp .next
         .ByHeight:mov  [MinHeight],-1
                   .Search:
                     cmp  dword[rdi+rdx*4-4],0
                     jne @f
                       lea   eax,[edx-1]
                       mov   cl,[GPUNestParams.RotationsLevel]
                       xor   r12,r12
                       shrd  r12d,eax,cl
                       shr   eax,cl
                       shr   r12,32-MAX_ROTATIONS_LEVEL-4
                       mov   rcx,[NFP]
                       lea   rax,[rcx+rax*8]
                       movzx ecx,word[rax+2]
                       movzx eax,word[rax+4]
                       jmp   [.case3+rax*8]
                       .case3 dq .snap_left,.snap_bottom,.snap_right
                         .snap_left:sub ecx,[rbp+TNestNode.DistanceMap+r12+DIST_LEFT*16+8]
                                    jmp .break
                       .snap_bottom:add ecx,[rbp+TNestNode.DistanceMap+r12+DIST_BOTTOM*16+4]
                                    jmp .break
                        .snap_right:add ecx,[rbp+TNestNode.DistanceMap+r12+DIST_RIGHT*16]
                     .break:
                     add   ecx,[rbp+TNestNode.DistanceMap+r12+DIST_TOP*16+4]
                     add   ecx,[rbp+TNestNode.DistanceMap+r12+DIST_BOTTOM*16+4]
                     cmp   ecx,[MinHeight]
                     ja @f
                       mov [MinHeight],ecx
                       lea ecx,[edx-1]
                       mov [OptimalPlace],ecx
                       mov [rbx+TSheet.Empty],0
                       mov [TargetSheet],rbx
                     @@:
                     dec   edx
                   jne .Search
      .next:
      cmp [StopNesting],0
      jne .exit

      inc  [CurSheet]
      add  rbx,sizeof.TSheet
      mov  eax,[OptimalPlace]
      inc  eax
      or   eax,dword[rbx-sizeof.TSheet+TSheet.Empty]
    je .NextSheet

;if place is found put shape in the target sheet
    cmp [OptimalPlace],-1
    je .PlaceNotFound
      mov       rax,[TargetSheet]
      invoke    glBindFramebuffer,GL_FRAMEBUFFER,[rax+TSheet.FBO]
      invoke    glLinkProgram,[Put]
      invoke    glUseProgram,[Put]
      invoke    glActiveTexture,GL_TEXTURE0
      invoke    glBindTexture,GL_TEXTURE_2D,[rbp+TNestNode.texture]
      invoke    glNamedBufferSubData,[SSBO],0,sizeof.DistanceMap,addr rbp+TNestNode.DistanceMap
      movups    xmm0,dqword[flt_1+12]
      movd      xmm1,dword[GPUNestParams.SheetWidth]
      cvtpi2ps  xmm2,qword[rbp+TNestNode.TexWidth]
      punpcklwd xmm1,xmm1
      divps     xmm0,xmm2
      psrld     xmm1,16
      cvtdq2ps  xmm1,xmm1
      movlhps   xmm0,xmm1
      movups    [tmpDQWORD],xmm0
      invoke    glUniform4fv,0,1,addr tmpDQWORD;1/TexWidth,0.5/TexHeight,GPUNestParams.SheetWidth,GPUNestParams.SheetHeight
      invoke    glUniform4fv,1,1,flt_1
      mov       edx,[OptimalPlace]
      mov       cl,[GPUNestParams.RotationsLevel]
      xor       ebx,ebx
      shrd      ebx,edx,cl
      shr       edx,cl
      shr       ebx,32-MAX_ROTATIONS_LEVEL
      mov       [rbp+TNestNode.angle],ebx  ;angle:=(OptimalPlace and ((1 shl GPUNestParams.RotationsLevel)-1))shl(MAX_ROTATIONS_LEVEL-GPUNestParams.RotationsLevel);
      shl       ebx,4
      shl       edx,3
      add       rdx,[NFP]
      movd      xmm0,[rdx]
      punpcklwd xmm0,xmm0
      psrld     xmm0,16
      cvtdq2pd  xmm0,xmm0
      movapd    dqword[rbp+TNestNode.x],xmm0
      movzx     eax,word[rdx+4]
      movzx     ecx,word[rdx]
      movzx     edx,word[rdx+2]
      jmp       [.case2+rax*8]
      .case2 dq .SnapLeft,.SnapBottom,.SnapRight
        .SnapLeft:mov [rbp+TNestNode.cx],ecx
                  sub edx,[rbp+TNestNode.DistanceMap+rbx+DIST_LEFT*16+8]
                  sub edx,[rbp+TNestNode.DistanceMap+rbx+DIST_BOTTOM*16+4]
                  mov [rbp+TNestNode.cy],edx
                  jmp near @f
      .SnapBottom:add ecx,[rbp+TNestNode.DistanceMap+rbx+DIST_BOTTOM*16]
                  sub ecx,[rbp+TNestNode.DistanceMap+rbx+DIST_LEFT*16+4]
                  mov [rbp+TNestNode.cx],ecx
                  mov [rbp+TNestNode.cy],edx
                  jmp near @f
       .SnapRight:sub ecx,[rbp+TNestNode.DistanceMap+rbx+DIST_RIGHT*16+4]
                  sub ecx,[rbp+TNestNode.DistanceMap+rbx+DIST_LEFT*16+4]
                  sub edx,[rbp+TNestNode.DistanceMap+rbx+DIST_BOTTOM*16+4]
                  add edx,[rbp+TNestNode.DistanceMap+rbx+DIST_RIGHT*16]
                  mov [rbp+TNestNode.cx],ecx
                  mov [rbp+TNestNode.cy],edx
      @@:

     cmp [GPUNestParams.NestingAlgo],1
     jne .SkipHeightMap
       mov eax,[rbp+TNestNode.angle]
       mul [rbp+TNestNode.HeightMap.Width]
       mov rdi,[rbp+TNestNode.HeightMap.Data]
       lea rdi,[rdi+rax*4]
       mov eax,[rbp+TNestNode.cx]
       mov rcx,[TargetSheet]
       lea rax,[rcx+TSheet.HeightMap+rax*4]
       mov ecx,[rdi]
       @@:mov   edx,[rdi+rcx*4]
          add   edx,[rbp+TNestNode.cy]
          cmp   [rax+rcx*4-4],edx
          cmova edx,[rax+rcx*4-4]
          mov   [rax+rcx*4-4],edx
       loop @b
     .SkipHeightMap:

      mov      ecx,[rbp+TNestNode.DistanceMap+rbx+DIST_LEFT*16+4]
      mov      edx,[rbp+TNestNode.DistanceMap+rbx+DIST_BOTTOM*16+4]
      add      [rbp+TNestNode.cx],ecx
      add      [rbp+TNestNode.cy],edx
      invoke   glBegin,GL_POINTS
      invoke   glVertex4i,[rbp+TNestNode.cx],[rbp+TNestNode.cy],[rbp+TNestNode.angle],1
      invoke   glEnd

      mov      rax,[TargetSheet]
      sub      rax,Sheets
      mov      ecx,sizeof.TSheet
      cdq
      div      ecx
      movzx    ecx,[GPUNestParams.SheetWidth]
      add      ecx,MAX_DIST
      mul      ecx
      add      eax,[rbp+TNestNode.cx]
      cvtsi2sd xmm0,eax
      mov      [rbp+TNestNode.cx],eax ;(GPUNestParams.SheetWidth+MAX_DIST)*TargetSheetIndex
      movsd    [rbp+TNestNode.x],xmm0
      jmp @f
    .PlaceNotFound:
      mov      [rbp+TNestNode.angle],0
      movzx    eax,[GPUNestParams.SheetHeight]
      add      eax,MAX_DIST
      cvtsi2sd xmm0,eax
      movlhps  xmm0,xmm0
      movlps   xmm0,[NotPlacedOfs]
      movaps   dqword[rbp+TNestNode.x],xmm0
      cvtsd2si ecx,xmm0
      movzx    edx,[GPUNestParams.SheetHeight]
      add      ecx,[rbp+TNestNode.DistanceMap+DIST_LEFT*16+4]
      add      edx,[rbp+TNestNode.DistanceMap+DIST_BOTTOM*16+4]
      add      edx,MAX_DIST
      mov      [rbp+TNestNode.cx],ecx
      mov      [rbp+TNestNode.cy],edx
      movzx    eax,[GPUNestParams.MinDist]
      cvtsi2sd xmm1,eax
      addsd    xmm0,[rbp+TNestNode.Width]
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
    invoke MessageBoxW,[DialogWindow],rbx,0,0
  .exit:

  invoke VirtualFree,[NFP],0,MEM_RELEASE
  invoke VirtualFree,[FrameBuffer],0,MEM_RELEASE
  invoke VirtualFree,[PlacementMap],0,MEM_RELEASE
  invoke glDeleteVertexArrays,1,VAO
  invoke glDeleteBuffers,1,VBO
  invoke glDeleteBuffers,3,SSBO
  mov    edi,[SheetsCount]
  mov    rsi,Sheets
  @@:invoke glDeleteFramebuffers,1,addr rsi+TSheet.FBO
     invoke glDeleteTextures,1,addr rsi+TSheet.Texture
     add    rsi,sizeof.TSheet
     sub    edi,1
  jnle @b

  mov    [NestingThreadId],0
  invoke SendMessageW,[ApplyButton],WM_SETTEXT,0,strApply
  invoke InvalidateRect,[ApplyButton],0,0
  invoke EnableWindow,[OkButton],1
  invoke EnableWindow,[RotationsTrackBar],1
  invoke EnableWindow,[MinDistTrackBar],1

  cmp rbx,errTDR
  jne @f
    invoke  PostMessageW,[DialogWindow],WM_CLOSE,r8,r9
  @@:
  ret
endp