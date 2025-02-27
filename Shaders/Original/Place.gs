#version 460
  #define MAX_ROTATIONS_LEVEL 6
  #define MAX_ROTATIONS       (1<<MAX_ROTATIONS_LEVEL)
  #define DIST_TOP            0
  #define DIST_RIGHT          (MAX_ROTATIONS>>2)
  #define DIST_BOTTOM         ((MAX_ROTATIONS*2)>>2)
  #define DIST_LEFT           ((MAX_ROTATIONS*3)>>2)
  #define Center              vec2(0.5,0.25)
  #define PI                  3.1415926535897932384626433832795

  layout(binding=0)buffer SSBO0{ivec4 DistanceMap[MAX_ROTATIONS*2];};
  layout(binding=1)buffer SSBO1{uint PlacementMap[];};
  layout(binding=2)buffer SSBO2{uint FrameBuffer[];};

  layout(location=0)uniform vec4 Param; /*float 1/TexWidth,1/TexHeight,ScreenWidth,ScreenHeight*/
  layout(location=1)uniform int Rotations;

  layout(points,invocations=MAX_ROTATIONS) in;
  layout(triangle_strip, max_vertices=4) out;
  out vec2 TexCoord;

  void main(){
    if(gl_InvocationID>>Rotations==0){
      uint i=gl_InvocationID<<(MAX_ROTATIONS_LEVEL-Rotations);
      float a=i*(PI*2)/MAX_ROTATIONS;

      ivec4 top   =DistanceMap[i+DIST_TOP];
      ivec4 right =DistanceMap[i+DIST_RIGHT];
      ivec4 bottom=DistanceMap[i+DIST_BOTTOM];
      ivec4 left  =DistanceMap[i+DIST_LEFT];

      ivec2 pos;
      switch(int(gl_in[0].gl_Position.w)){
        case 0:pos=ivec2(gl_in[0].gl_Position.xy)-ivec2(0,left.z+bottom.y);
               break;
        case 1:pos=ivec2(gl_in[0].gl_Position.xy)+ivec2(bottom.x-left.y,0);
               break;
        case 2:pos=ivec2(gl_in[0].gl_Position.xy)+ivec2(-left.y,right.x)-ivec2(right.y,bottom.y);
               break;
      }
      ivec2 UpperRight=ivec2(left.y,bottom.y)+ivec2(right.y,top.y)+pos;

      gl_PrimitiveID=(int(gl_in[0].gl_Position.z)<<Rotations)+gl_InvocationID;
      if(any(lessThan(vec4(pos.xy,Param.zw),vec4(0,0,UpperRight)))){
        PlacementMap[gl_PrimitiveID]=0xFFFFFFFF;
        return;
      }

      vec4 Rect=fma(vec4(pos,UpperRight),2/Param.zwzw,vec4(-1,-1,-1,-1));
      ivec2 center=ivec2(pos+ivec2(left.y,bottom.y));

      ivec3 index=(center.yyy-ivec3(-left.x,bottom.y,right.x))*ivec3(Param.zzz)+center.xxx-ivec3(left.y+1,bottom.x,1-right.y);
      if(any(greaterThan(uvec3(FrameBuffer[index.x],FrameBuffer[index.y],FrameBuffer[index.z]),uvec3(0,0,0)))){
        PlacementMap[gl_PrimitiveID]=0xFFFFFFFF;
        return;
      }

      float b=atan(left.y,top.y)-a;
      gl_Position=vec4(Rect.xw,0,1);
      TexCoord=fma(vec2(-sin(b),cos(b))*length(vec2(left.y,top.y)),Param.xy,Center);
      EmitVertex();
      b=-atan(right.y,top.y)-a;
      gl_Position=vec4(Rect.zw,0,1);
      TexCoord=fma(vec2(-sin(b),cos(b))*length(vec2(right.y,top.y)),Param.xy,Center);
      EmitVertex();
      b=PI-atan(left.y,bottom.y)-a;
      gl_Position=vec4(Rect.xy,0,1);
      TexCoord=fma(vec2(-sin(b),cos(b))*length(vec2(left.y,bottom.y)),Param.xy,Center);
      EmitVertex();
      b=PI+atan(right.y,bottom.y)-a;
      gl_Position=vec4(Rect.zy,0,1);
      TexCoord=fma(vec2(-sin(b),cos(b))*length(vec2(right.y,bottom.y)),Param.xy,Center);
      EmitVertex();
      EndPrimitive();
    }
  }