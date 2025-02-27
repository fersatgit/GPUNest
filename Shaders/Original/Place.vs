#version 460
  in vec4 Pos;
  void main(){gl_Position=vec4(Pos.xy,gl_VertexID,Pos.z);}
