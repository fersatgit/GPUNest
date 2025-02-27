#version 460
  #define PI 3.1415926535897932384626433832795

  layout(points) in;
  layout(triangle_strip, max_vertices=8) out;
  out vec3 TexCoord;

  layout(location=0)uniform vec4 Param; /*TParam{int TexWidth,TexHeight,ScreenWidth,ScreenHeight;};*/
  layout(location=1)uniform mat4 MVP;

  void main(){
    float a=gl_in[0].gl_Position.z*PI/32;
    float b=atan(Param.x,Param.y);
    float c=length(Param.xy)*0.5;

    gl_Position=MVP*vec4(gl_in[0].gl_Position.xy+vec2(-sin(a-b),cos(a-b))*c,0,1);
    TexCoord=vec3(1,1,1);
    EmitVertex();
    gl_Position=MVP*vec4(gl_in[0].gl_Position.xy+vec2(-sin(a+b),cos(a+b))*c,0,1);
    TexCoord=vec3(0,1,1);
    EmitVertex();
    gl_Position=MVP*vec4(gl_in[0].gl_Position.xy+vec2(-sin(PI+a+b),cos(PI+a+b))*c,0,1);
    TexCoord=vec3(1,0.5,1);
    EmitVertex();
    gl_Position=MVP*vec4(gl_in[0].gl_Position.xy+vec2(-sin(PI+a-b),cos(PI+a-b))*c,0,1);
    TexCoord=vec3(0,0.5,1);
    EmitVertex();
    EndPrimitive();

    gl_Position=MVP*vec4(gl_in[0].gl_Position.xy+vec2(-sin(a-b),cos(a-b))*c,0,1);
    TexCoord=vec3(1,0.5,0.5);
    EmitVertex();
    gl_Position=MVP*vec4(gl_in[0].gl_Position.xy+vec2(-sin(a+b),cos(a+b))*c,0,1);
    TexCoord=vec3(0,0.5,0.5);
    EmitVertex();
    gl_Position=MVP*vec4(gl_in[0].gl_Position.xy+vec2(-sin(PI+a+b),cos(PI+a+b))*c,0,1);
    TexCoord=vec3(1,0,0.5);
    EmitVertex();
    gl_Position=MVP*vec4(gl_in[0].gl_Position.xy+vec2(-sin(PI+a-b),cos(PI+a-b))*c,0,1);
    TexCoord=vec3(0,0,0.5);
    EmitVertex();
    EndPrimitive();
  }