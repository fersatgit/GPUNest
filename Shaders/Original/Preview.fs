#version 460
  in vec3 TexCoord;
  out vec4 Color;
  layout(binding=0)uniform sampler2D image;
  void main(){
    Color=texture(image,TexCoord.xy).rrrr*vec4(0.45,0.45,0.45,TexCoord.z);
  }