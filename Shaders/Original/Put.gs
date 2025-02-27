#version 460
  #define MAX_ROTATIONS_LEVEL 6
  #define MAX_ROTATIONS       (1<<MAX_ROTATIONS_LEVEL)
  #define DIST_TOP            0
  #define DIST_RIGHT          (MAX_ROTATIONS>>2)
  #define DIST_BOTTOM         ((MAX_ROTATIONS*2)>>2)
  #define DIST_LEFT           ((MAX_ROTATIONS*3)>>2)
  #define PI                  3.1415926535897932384626433832795
  #define Center              vec2(0.5,0.25)
  layout(binding=0)buffer SSBO0{ivec4 DistanceMap[MAX_ROTATIONS*2];};
  layout(points) in;
  layout(triangle_strip, max_vertices=4) out;
  out vec2 TexCoord;
  layout(location=0)uniform vec4 Param; /*float 1/TexWidth,1/TexHeight,ScreenWidth,ScreenHeight*/
  void main(){
  int i=int(gl_in[0].gl_Position.z);
  float a=i*PI*2/MAX_ROTATIONS;

  int top   =DistanceMap[i+DIST_TOP].y;
  int right =DistanceMap[i+DIST_RIGHT].y;
  int bottom=DistanceMap[i+DIST_BOTTOM].y;
  int left  =DistanceMap[i+DIST_LEFT].y;

  vec4  Rect=fma(gl_in[0].gl_Position.xyxy+vec4(-left,-bottom,right,top),2/Param.zwzw,vec4(-1,-1,-1,-1));

  float b=atan(left,top)-a;
  gl_Position=vec4(Rect.xw,0,1);
  TexCoord=fma(vec2(-sin(b),cos(b))*length(vec2(left,top)),Param.xy,Center);
  EmitVertex();
  b=-atan(right,top)-a;
  gl_Position=vec4(Rect.zw,0,1);
  TexCoord=fma(vec2(-sin(b),cos(b))*length(vec2(right,top)),Param.xy,Center);
  EmitVertex();
  b=PI-atan(left,bottom)-a;
  gl_Position=vec4(Rect.xy,0,1);
  TexCoord=fma(vec2(-sin(b),cos(b))*length(vec2(left,bottom)),Param.xy,Center);
  EmitVertex();
  b=PI+atan(right,bottom)-a;
  gl_Position=vec4(Rect.zy,0,1);
  TexCoord=fma(vec2(-sin(b),cos(b))*length(vec2(right,bottom)),Param.xy,Center);
  EmitVertex();
  EndPrimitive();}