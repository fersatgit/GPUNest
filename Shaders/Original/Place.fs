#version 460
  in vec2 TexCoord;
  layout(binding=0)uniform sampler2D image;
  layout(binding=1)uniform sampler2DRect back;
  layout(binding=1)buffer SSBO1{uint PlacementMap[];};
  void main(){
    if((texture(image,vec2(TexCoord.x,clamp(TexCoord.y,0,0.5))).r*texture(back,gl_FragCoord.xy).r!=0)){
      PlacementMap[gl_PrimitiveID]=0xFFFFFFFF;
    }
  }