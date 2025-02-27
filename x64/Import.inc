data import
  library kernel,'KERNEL32.DLL',\
          user,'USER32.DLL',\
          gdi,'GDI32.DLL',\
          opengl,'OPENGL32.DLL',\
          oleaut,'OLEAUT32.DLL',\
          advapi,'ADVAPI32.DLL'

  import kernel,\
         CloseHandle,'CloseHandle',\
         CreateFileW,'CreateFileW',\
         CreateThread,'CreateThread',\
         GetTempPathW,'GetTempPathW',\
         lstrcmpW,'lstrcmpW',\
         lstrcpyW,'lstrcpyW',\
         VirtualAlloc,'VirtualAlloc',\
         VirtualFree,'VirtualFree',\
         WriteFile,'WriteFile',\
         ExitProcess,'ExitProcess'

  import user,\
         SetCapture,'SetCapture',\
         ReleaseCapture,'ReleaseCapture',\
         GetParent,'GetParent',\
         TrackMouseEvent,'TrackMouseEvent',\
         GetDC,'GetDC',\
         DrawTextW,'DrawTextW',\
         GetSysColor,'GetSysColor',\
         CallWindowProcW,'CallWindowProcW',\
         DefWindowProcW,'DefWindowProcW',\
         SetWindowLongPtrW,'SetWindowLongPtrW',\
         InvalidateRect,'InvalidateRect',\
         MoveWindow,'MoveWindow',\
         GetClientRect,'GetClientRect',\
         GetWindowRect,'GetWindowRect',\
         CreateDialogIndirectParamW,'CreateDialogIndirectParamW',\
         DialogBoxIndirectParamW,'DialogBoxIndirectParamW',\
         EnableWindow,'EnableWindow',\
         EndDialog,'EndDialog',\
         GetDlgItem,'GetDlgItem',\
         MessageBoxA,'MessageBoxA',\
         MessageBoxW,'MessageBoxW',\
         PostMessageW,'PostMessageW',\
         SendMessageW,'SendMessageW',\
         SendMessageA,'SendMessageA',\
         ValidateRect,'ValidateRect',\
         ReleaseDC,'ReleaseDC',\
         wsprintfA,'wsprintfA',\
         wsprintfW,'wsprintfW'

  import opengl,\
         glReadPixels,'glReadPixels',\
         glGetString,'glGetString',\
         glGetError,'glGetError',\
         glGenTextures,'glGenTextures',\
         glDeleteTextures,'glDeleteTextures',\
         glBindTexture,'glBindTexture',\
         glTexParameteri,'glTexParameteri',\
         glTexImage2D,'glTexImage2D',\
         glTexSubImage2D,'glTexSubImage2D',\
         glPixelStorei,'glPixelStorei',\
         glBlendFunc,'glBlendFunc',\
         glAlphaFunc,'glAlphaFunc',\
         glEnable,'glEnable',\
         glDisable,'glDisable',\
         glClearColor,'glClearColor',\
         glViewport,'glViewport',\
         glClear,'glClear',\
         glBegin,'glBegin',\
         glEnd,'glEnd',\
         glLoadMatrixf,'glLoadMatrixf',\
         glColor4f,'glColor4f',\
         glVertex2i,'glVertex2i',\
         glVertex4fv,'glVertex4fv',\
         glVertex4i,'glVertex4i',\
         glFinish,'glFinish',\
         wglCreateContext,'wglCreateContext',\
         wglDeleteContext,'wglDeleteContext',\
         wglMakeCurrent,'wglMakeCurrent',\
         wglGetProcAddress,'wglGetProcAddress'

  import gdi,\
         BeginPath,'BeginPath',\
         FillPath,'FillPath',\
         StrokePath,'StrokePath',\
         EndPath,'EndPath',\
         SaveDC,'SaveDC',\
         RestoreDC,'RestoreDC',\
         BitBlt,'BitBlt',\
         CreateCompatibleDC,'CreateCompatibleDC',\
         CreateDIBSection,'CreateDIBSection',\
         SetPixelFormat,'SetPixelFormat',\
         ChoosePixelFormat,'ChoosePixelFormat',\
         SetBkMode,'SetBkMode',\
         Rectangle,'Rectangle',\
         Ellipse,'Ellipse',\
         PolyDraw,'PolyDraw',\
         SetBkColor,'SetBkColor',\
         SetTextColor,'SetTextColor',\
         CreateSolidBrush,'CreateSolidBrush',\
         CreatePen,'CreatePen',\
         SelectObject,'SelectObject',\
         DeleteObject,'DeleteObject',\
         DeleteDC,'DeleteDC',\
         SwapBuffers,'SwapBuffers'

  import oleaut,\
         SafeArrayDestroy,'SafeArrayDestroy'

  import advapi,\
         RegOpenKeyExW,'RegOpenKeyExW',\
         RegQueryValueExW,'RegQueryValueExW',\
         RegCloseKey,'RegCloseKey'
end data