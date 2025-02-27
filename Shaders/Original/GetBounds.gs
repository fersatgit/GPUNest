#version 460
  #define MAX_ROTATIONS_LEVEL 6
  #define MAX_ROTATIONS       (1<<MAX_ROTATIONS_LEVEL)
  const vec2 center=vec2(0.5,0.25);
  layout(points,invocations=MAX_ROTATIONS) in;
  layout(triangle_strip, max_vertices=4) out;
  out vec2 TexCoord;
  void main(){
    float a=6.283185307179586476925286766559/MAX_ROTATIONS*gl_InvocationID;
    mat4 RotMat=mat4(cos(a),-sin(a),0.0,0.0,
                     sin(a),cos(a),0.0,0.0,
                     0.0,0.0,0.0,0.0,
                     0.0,0.0,0.0,1.0);
    gl_PrimitiveID=gl_InvocationID;
    gl_Position=vec4(-gl_in[0].gl_Position.x, gl_in[0].gl_Position.y,0.0,1.0)*RotMat;
    TexCoord=vec2(-gl_in[0].gl_Position.z,gl_in[0].gl_Position.w)+center;
    EmitVertex();
    gl_Position=vec4(gl_in[0].gl_Position.xy,0.0,1.0)*RotMat;
    TexCoord=gl_in[0].gl_Position.zw+center;
    EmitVertex();
    gl_Position=vec4(-gl_in[0].gl_Position.xy,0.0,1.0)*RotMat;
    TexCoord=center-gl_in[0].gl_Position.zw;
    EmitVertex();
    gl_Position=vec4(gl_in[0].gl_Position.x,-gl_in[0].gl_Position.y,0.0,1.0)*RotMat;
    TexCoord=vec2(gl_in[0].gl_Position.z,-gl_in[0].gl_Position.w)+center;
    EmitVertex();
    EndPrimitive();
  }