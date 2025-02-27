#version 460
  in vec2 TexCoord;
  out vec4 Color;
  layout(location=1)uniform vec4 DrawColor;
  layout(binding=0)uniform sampler2D image;
  void main(){
    Color = texture(image,vec2(TexCoord.x,clamp(TexCoord.y,0,0.5))).rrrr*DrawColor;
  }